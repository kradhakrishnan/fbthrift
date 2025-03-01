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

#include <folly/Portability.h>
#include <folly/fibers/Baton.h>
#if FOLLY_HAS_COROUTINES
#include <folly/experimental/coro/AsyncGenerator.h>
#include <folly/experimental/coro/Baton.h>
#include <folly/experimental/coro/Task.h>
#endif // FOLLY_HAS_COROUTINES
#include <folly/Try.h>
#include <thrift/lib/cpp2/async/ServerStreamDetail.h>
#include <thrift/lib/cpp2/async/TwoWayBridge.h>

namespace apache {
namespace thrift {
namespace detail {

class ServerStreamConsumer {
 public:
  virtual ~ServerStreamConsumer() = default;
  virtual void consume() = 0;
  virtual void canceled() = 0;
};

class ServerGeneratorStream : public TwoWayBridge<
                                  ServerGeneratorStream,
                                  folly::Try<StreamPayload>,
                                  ServerStreamConsumer,
                                  int64_t,
                                  ServerGeneratorStream>,
                              private StreamServerCallback {
 public:
#if FOLLY_HAS_COROUTINES
  template <typename T>
  static ServerStreamFactoryFn<T> fromAsyncGenerator(
      folly::coro::AsyncGenerator<T&&>&& gen) {
    return [gen = std::move(gen)](
               FirstResponsePayload&& payload,
               StreamClientCallback* callback,
               folly::EventBase* clientEb,
               folly::Executor::KeepAlive<> serverExecutor,
               folly::Try<StreamPayload> (*encode)(folly::Try<T> &&)) mutable {
      auto stream = new ServerGeneratorStream(callback, clientEb);
      auto streamPtr = stream->copy();
      clientEb->add([=, payload = std::move(payload)]() mutable {
        callback->onFirstResponse(std::move(payload), clientEb, stream);
        stream->processPayloads();
      });
      folly::coro::co_invoke(
          [stream = std::move(streamPtr),
           encode,
           gen = std::move(gen)]() mutable -> folly::coro::Task<void> {
            int64_t credits = 0;
            class ReadyCallback
                : public apache::thrift::detail::ServerStreamConsumer {
             public:
              void consume() override {
                baton.post();
              }

              void canceled() override {
                std::terminate();
              }

              folly::coro::Baton baton;
            };
            SCOPE_EXIT {
              stream->serverClose();
            };

            while (true) {
              if (credits == 0) {
                ReadyCallback callback;
                if (stream->wait(&callback)) {
                  co_await callback.baton;
                }
              }

              {
                auto queue = stream->getMessages();
                while (!queue.empty()) {
                  auto next = queue.front();
                  queue.pop();
                  if (next == -1) {
                    co_return;
                  }
                  credits += next;
                }
              }

              try {
                auto&& payload = co_await folly::coro::co_withCancellation(
                    stream->cancelSource_.getToken(), gen.next());
                if (payload) {
                  stream->publish(encode(folly::Try<T>(std::move(*payload))));
                  --credits;
                  continue;
                }
                stream->publish({});
              } catch (const std::exception& e) {
                stream->publish(folly::Try<StreamPayload>(
                    folly::exception_wrapper(std::current_exception(), e)));
              } catch (...) {
                stream->publish(folly::Try<StreamPayload>(
                    folly::exception_wrapper(std::current_exception())));
              }
              co_return;
            }
          })
          .scheduleOn(std::move(serverExecutor))
          .start([](folly::Try<folly::Unit> t) {
            if (t.hasException()) {
              LOG(FATAL) << t.exception().what();
            }
          });
    };
  }
#endif // FOLLY_HAS_COROUTINES

  void consume() {
    clientEventBase_->add([this]() { processPayloads(); });
  }
  void canceled() {
    Ptr(this);
  }

 private:
  ServerGeneratorStream(
      StreamClientCallback* clientCallback,
      folly::EventBase* clientEb)
      : streamClientCallback_(clientCallback), clientEventBase_(clientEb) {}

  bool wait(ServerStreamConsumer* consumer) {
    return serverWait(consumer);
  }

  void publish(folly::Try<StreamPayload>&& payload) {
    serverPush(std::move(payload));
  }

  ServerQueue getMessages() {
    return serverGetMessages();
  }

  void onStreamRequestN(uint64_t credits) {
    clientPush(std::move(credits));
  }

  void onStreamCancel() override {
    cancelSource_.requestCancellation();
    clientPush(-1);
    clientClose();
    streamClientCallback_ = nullptr;
  }

  void resetClientCallback(StreamClientCallback& clientCallback) override {
    streamClientCallback_ = &clientCallback;
  }

  void processPayloads() {
    clientEventBase_->dcheckIsInEventBaseThread();
    while (!clientWait(this)) {
      for (auto messages = clientGetMessages();
           !messages.empty() && streamClientCallback_;
           messages.pop()) {
        auto& payload = messages.front();
        if (payload.hasValue()) {
          streamClientCallback_->onStreamNext(std::move(payload.value()));
        } else if (payload.hasException()) {
          streamClientCallback_->onStreamError(std::move(payload.exception()));
          Ptr(this);
          return;
        } else {
          streamClientCallback_->onStreamComplete();
          Ptr(this);
          return;
        }
      }
    }
  }

  StreamClientCallback* streamClientCallback_;
  folly::EventBase* clientEventBase_;
  folly::CancellationSource cancelSource_;
};

} // namespace detail
} // namespace thrift
} // namespace apache
