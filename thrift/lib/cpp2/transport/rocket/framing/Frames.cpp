/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <thrift/lib/cpp2/transport/rocket/framing/Frames.h>

#include <chrono>
#include <type_traits>
#include <utility>

#include <glog/logging.h>

#include <fmt/core.h>
#include <folly/CPortability.h>
#include <folly/Likely.h>
#include <folly/Range.h>
#include <folly/Utility.h>
#include <folly/functional/Invoke.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>

#include <thrift/lib/cpp2/transport/rocket/Types.h>
#include <thrift/lib/cpp2/transport/rocket/framing/FrameType.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Serializer.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Util.h>

namespace apache {
namespace thrift {
namespace rocket {

namespace {
// All frame sizes (header size + payload size) are encoded in 3 bytes
constexpr size_t kMaxFragmentedPayloadSize = 0xffffff - 512;

Payload readPayload(
    bool expectingMetadata,
    folly::io::Cursor& cursor,
    std::unique_ptr<folly::IOBuf> buffer) {
  size_t metadataSize = 0;
  if (expectingMetadata) {
    metadataSize = readFrameOrMetadataSize(cursor);
  }

  buffer->trimStart(cursor.getCurrentPosition());
  return Payload::makeCombined(std::move(buffer), metadataSize);
}

template <class Frame>
void serializeInFragmentsSlowCommon(
    Frame&& frame,
    Flags flags,
    Serializer& writer) {
  auto metadataSize = frame.payload().metadataSize();
  folly::IOBufQueue bufferQueue(folly::IOBufQueue::cacheChainLength());
  bufferQueue.append(std::move(frame.payload()).buffer());

  bool isFirstFrame = true;
  bool finished = false;
  do {
    size_t metadataChunk = std::min(metadataSize, kMaxFragmentedPayloadSize);
    metadataSize -= metadataChunk;
    auto chunk = bufferQueue.splitAtMost(kMaxFragmentedPayloadSize);
    finished = bufferQueue.empty();

    auto p = Payload::makeCombined(std::move(chunk), metadataChunk);
    if (std::exchange(isFirstFrame, false)) {
      frame.payload() = std::move(p);
      frame.setHasFollows(!finished);
      std::move(frame).serialize(writer);
    } else {
      PayloadFrame pf(frame.streamId(), std::move(p), flags.follows(!finished));
      std::move(pf).serialize(writer);
    }
  } while (!finished);
}

FOLLY_CREATE_MEMBER_INVOKE_TRAITS(
    has_initial_request_n_traits,
    initialRequestN);

template <typename T>
using has_initial_request_n = has_initial_request_n_traits::is_invocable<T>;

template <class Frame>
std::enable_if_t<has_initial_request_n<Frame>::value, void>
serializeInitialNIfPresent(const Frame& frame, HeaderSerializer& serializer) {
  serializer.writeBE<uint32_t>(frame.initialRequestN());
}

template <class Frame>
std::enable_if_t<!has_initial_request_n<Frame>::value, void>
serializeInitialNIfPresent(const Frame&, HeaderSerializer&) {}

template <class Frame>
std::unique_ptr<folly::IOBuf> serializeIntoIOBuf(Frame&& frame) {
  Serializer writer;
  std::move(frame).serialize(writer);
  return std::move(writer).move();
}

template <class Frame>
std::unique_ptr<folly::IOBuf> serializeIntoHeadroom(Frame&& frame) {
  auto flags = frame.flags();
  // We can assume here that we have non-zero buffer with adequate
  // headroom for the rsocket header.
  auto dataLen = frame.payload().dataSize();
  auto metadataLen = frame.payload().metadataSize();
  auto buffer = std::move(frame.payload()).buffer();

  constexpr size_t rsocketLen =
      (2 * Serializer::kBytesForFrameOrMetadataLength) +
      Frame::frameHeaderSize();
  DCHECK(buffer->headroom() >= rsocketLen);

  // Write rsocket header directly into the buffer headroom.
  HeaderSerializer writer(buffer->writableData() - rsocketLen, rsocketLen);
  const size_t frameSize = frame.frameHeaderSize() + dataLen + metadataLen +
      Serializer::kBytesForFrameOrMetadataLength;
  writer.writeFrameOrMetadataSize(frameSize);
  writer.write(frame.streamId());
  writer.writeFrameTypeAndFlags(frame.frameType(), flags);
  serializeInitialNIfPresent(frame, writer);
  writer.writeFrameOrMetadataSize(metadataLen);
  DCHECK_EQ(writer.result().size(), rsocketLen);
  buffer->prepend(rsocketLen);

  return buffer;
}

template <class Frame>
std::unique_ptr<folly::IOBuf> serializeIntoHeadroomIfPossible(Frame&& frame) {
  constexpr size_t kHeadroomSize =
      Frame::frameHeaderSize() + 2 * Serializer::kBytesForFrameOrMetadataLength;
  if (LIKELY(
          frame.payload().metadataAndDataSize() <= kMaxFragmentedPayloadSize &&
          frame.payload().hasNonemptyMetadata() &&
          frame.payload().buffer()->headroom() >= kHeadroomSize)) {
    return serializeIntoHeadroom(std::move(frame));
  } else {
    return serializeIntoIOBuf(std::move(frame));
  }
}
} // namespace

std::unique_ptr<folly::IOBuf> SetupFrame::serialize() && {
  return serializeIntoIOBuf(std::move(*this));
}

void SetupFrame::serialize(Serializer& writer) && {
  /**
   *  0                   1                   2                   3
   *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |                         Stream ID = 0                         |
   * +-----------+-+-+-+-+-----------+-------------------------------+
   * |Frame Type |0|M|R|L|  Flags    |
   * +-----------+-+-+-+-+-----------+-------------------------------+
   * |         Major Version         |        Minor Version          |
   * +-------------------------------+-------------------------------+
   * |0|                 Time Between KEEPALIVE Frames               |
   * +---------------------------------------------------------------+
   * |0|                       Max Lifetime                          |
   * +---------------------------------------------------------------+
   * |         Token Length          | Resume Identification Token  ...
   * +---------------+-----------------------------------------------+
   * |  MIME Length  |   Metadata Encoding MIME Type                ...
   * +---------------+-----------------------------------------------+
   * |  MIME Length  |     Data Encoding MIME Type                  ...
   * +---------------+-----------------------------------------------+
   *                       Metadata & Setup Payload
   */

  // Excludes room for frame length
  const auto frameSize = frameHeaderSize() + payload().serializedSize();
  auto nwritten = writer.writeFrameOrMetadataSize(frameSize);

  // SETUP frames are only sent when connections are first established, so we
  // forgo potential batching optimizations.
  nwritten += writer.write(StreamId{0});

  // The metadata flag can get out of sync with whether the payload actually has
  // metadata. In order to work around this and any other potential
  // inconsistencies, we recompute all flags during serialization as opposed to
  // serializing flags_ directly. This comment applies to all frames.
  nwritten += writer.writeFrameTypeAndFlags(
      frameType(),
      Flags::none()
          .metadata(payload_.hasNonemptyMetadata())
          .resumeToken(hasResumeIdentificationToken()));

  // Major and minor version. Our rsocket implementation only handles rsocket
  // protocol version 1.0.
  nwritten += writer.writeBE<uint16_t>(1); // rsocket major version
  nwritten += writer.writeBE<uint16_t>(0); // rsocket minor version

  // Time between KEEPALVE frames. Note that KEEPALIVE frames are unsupported.
  // Note: keepalive time MUST be > 0 and at most 2^31 - 1.
  constexpr std::chrono::milliseconds kMaxKeepaliveTime{(1ull << 31) - 1};
  nwritten += writer.writeBE<uint32_t>(kMaxKeepaliveTime.count());

  // Max lifetime. Protocol specifies that max lifetime MUST be > 0, but not
  // used in this implementation.
  constexpr std::chrono::milliseconds kMaxLifetime{(1ull << 31) - 1};
  nwritten += writer.writeBE<uint32_t>(kMaxLifetime.count());

  // Resume identification token length and token are not present if 'R' flag is
  // not present.
  if (hasResumeIdentificationToken()) {
    // Resume identification token length and token
    nwritten += writer.writeBE<uint16_t>(resumeIdentificationToken_.size());
    nwritten += writer.write(resumeIdentificationToken_);
  }

  // Length of metadata MIME type
  nwritten += writer.writeBE<uint8_t>(static_cast<uint8_t>(kMimeType.size()));
  nwritten += writer.write(kMimeType);

  // Length of data MIME type
  nwritten += writer.writeBE<uint8_t>(static_cast<uint8_t>(kMimeType.size()));
  nwritten += writer.write(kMimeType);

  // Setup metadata and data
  nwritten += writer.writePayload(std::move(payload()));

  DCHECK_EQ(Serializer::kBytesForFrameOrMetadataLength + frameSize, nwritten);
}

std::unique_ptr<folly::IOBuf> RequestResponseFrame::serialize() && {
  return serializeIntoHeadroomIfPossible(std::move(*this));
}

void RequestResponseFrame::serialize(Serializer& writer) && {
  if (UNLIKELY(payload().metadataAndDataSize() > kMaxFragmentedPayloadSize)) {
    return std::move(*this).serializeInFragmentsSlow(writer);
  }
  std::move(*this).serializeIntoSingleFrame(writer);
}

void RequestResponseFrame::serializeIntoSingleFrame(Serializer& writer) && {
  /**
   *  0                   1                   2                   3
   *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |                           Stream ID                           |
   * +-----------+-+-+-+-------------+-------------------------------+
   * |Frame Type |0|M|F|     Flags   |
   * +-------------------------------+
   *                      Metadata & Request Data
   */

  // Excludes room for frame length
  const auto frameSize = frameHeaderSize() + payload().serializedSize();
  auto nwritten = writer.writeFrameOrMetadataSize(frameSize);

  nwritten += writer.write(streamId());
  nwritten += writer.writeFrameTypeAndFlags(
      frameType(),
      Flags::none()
          .metadata(payload_.hasNonemptyMetadata())
          .follows(hasFollows()));
  nwritten += writer.writePayload(std::move(payload()));

  DCHECK_EQ(Serializer::kBytesForFrameOrMetadataLength + frameSize, nwritten);
}

std::unique_ptr<folly::IOBuf> RequestFnfFrame::serialize() && {
  return serializeIntoHeadroomIfPossible(std::move(*this));
}

void RequestFnfFrame::serialize(Serializer& writer) && {
  if (UNLIKELY(payload().metadataAndDataSize() > kMaxFragmentedPayloadSize)) {
    return std::move(*this).serializeInFragmentsSlow(writer);
  }
  std::move(*this).serializeIntoSingleFrame(writer);
}

void RequestFnfFrame::serializeIntoSingleFrame(Serializer& writer) && {
  /**
   *  0                   1                   2                   3
   *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |                           Stream ID                           |
   * +-----------+-+-+-+-------------+-------------------------------+
   * |Frame Type |0|M|F|    Flags    |
   * +-------------------------------+
   *                      Metadata & Request Data
   */

  // Excludes room for frame length
  const auto frameSize = frameHeaderSize() + payload().serializedSize();
  auto nwritten = writer.writeFrameOrMetadataSize(frameSize);

  nwritten += writer.write(streamId());
  nwritten += writer.writeFrameTypeAndFlags(
      frameType(),
      Flags::none()
          .metadata(payload_.hasNonemptyMetadata())
          .follows(hasFollows()));
  nwritten += writer.writePayload(std::move(payload()));

  DCHECK_EQ(Serializer::kBytesForFrameOrMetadataLength + frameSize, nwritten);
}

std::unique_ptr<folly::IOBuf> RequestStreamFrame::serialize() && {
  return serializeIntoHeadroomIfPossible(std::move(*this));
}

void RequestStreamFrame::serialize(Serializer& writer) && {
  if (UNLIKELY(payload().metadataAndDataSize() > kMaxFragmentedPayloadSize)) {
    return std::move(*this).serializeInFragmentsSlow(writer);
  }
  std::move(*this).serializeIntoSingleFrame(writer);
}

void RequestStreamFrame::serializeIntoSingleFrame(Serializer& writer) && {
  /**
   *  0                   1                   2                   3
   *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |                           Stream ID                           |
   * +-----------+-+-+-+-------------+-------------------------------+
   * |Frame Type |0|M|F|    Flags    |
   * +-------------------------------+-------------------------------+
   * |0|                    Initial Request N                        |
   * +---------------------------------------------------------------+
   *                       Metadata & Request Data
   */

  // Excludes room for frame length
  const auto frameSize = frameHeaderSize() + payload().serializedSize();
  auto nwritten = writer.writeFrameOrMetadataSize(frameSize);

  nwritten += writer.write(streamId());
  nwritten += writer.writeFrameTypeAndFlags(
      frameType(),
      Flags::none()
          .metadata(payload_.hasNonemptyMetadata())
          .follows(hasFollows()));
  nwritten += writer.writeBE<uint32_t>(initialRequestN());
  nwritten += writer.writePayload(std::move(payload()));

  DCHECK_EQ(Serializer::kBytesForFrameOrMetadataLength + frameSize, nwritten);
}

std::unique_ptr<folly::IOBuf> RequestChannelFrame::serialize() && {
  return serializeIntoHeadroomIfPossible(std::move(*this));
}

void RequestChannelFrame::serialize(Serializer& writer) && {
  if (UNLIKELY(payload().metadataAndDataSize() > kMaxFragmentedPayloadSize)) {
    return std::move(*this).serializeInFragmentsSlow(writer);
  }
  std::move(*this).serializeIntoSingleFrame(writer);
}

void RequestChannelFrame::serializeIntoSingleFrame(Serializer& writer) && {
  /**
   *  0                   1                   2                   3
   *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |                           Stream ID                           |
   * +-----------+-+-+-+-+-----------+-------------------------------+
   * |Frame Type |0|M|F|C|  Flags    |
   * +-------------------------------+-------------------------------+
   * |0|                    Initial Request N                        |
   * +---------------------------------------------------------------+
   *                        Metadata & Request Data
   */
  // Excludes room for frame length
  const auto frameSize = frameHeaderSize() + payload().serializedSize();
  auto nwritten = writer.writeFrameOrMetadataSize(frameSize);

  nwritten += writer.write(streamId());
  nwritten += writer.writeFrameTypeAndFlags(
      frameType(),
      Flags::none()
          .metadata(payload_.hasNonemptyMetadata())
          .follows(hasFollows())
          .complete(hasComplete()));
  nwritten += writer.writeBE<uint32_t>(initialRequestN());
  nwritten += writer.writePayload(std::move(payload()));

  DCHECK_EQ(Serializer::kBytesForFrameOrMetadataLength + frameSize, nwritten);
}

std::unique_ptr<folly::IOBuf> RequestNFrame::serialize() && {
  return serializeIntoIOBuf(std::move(*this));
}

void RequestNFrame::serialize(Serializer& writer) && {
  /**
   *  0                   1                   2                   3
   *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |                           Stream ID                           |
   * +-----------+-+-+---------------+-------------------------------+
   * |Frame Type |0|0|     Flags     |
   * +-------------------------------+-------------------------------+
   * |0|                         Request N                           |
   * +---------------------------------------------------------------+
   */

  // Excludes room for frame length
  constexpr auto frameSize = frameHeaderSize();
  auto nwritten = writer.writeFrameOrMetadataSize(frameSize);

  nwritten += writer.write(streamId());
  nwritten += writer.writeFrameTypeAndFlags(frameType(), Flags::none());
  nwritten += writer.writeBE<uint32_t>(requestN());

  DCHECK_EQ(Serializer::kBytesForFrameOrMetadataLength + frameSize, nwritten);
}

std::unique_ptr<folly::IOBuf> CancelFrame::serialize() && {
  return serializeIntoIOBuf(std::move(*this));
}

void CancelFrame::serialize(Serializer& writer) && {
  /**
   *  0                   1                   2                   3
   *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |                           Stream ID                           |
   * +-----------+-+-+---------------+-------------------------------+
   * |Frame Type |0|0|    Flags      |
   * +-------------------------------+-------------------------------+
   */

  // Excludes room for frame length
  constexpr auto frameSize = frameHeaderSize();
  auto nwritten = writer.writeFrameOrMetadataSize(frameSize);

  nwritten += writer.write(streamId());
  nwritten += writer.writeFrameTypeAndFlags(frameType(), Flags::none());

  DCHECK_EQ(Serializer::kBytesForFrameOrMetadataLength + frameSize, nwritten);
}

void PayloadFrame::serialize(Serializer& writer) && {
  if (UNLIKELY(payload().metadataAndDataSize() > kMaxFragmentedPayloadSize)) {
    return std::move(*this).serializeInFragmentsSlow(writer);
  }
  std::move(*this).serializeIntoSingleFrame(writer);
}

void PayloadFrame::serializeIntoSingleFrame(Serializer& writer) && {
  /**
   *  0                   1                   2                   3
   *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |                           Stream ID                           |
   * +-----------+-+-+-+-+-+---------+-------------------------------+
   * |Frame Type |0|M|F|C|N|  Flags  |
   * +-------------------------------+-------------------------------+
   *                    Metadata & Data
   */

  // Excludes room for frame length
  const auto frameSize = frameHeaderSize() + payload().serializedSize();
  auto nwritten = writer.writeFrameOrMetadataSize(frameSize);

  nwritten += writer.write(streamId());

  // Protocol states that complete or next (or both) MUST be set... but this
  // appears to not hold in the case of Payload fragment frames.
  // DCHECK(hasComplete() || hasNext());
  nwritten += writer.writeFrameTypeAndFlags(
      frameType(),
      Flags::none()
          .metadata(payload_.hasNonemptyMetadata())
          .follows(hasFollows())
          .complete(hasComplete())
          .next(hasNext()));

  nwritten += writer.writePayload(std::move(payload()));

  DCHECK_EQ(Serializer::kBytesForFrameOrMetadataLength + frameSize, nwritten);
}

std::unique_ptr<folly::IOBuf> PayloadFrame::serialize() && {
  return serializeIntoHeadroomIfPossible(std::move(*this));
}

std::unique_ptr<folly::IOBuf> ErrorFrame::serialize() && {
  return serializeIntoIOBuf(std::move(*this));
}

void ErrorFrame::serialize(Serializer& writer) && {
  /**
   *  0                   1                   2                   3
   *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |                           Stream ID                           |
   * +-----------+-+-+---------------+-------------------------------+
   * |Frame Type |0|0|      Flags    |
   * +-----------+-+-+---------------+-------------------------------+
   * |                          Error Code                           |
   * +---------------------------------------------------------------+
   *                            Error Payload
   */

  // Excludes room for frame length
  const auto frameSize = frameHeaderSize() + payload().serializedSize();
  auto nwritten = writer.writeFrameOrMetadataSize(frameSize);

  nwritten += writer.write(streamId());
  nwritten += writer.writeFrameTypeAndFlags(frameType(), Flags::none());

  nwritten += writer.writeBE(folly::to_underlying(errorCode()));

  nwritten += writer.writePayload(std::move(payload()));

  DCHECK_EQ(Serializer::kBytesForFrameOrMetadataLength + frameSize, nwritten);
}

void MetadataPushFrame::serialize(Serializer& writer) && {
  /**
   *  0                   1                   2                   3
   *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |                         Stream ID = 0                         |
   * +-----------+-+-+---------------+-------------------------------+
   * |Frame Type |0|1|     Flags     |
   * +-------------------------------+-------------------------------+
   *                              Metadata
   */

  // Excludes room for frame length
  const auto frameSize =
      frameHeaderSize() + metadata_->computeChainDataLength();
  auto nwritten = writer.writeFrameOrMetadataSize(frameSize);

  nwritten += writer.write(StreamId{0});
  nwritten +=
      writer.writeFrameTypeAndFlags(frameType(), Flags::none().metadata(true));
  nwritten += writer.write(std::move(metadata_));

  DCHECK_EQ(Serializer::kBytesForFrameOrMetadataLength + frameSize, nwritten);
}

std::unique_ptr<folly::IOBuf> MetadataPushFrame::serialize() && {
  Serializer writer;
  std::move(*this).serialize(writer);
  return std::move(writer).move();
}

std::unique_ptr<folly::IOBuf> KeepAliveFrame::serialize() && {
  return serializeIntoIOBuf(std::move(*this));
}

void KeepAliveFrame::serialize(Serializer& writer) && {
  /**
   * 0                   1                   2                   3
   * 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |                         Stream ID = 0                         |
   * +-----------+-+-+-+-------------+-------------------------------+
   * |Frame Type |0|0|R|    Flags    |
   * +-----------+-+-+-+-------------+-------------------------------+
   * |0|                  Last Received Position                     |
   * +                                                               +
   * |                                                               |
   * +---------------------------------------------------------------+
   *                              Data
   */

  // Excludes room for frame length
  const auto frameSize = frameHeaderSize() + data_->computeChainDataLength();
  auto nwritten = writer.writeFrameOrMetadataSize(frameSize);

  nwritten += writer.write(StreamId{0});
  nwritten += writer.writeFrameTypeAndFlags(frameType(), flags_);

  // Last received position: send 0 if not supported.
  nwritten += writer.writeBE(static_cast<uint64_t>(0));

  nwritten += writer.write(std::move(data_));

  DCHECK_EQ(Serializer::kBytesForFrameOrMetadataLength + frameSize, nwritten);
}

FOLLY_NOINLINE void RequestResponseFrame::serializeInFragmentsSlow(
    Serializer& writer) && {
  serializeInFragmentsSlowCommon(std::move(*this), Flags::none(), writer);
}

FOLLY_NOINLINE void RequestFnfFrame::serializeInFragmentsSlow(
    Serializer& writer) && {
  serializeInFragmentsSlowCommon(std::move(*this), Flags::none(), writer);
}

FOLLY_NOINLINE void RequestStreamFrame::serializeInFragmentsSlow(
    Serializer& writer) && {
  serializeInFragmentsSlowCommon(std::move(*this), Flags::none(), writer);
}

FOLLY_NOINLINE void RequestChannelFrame::serializeInFragmentsSlow(
    Serializer& writer) && {
  serializeInFragmentsSlowCommon(
      std::move(*this), Flags::none().complete(hasComplete()), writer);
}

FOLLY_NOINLINE void PayloadFrame::serializeInFragmentsSlow(
    Serializer& writer) && {
  serializeInFragmentsSlowCommon(
      std::move(*this),
      Flags::none().complete(hasComplete()).next(hasNext()),
      writer);
}

SetupFrame::SetupFrame(std::unique_ptr<folly::IOBuf> frame) {
  // Trick to avoid the default-constructed IOBuf. See expanded comment in
  // PayloadFrame constructor. Do this optimization in Setup frame for
  // consistency, not performance.
  DCHECK(!frame->isChained());

  folly::io::Cursor cursor(frame.get());
  const StreamId zero(readStreamId(cursor));
  DCHECK_EQ(StreamId{0}, zero);

  FrameType type;
  std::tie(type, flags_) = readFrameTypeAndFlags(cursor);

  const auto majorVersion = cursor.readBE<uint16_t>();
  const auto minorVersion = cursor.readBE<uint16_t>();

  if (majorVersion != 1 || minorVersion != 0) {
    throw std::runtime_error(fmt::format(
        "SETUP frame received with unsupported version {}.{}",
        majorVersion,
        minorVersion));
  }

  // Skip keep-alive interval (4 bytes) and max lifetime (4 bytes). These values
  // are not currently used in Thrift.
  cursor.skip(8);

  // Resumption is not currently supported, but we handle the resume
  // identification token properly in case remote end sends a token.
  if (hasResumeIdentificationToken()) {
    const auto tokenLength = cursor.readBE<uint16_t>();
    cursor.skip(tokenLength);
  }

  // MIME types are currently not used, but we still handle the bytes properly.
  const auto metadataMimeLength = cursor.read<uint8_t>();
  cursor.skip(metadataMimeLength);
  const auto dataMimeLength = cursor.read<uint8_t>();
  cursor.skip(dataMimeLength);

  payload_ = readPayload(flags_.metadata(), cursor, std::move(frame));
}

RequestResponseFrame::RequestResponseFrame(
    std::unique_ptr<folly::IOBuf> frame) {
  folly::io::Cursor cursor(frame.get());
  DCHECK(!frame->isChained());

  streamId_ = readStreamId(cursor);

  FrameType type;
  std::tie(type, flags_) = readFrameTypeAndFlags(cursor);
  DCHECK(frameType() == type);

  payload_ = readPayload(flags_.metadata(), cursor, std::move(frame));
}

RequestResponseFrame::RequestResponseFrame(
    StreamId streamId,
    Flags flags,
    folly::io::Cursor& cursor,
    std::unique_ptr<folly::IOBuf> underlyingBuffer)
    : streamId_(streamId), flags_(flags) {
  payload_ =
      readPayload(flags_.metadata(), cursor, std::move(underlyingBuffer));
}

RequestFnfFrame::RequestFnfFrame(std::unique_ptr<folly::IOBuf> frame) {
  folly::io::Cursor cursor(frame.get());
  DCHECK(!frame->isChained());

  streamId_ = readStreamId(cursor);

  FrameType type;
  std::tie(type, flags_) = readFrameTypeAndFlags(cursor);
  DCHECK(frameType() == type);

  payload_ = readPayload(flags_.metadata(), cursor, std::move(frame));
}

RequestFnfFrame::RequestFnfFrame(
    StreamId streamId,
    Flags flags,
    folly::io::Cursor& cursor,
    std::unique_ptr<folly::IOBuf> underlyingBuffer)
    : streamId_(streamId), flags_(flags) {
  payload_ =
      readPayload(flags_.metadata(), cursor, std::move(underlyingBuffer));
}

RequestStreamFrame::RequestStreamFrame(std::unique_ptr<folly::IOBuf> frame) {
  folly::io::Cursor cursor(frame.get());
  DCHECK(!frame->isChained());

  streamId_ = readStreamId(cursor);

  FrameType type;
  std::tie(type, flags_) = readFrameTypeAndFlags(cursor);
  DCHECK(frameType() == type);

  initialRequestN_ = cursor.readBE<int32_t>();

  payload_ = readPayload(flags_.metadata(), cursor, std::move(frame));
}

RequestStreamFrame::RequestStreamFrame(
    StreamId streamId,
    Flags flags,
    folly::io::Cursor& cursor,
    std::unique_ptr<folly::IOBuf> underlyingBuffer)
    : streamId_(streamId), flags_(flags) {
  initialRequestN_ = cursor.readBE<int32_t>();
  payload_ =
      readPayload(flags_.metadata(), cursor, std::move(underlyingBuffer));
}

RequestChannelFrame::RequestChannelFrame(std::unique_ptr<folly::IOBuf> frame) {
  folly::io::Cursor cursor(frame.get());
  DCHECK(!frame->isChained());

  streamId_ = readStreamId(cursor);

  FrameType type;
  std::tie(type, flags_) = readFrameTypeAndFlags(cursor);
  DCHECK(frameType() == type);

  initialRequestN_ = cursor.readBE<int32_t>();

  payload_ = readPayload(flags_.metadata(), cursor, std::move(frame));
}

RequestChannelFrame::RequestChannelFrame(
    StreamId streamId,
    Flags flags,
    folly::io::Cursor& cursor,
    std::unique_ptr<folly::IOBuf> underlyingBuffer)
    : streamId_(streamId), flags_(flags) {
  initialRequestN_ = cursor.readBE<int32_t>();
  payload_ =
      readPayload(flags_.metadata(), cursor, std::move(underlyingBuffer));
}

RequestNFrame::RequestNFrame(std::unique_ptr<folly::IOBuf> frame) {
  folly::io::Cursor cursor(frame.get());
  DCHECK(!frame->isChained());

  streamId_ = readStreamId(cursor);

  // RequestN frame has no flags, but we need to skip over the two bytes.
  FrameType type;
  Flags flags;
  std::tie(type, flags) = readFrameTypeAndFlags(cursor);
  DCHECK(frameType() == type);
  DCHECK(Flags::none() == flags);

  requestN_ = cursor.readBE<int32_t>();
}

RequestNFrame::RequestNFrame(
    StreamId streamId,
    Flags flags,
    folly::io::Cursor& cursor)
    : streamId_(streamId) {
  DCHECK(Flags::none() == flags);
  requestN_ = cursor.readBE<int32_t>();
}

CancelFrame::CancelFrame(std::unique_ptr<folly::IOBuf> frame) {
  folly::io::Cursor cursor(frame.get());
  DCHECK(!frame->isChained());

  streamId_ = readStreamId(cursor);

  // Cancel frame has no flags.
  FrameType type;
  Flags flags;
  std::tie(type, flags) = readFrameTypeAndFlags(cursor);
  DCHECK(frameType() == type);
  DCHECK(Flags::none() == flags);
}

PayloadFrame::PayloadFrame(std::unique_ptr<folly::IOBuf> frame) {
  folly::io::Cursor cursor(frame.get());
  DCHECK(!frame->isChained());

  streamId_ = readStreamId(cursor);

  FrameType type;
  std::tie(type, flags_) = readFrameTypeAndFlags(cursor);
  DCHECK(frameType() == type);

  payload_ = readPayload(flags_.metadata(), cursor, std::move(frame));
}

PayloadFrame::PayloadFrame(
    StreamId streamId,
    Flags flags,
    folly::io::Cursor& cursor,
    std::unique_ptr<folly::IOBuf> underlyingBuffer)
    : streamId_(streamId),
      flags_(flags),
      payload_(
          readPayload(flags_.metadata(), cursor, std::move(underlyingBuffer))) {
}

ErrorFrame::ErrorFrame(std::unique_ptr<folly::IOBuf> frame) {
  folly::io::Cursor cursor(frame.get());
  DCHECK(!frame->isChained());
  DCHECK_GE(frame->length(), frameHeaderSize());

  streamId_ = readStreamId(cursor);

  // Error frame has no flags, but we still need to skip the two bytes.
  FrameType type;
  Flags flags;
  std::tie(type, flags) = readFrameTypeAndFlags(cursor);
  DCHECK(frameType() == type);
  DCHECK(Flags::none() == flags);

  errorCode_ = static_cast<ErrorCode>(cursor.readBE<uint32_t>());

  // Finally, adjust the data portion of frame.
  frame->trimStart(frameHeaderSize());
  payload_ = Payload::makeFromData(std::move(frame));
}

MetadataPushFrame::MetadataPushFrame(std::unique_ptr<folly::IOBuf> frame) {
  DCHECK(!frame->isChained());
  DCHECK_GE(frame->length(), frameHeaderSize());

  folly::io::Cursor cursor(frame.get());
  const StreamId zero(readStreamId(cursor));
  DCHECK_EQ(StreamId{0}, zero);

  // METADATA_PUSH frame has only metadata flag.
  FrameType type;
  Flags flags;
  std::tie(type, flags) = readFrameTypeAndFlags(cursor);
  DCHECK(frameType() == type);
  DCHECK(flags.metadata());

  frame->trimStart(frameHeaderSize());
  metadata_ = std::move(frame);
}

KeepAliveFrame::KeepAliveFrame(std::unique_ptr<folly::IOBuf> frame) {
  DCHECK(!frame->isChained());
  DCHECK_GE(frame->length(), frameHeaderSize());

  folly::io::Cursor cursor(frame.get());
  const StreamId zero(readStreamId(cursor));
  DCHECK_EQ(StreamId{0}, zero);

  FrameType type;
  std::tie(type, flags_) = readFrameTypeAndFlags(cursor);
  DCHECK(frameType() == type);

  cursor.skip(sizeof(uint64_t)); // Skip 'last received position'

  frame->trimStart(cursor.getCurrentPosition());
  data_ = std::move(frame);
}

// Static member definition
constexpr folly::StringPiece SetupFrame::kMimeType;

} // namespace rocket
} // namespace thrift
} // namespace apache
