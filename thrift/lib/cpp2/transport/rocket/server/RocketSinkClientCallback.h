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

#pragma once

#include <chrono>
#include <memory>

#include <folly/ExceptionWrapper.h>
#include <folly/io/async/HHWheelTimer.h>

#include <thrift/lib/cpp2/async/StreamCallbacks.h>
#include <thrift/lib/cpp2/transport/rocket/Types.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketServerFrameContext.h>

namespace apache {
namespace thrift {

class RocketSinkClientCallback final : public SinkClientCallback {
 public:
  explicit RocketSinkClientCallback(rocket::RocketServerFrameContext&& context);
  ~RocketSinkClientCallback() override = default;
  void onFirstResponse(
      FirstResponsePayload&&,
      folly::EventBase*,
      SinkServerCallback*) override;
  void onFirstResponseError(folly::exception_wrapper) override;

  void onFinalResponse(StreamPayload&&) override;
  void onFinalResponseError(folly::exception_wrapper) override;

  void onSinkRequestN(uint64_t) override;

  bool onSinkNext(StreamPayload&&);
  bool onSinkError(folly::exception_wrapper);
  bool onSinkComplete();

  void onStreamCancel();
  void setChunkTimeout(std::chrono::milliseconds timeout);
  void timeoutExpired() noexcept;

 private:
  class TimeoutCallback : public folly::HHWheelTimer::Callback {
   public:
    explicit TimeoutCallback(
        RocketSinkClientCallback& parent,
        std::chrono::milliseconds chunkTimeout)
        : parent_(parent), chunkTimeout_(chunkTimeout) {
      DCHECK(chunkTimeout != std::chrono::milliseconds::zero());
    }
    void timeoutExpired() noexcept override {
      parent_.timeoutExpired();
    }
    void incCredits(uint64_t n);
    void decCredits();

   private:
    RocketSinkClientCallback& parent_;
    std::chrono::milliseconds chunkTimeout_;
    uint64_t credits_{0};
  };

  void scheduleTimeout(std::chrono::milliseconds chunkTimeout);
  void cancelTimeout();

  enum class State { BothOpen, StreamOpen };
  State state_{State::BothOpen};
  rocket::RocketServerFrameContext context_;
  SinkServerCallback* serverCallback_{nullptr};
  std::unique_ptr<TimeoutCallback> timeout_;
};

} // namespace thrift
} // namespace apache
