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

#include <thrift/lib/cpp2/transport/rocket/server/RocketSinkClientCallback.h>

#include <functional>
#include <memory>
#include <utility>

#include <glog/logging.h>

#include <folly/ExceptionWrapper.h>
#include <folly/Range.h>
#include <folly/ScopeGuard.h>
#include <folly/io/IOBufQueue.h>
#include <folly/io/async/EventBase.h>

#include <thrift/lib/cpp/TApplicationException.h>
#include <thrift/lib/cpp2/async/Stream.h>
#include <thrift/lib/cpp2/async/StreamCallbacks.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/transport/rocket/PayloadUtils.h>
#include <thrift/lib/cpp2/transport/rocket/RocketException.h>
#include <thrift/lib/cpp2/transport/rocket/Types.h>
#include <thrift/lib/cpp2/transport/rocket/framing/ErrorCode.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Flags.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketServerFrameContext.h>

namespace apache {
namespace thrift {

// RocketSinkClientCallback methods
RocketSinkClientCallback::RocketSinkClientCallback(
    rocket::RocketServerFrameContext&& context)
    : context_(std::move(context)) {}

void RocketSinkClientCallback::onFirstResponse(
    FirstResponsePayload&& firstResponse,
    folly::EventBase* /* unused */,
    SinkServerCallback* serverCallback) {
  serverCallback_ = serverCallback;
  context_.sendPayload(
      rocket::pack(std::move(firstResponse)).value(),
      rocket::Flags::none().next(true));
  context_.takeOwnership(this);
}

void RocketSinkClientCallback::onFirstResponseError(
    folly::exception_wrapper ew) {
  SCOPE_EXIT {
    delete this;
  };

  ew.with_exception<thrift::detail::EncodedError>([&](auto&& encodedError) {
    context_.sendPayload(
        rocket::Payload::makeFromData(std::move(encodedError.encoded)),
        rocket::Flags::none().next(true).complete(true));
  });
}

void RocketSinkClientCallback::onFinalResponse(StreamPayload&& firstResponse) {
  DCHECK(state_ == State::BothOpen || state_ == State::StreamOpen);
  context_.sendPayload(
      rocket::pack(std::move(firstResponse)).value(),
      rocket::Flags::none().next(true).complete(true));
  context_.freeStream();
}

void RocketSinkClientCallback::onFinalResponseError(
    folly::exception_wrapper ew) {
  DCHECK(state_ == State::BothOpen || state_ == State::StreamOpen);
  if (!ew.with_exception<rocket::RocketException>([this](auto&& rex) {
        context_.sendError(rocket::RocketException(
            rocket::ErrorCode::APPLICATION_ERROR, rex.moveErrorData()));
      })) {
    context_.sendError(rocket::RocketException(
        rocket::ErrorCode::APPLICATION_ERROR, ew.what()));
  }
  context_.freeStream();
}

void RocketSinkClientCallback::onSinkRequestN(uint64_t n) {
  if (timeout_) {
    timeout_->incCredits(n);
  }
  DCHECK(state_ == State::BothOpen);
  context_.sendRequestN(n);
}

bool RocketSinkClientCallback::onSinkNext(StreamPayload&& payload) {
  if (state_ != State::BothOpen) {
    cancelTimeout();
    return false;
  }

  if (timeout_) {
    timeout_->decCredits();
  }

  serverCallback_->onSinkNext(std::move(payload));
  return true;
}

bool RocketSinkClientCallback::onSinkError(folly::exception_wrapper ew) {
  cancelTimeout();
  if (state_ != State::BothOpen) {
    return false;
  }
  serverCallback_->onSinkError(std::move(ew));
  return true;
}

bool RocketSinkClientCallback::onSinkComplete() {
  cancelTimeout();
  if (state_ != State::BothOpen) {
    return false;
  }
  state_ = State::StreamOpen;
  serverCallback_->onSinkComplete();
  return true;
}

void RocketSinkClientCallback::onStreamCancel() {
  serverCallback_->onStreamCancel();
}

void RocketSinkClientCallback::setChunkTimeout(
    std::chrono::milliseconds timeout) {
  if (timeout != std::chrono::milliseconds::zero()) {
    timeout_ = std::make_unique<TimeoutCallback>(*this, timeout);
  }
}

void RocketSinkClientCallback::timeoutExpired() noexcept {
  auto ew = folly::make_exception_wrapper<TApplicationException>(
      TApplicationException::TApplicationExceptionType::TIMEOUT);
  onSinkError(ew);
  onFinalResponseError(std::move(ew));
}

void RocketSinkClientCallback::scheduleTimeout(
    std::chrono::milliseconds chunkTimeout) {
  if (timeout_) {
    context_.scheduleSinkTimeout(timeout_.get(), chunkTimeout);
  }
}

void RocketSinkClientCallback::cancelTimeout() {
  if (timeout_) {
    timeout_->cancelTimeout();
  }
}

void RocketSinkClientCallback::TimeoutCallback::incCredits(uint64_t n) {
  if (credits_ == 0) {
    parent_.scheduleTimeout(chunkTimeout_);
  }
  credits_ += n;
}

void RocketSinkClientCallback::TimeoutCallback::decCredits() {
  DCHECK(credits_ != 0);
  if (--credits_ != 0) {
    parent_.scheduleTimeout(chunkTimeout_);
  } else {
    parent_.cancelTimeout();
  }
}

} // namespace thrift
} // namespace apache
