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

#include <thrift/lib/cpp2/async/AsyncProcessor.h>

namespace apache {
namespace thrift {

constexpr std::chrono::seconds ServerInterface::BlockingThreadManager::kTimeout;
thread_local RequestParams ServerInterface::requestParams_;

void HandlerCallbackBase::sendReply(
    folly::IOBufQueue queue,
    apache::thrift::Stream<folly::IOBufQueue>&& stream) {
  folly::Optional<uint32_t> crc32c = checksumIfNeeded(queue);
  transform(queue);
  auto stream_ =
      std::move(stream).map([](auto&& value) mutable { return value.move(); });
  if (getEventBase()->isInEventBaseThread()) {
    req_->sendStreamReply({queue.move(), std::move(stream_)}, nullptr, crc32c);
  } else {
    getEventBase()->runInEventBaseThread([req = std::move(req_),
                                          queue = std::move(queue),
                                          stream = std::move(stream_),
                                          crc32c]() mutable {
      req->sendStreamReply({queue.move(), std::move(stream)}, nullptr, crc32c);
    });
  }
}

#ifdef FOLLY_HAS_COROUTINES
void HandlerCallbackBase::sendReply(
    folly::IOBufQueue queue,
    apache::thrift::detail::SinkConsumerImpl&& sinkConsumer) {
  folly::Optional<uint32_t> crc32c = checksumIfNeeded(queue);
  transform(queue);

  if (getEventBase()->isInEventBaseThread()) {
    req_->sendSinkReply(queue.move(), std::move(sinkConsumer), crc32c);
  } else {
    getEventBase()->runInEventBaseThread(
        [req = std::move(req_),
         queue = std::move(queue),
         sinkConsumer = std::move(sinkConsumer),
         crc32c]() mutable {
          req->sendSinkReply(queue.move(), std::move(sinkConsumer), crc32c);
        });
  }
}
#endif
} // namespace thrift
} // namespace apache
