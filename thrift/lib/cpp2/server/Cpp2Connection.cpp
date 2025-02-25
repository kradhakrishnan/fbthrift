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

#include <thrift/lib/cpp2/server/Cpp2Connection.h>

#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp2/GeneratedCodeHelper.h>
#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/server/Cpp2Worker.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/server/admission_strategy/AdmissionStrategy.h>

namespace apache {
namespace thrift {

using namespace apache::thrift::protocol;
using namespace apache::thrift::server;
using namespace apache::thrift::transport;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::async;
using namespace std;
using apache::thrift::TApplicationException;

Cpp2Connection::Cpp2Connection(
    const std::shared_ptr<TAsyncTransport>& transport,
    const folly::SocketAddress* address,
    std::shared_ptr<Cpp2Worker> worker,
    const std::shared_ptr<HeaderServerChannel>& serverChannel)
    : processor_(worker->getServer()->getCpp2Processor()),
      duplexChannel_(
          worker->getServer()->isDuplex() ? std::make_unique<DuplexChannel>(
                                                DuplexChannel::Who::SERVER,
                                                transport)
                                          : nullptr),
      channel_(
          serverChannel ? serverChannel : // used by client
              duplexChannel_ ? duplexChannel_->getServerChannel() : // server
                  std::shared_ptr<HeaderServerChannel>(
                      new HeaderServerChannel(transport),
                      folly::DelayedDestruction::Destructor())),
      worker_(std::move(worker)),
      context_(
          address,
          transport.get(),
          worker_->getServer()->getEventBaseManager(),
          duplexChannel_ ? duplexChannel_->getClientChannel() : nullptr,
          nullptr,
          worker_->getServer()->getClientIdentityHook()),
      transport_(transport),
      threadManager_(worker_->getServer()->getThreadManager()) {
  channel_->setQueueSends(worker_->getServer()->getQueueSends());
  channel_->setMinCompressBytes(worker_->getServer()->getMinCompressBytes());
  channel_->setDefaultWriteTransforms(
      worker_->getServer()->getDefaultWriteTransforms());

  if (auto* observer = worker_->getServer()->getObserver()) {
    channel_->setSampleRate(observer->getSampleRate());
  }

  auto handler = worker_->getServer()->getEventHandlerUnsafe();
  if (handler) {
    handler->newConnection(&context_);
  }
}

Cpp2Connection::~Cpp2Connection() {
  auto handler = worker_->getServer()->getEventHandlerUnsafe();
  if (handler) {
    handler->connectionDestroyed(&context_);
  }

  channel_.reset();
}

void Cpp2Connection::stop() {
  if (getConnectionManager()) {
    getConnectionManager()->removeConnection(this);
  }

  for (auto req : activeRequests_) {
    VLOG(1) << "Task killed due to channel close: "
            << context_.getPeerAddress()->describe();
    req->cancelRequest();
    if (auto* observer = worker_->getServer()->getObserver()) {
      observer->taskKilled();
    }
  }

  if (channel_) {
    channel_->setCallback(nullptr);

    // Release the socket to avoid long CLOSE_WAIT times
    channel_->closeNow();
  }

  transport_.reset();

  this_.reset();
}

void Cpp2Connection::timeoutExpired() noexcept {
  // Only disconnect if there are no active requests. No need to set another
  // timeout here because it's going to be set when all the requests are
  // handled.
  if (activeRequests_.empty()) {
    disconnect("idle timeout");
  }
}

void Cpp2Connection::disconnect(const char* comment) noexcept {
  // This must be the last call, it may delete this.
  auto guard = folly::makeGuard([&] { stop(); });

  VLOG(1) << "ERROR: Disconnect: " << comment
          << " on channel: " << context_.getPeerAddress()->describe();
  if (auto* observer = worker_->getServer()->getObserver()) {
    observer->connDropped();
  }
}

void Cpp2Connection::setServerHeaders(
    HeaderServerChannel::HeaderRequest& request) {
  if (getWorker()->stopping_) {
    request.getHeader()->setHeader("connection", "goaway");
  }

  const auto& headers = request.getHeader()->getHeaders();
  std::string loadHeaderFound;
  auto it = headers.find(THeader::QUERY_LOAD_HEADER);
  if (it != headers.end()) {
    loadHeaderFound = it->second;
  } else {
    return;
  }

  auto load = getWorker()->getServer()->getLoad(loadHeaderFound);

  request.getHeader()->setHeader(
      THeader::QUERY_LOAD_HEADER, folly::to<std::string>(load));
}

void Cpp2Connection::requestTimeoutExpired() {
  VLOG(1) << "ERROR: Task expired on channel: "
          << context_.getPeerAddress()->describe();
  if (auto* observer = worker_->getServer()->getObserver()) {
    observer->taskTimeout();
  }
}

void Cpp2Connection::queueTimeoutExpired() {
  VLOG(1) << "ERROR: Queue timeout on channel: "
          << context_.getPeerAddress()->describe();
  if (auto* observer = worker_->getServer()->getObserver()) {
    observer->queueTimeout();
  }
}

bool Cpp2Connection::pending() {
  return transport_ ? transport_->isPending() : false;
}

void Cpp2Connection::killRequest(
    ResponseChannelRequest& req,
    TApplicationException::TApplicationExceptionType reason,
    const std::string& errorCode,
    const char* comment) {
  VLOG(1) << "ERROR: Task killed: " << comment << ": "
          << context_.getPeerAddress()->getAddressStr();

  auto server = worker_->getServer();
  if (auto* observer = server->getObserver()) {
    if (reason ==
        TApplicationException::TApplicationExceptionType::LOADSHEDDING) {
      observer->serverOverloaded();
    } else {
      observer->taskKilled();
    }
  }

  // Nothing to do for Thrift oneway request.
  if (req.isOneway()) {
    return;
  }

  auto header_req = static_cast<HeaderServerChannel::HeaderRequest*>(&req);
  setServerHeaders(*header_req);

  // Thrift1 oneway request doesn't use ONEWAY_REQUEST_ID and
  // may end up here. No need to send error back for such requests
  if (!processor_->isOnewayMethod(req.getBuf(), header_req->getHeader())) {
    header_req->sendErrorWrapped(
        folly::make_exception_wrapper<TApplicationException>(reason, comment),
        errorCode,
        nullptr);
  } else {
    // Send an empty response so reqId will be handled properly
    req.sendReply(std::unique_ptr<folly::IOBuf>());
  }
}

// Response Channel callbacks
void Cpp2Connection::requestReceived(unique_ptr<ResponseChannelRequest>&& req) {
  auto baseReqCtx = processor_->getBaseContextForRequest();
  auto reqCtx = baseReqCtx
      ? std::make_shared<folly::RequestContext>(*baseReqCtx)
      : std::make_shared<folly::RequestContext>();
  auto handler = worker_->getServer()->getEventHandlerUnsafe();
  if (handler) {
    handler->connectionNewRequest(&context_, reqCtx.get());
  }
  folly::RequestContextScopeGuard rctx(reqCtx);

  auto server = worker_->getServer();
  auto* observer = server->getObserver();

  server->touchRequestTimestamp();

  auto injectedFailure = server->maybeInjectFailure();
  switch (injectedFailure) {
    case ThriftServer::InjectedFailure::NONE:
      break;
    case ThriftServer::InjectedFailure::ERROR:
      killRequest(
          *req,
          TApplicationException::TApplicationExceptionType::INJECTED_FAILURE,
          kInjectedFailureErrorCode,
          "injected failure");
      return;
    case ThriftServer::InjectedFailure::DROP:
      VLOG(1) << "ERROR: injected drop: "
              << context_.getPeerAddress()->getAddressStr();
      return;
    case ThriftServer::InjectedFailure::DISCONNECT:
      disconnect("injected failure");
      return;
  }

  auto* hreq = static_cast<HeaderServerChannel::HeaderRequest*>(req.get());
  bool useHttpHandler = false;
  // Any POST not for / should go to the status handler
  if (hreq->getHeader()->getClientType() == THRIFT_HTTP_SERVER_TYPE) {
    auto buf = req->getBuf();
    // 7 == length of "POST / " - we are matching on the path
    if (buf->length() >= 7 &&
        0 == strncmp(reinterpret_cast<const char*>(buf->data()), "POST", 4) &&
        buf->data()[6] != ' ') {
      useHttpHandler = true;
    }

    // Any GET should use the handler
    if (buf->length() >= 3 &&
        0 == strncmp(reinterpret_cast<const char*>(buf->data()), "GET", 3)) {
      useHttpHandler = true;
    }

    // Any HEAD should use the handler
    if (buf->length() >= 4 &&
        0 == strncmp(reinterpret_cast<const char*>(buf->data()), "HEAD", 4)) {
      useHttpHandler = true;
    }
  }

  if (useHttpHandler && worker_->getServer()->getGetHandler()) {
    worker_->getServer()->getGetHandler()(
        worker_->getEventBase(), transport_, req->extractBuf());

    // Close the channel, since the handler now owns the socket.
    channel_->setCallback(nullptr);
    channel_->setTransport(nullptr);
    stop();
    return;
  }

  if (worker_->getServer()->getGetHeaderHandler()) {
    worker_->getServer()->getGetHeaderHandler()(
        hreq->getHeader(), context_.getPeerAddress());
  }

  auto protoId = static_cast<apache::thrift::protocol::PROTOCOL_TYPES>(
      hreq->getHeader()->getProtocolId());
  auto methodName =
      apache::thrift::detail::ap::deserializeMethodName(req, protoId);
  if (server->isOverloaded(&hreq->getHeader()->getHeaders(), &methodName)) {
    killRequest(
        *req,
        TApplicationException::TApplicationExceptionType::LOADSHEDDING,
        server->getOverloadedErrorCode(),
        "loadshedding request");
    return;
  }

  auto admissionStrategy = worker_->getServer()->getAdmissionStrategy();
  auto admissionController =
      admissionStrategy->select(methodName, hreq->getHeader());
  if (!admissionController->admit()) {
    killRequest(
        *req,
        TApplicationException::TApplicationExceptionType::LOADSHEDDING,
        server->getOverloadedErrorCode(),
        "adaptive loadshedding rejection");
    return;
  }

  if (worker_->stopping_) {
    killRequest(
        *req,
        TApplicationException::TApplicationExceptionType::INTERNAL_ERROR,
        kQueueOverloadedErrorCode,
        "server shutting down");
    return;
  }

  server->incActiveRequests();
  auto samplingStatus = req->timestamps_.getSamplingStatus();
  if (samplingStatus.isEnabled()) {
    // Expensive operations; happens only when sampling is enabled
    req->timestamps_.processBegin =
        apache::thrift::concurrency::Util::currentTimeUsec();
    if (samplingStatus.isEnabledByServer() && observer) {
      observer->queuedRequests(threadManager_->pendingTaskCount());
      observer->activeRequests(server->getActiveRequests());
    }
  }

  // After this, the request buffer is no longer owned by the request
  // and will be released after deserializeRequest.
  unique_ptr<folly::IOBuf> buf = hreq->extractBuf();

  Cpp2Request* t2r = new Cpp2Request(std::move(req), this_);
  if (admissionController) {
    t2r->setAdmissionController(std::move(admissionController));
  }
  auto up2r = std::unique_ptr<ResponseChannelRequest>(t2r);
  activeRequests_.insert(t2r);
  ++worker_->activeRequests_;

  if (observer) {
    observer->receivedRequest();
  }

  std::chrono::milliseconds queueTimeout;
  std::chrono::milliseconds taskTimeout;
  auto differentTimeouts = server->getTaskExpireTimeForRequest(
      *(hreq->getHeader()), queueTimeout, taskTimeout);
  if (differentTimeouts) {
    if (queueTimeout > std::chrono::milliseconds(0)) {
      scheduleTimeout(&t2r->queueTimeout_, queueTimeout);
    }
  }
  if (taskTimeout > std::chrono::milliseconds(0)) {
    scheduleTimeout(&t2r->taskTimeout_, taskTimeout);
  }

  auto reqContext = t2r->getContext();
  reqContext->setRequestTimeout(taskTimeout);

  try {
    if (!apache::thrift::detail::ap::deserializeMessageBegin(
            protoId, up2r, buf.get(), reqContext, worker_->getEventBase())) {
      return;
    }

    processor_->process(
        std::move(up2r),
        std::move(buf),
        protoId,
        reqContext,
        worker_->getEventBase(),
        threadManager_.get());
  } catch (...) {
    LOG(WARNING) << "Process exception: "
                 << folly::exceptionStr(std::current_exception());
    throw;
  }
}

void Cpp2Connection::channelClosed(folly::exception_wrapper&& ex) {
  // This must be the last call, it may delete this.
  auto guard = folly::makeGuard([&] { stop(); });

  VLOG(4) << "Channel " << context_.getPeerAddress()->describe()
          << " closed: " << ex.what();
}

void Cpp2Connection::removeRequest(Cpp2Request* req) {
  activeRequests_.erase(req);
  if (activeRequests_.empty()) {
    resetTimeout();
  }
}

Cpp2Connection::Cpp2Request::Cpp2Request(
    std::unique_ptr<ResponseChannelRequest> req,
    std::shared_ptr<Cpp2Connection> con)
    : req_(static_cast<HeaderServerChannel::HeaderRequest*>(req.release())),
      connection_(std::move(con)),
      // Note: tricky ordering here; see the note on connection_ in the class
      // definition.
      reqContext_(&connection_->context_, req_->getHeader()),
      debugStub_(
          *connection_->getWorker()->getRequestsRegistry(),
          *this,
          reqContext_) {
  queueTimeout_.request_ = this;
  taskTimeout_.request_ = this;
}

MessageChannel::SendCallback* Cpp2Connection::Cpp2Request::prepareSendCallback(
    MessageChannel::SendCallback* sendCallback,
    apache::thrift::server::TServerObserver* observer) {
  // If we are sampling this call, wrap it with a Cpp2Sample, which also
  // implements MessageChannel::SendCallback. Callers of sendReply/sendError
  // are responsible for cleaning up their own callbacks.
  MessageChannel::SendCallback* cb = sendCallback;
  if (req_->timestamps_.getSamplingStatus().isEnabledByServer()) {
    // Cpp2Sample will delete itself when it's callback is called.
    cb = new Cpp2Sample(std::move(req_->timestamps_), observer, sendCallback);
  }
  return cb;
}

void Cpp2Connection::Cpp2Request::setServerHeaders() {
  connection_->setServerHeaders(*req_);
}

void Cpp2Connection::Cpp2Request::sendReply(
    std::unique_ptr<folly::IOBuf>&& buf,
    MessageChannel::SendCallback* sendCallback,
    folly::Optional<uint32_t>) {
  if (req_->isActive()) {
    setServerHeaders();
    markProcessEnd();
    auto* observer = connection_->getWorker()->getServer()->getObserver();
    auto maxResponseSize =
        connection_->getWorker()->getServer()->getMaxResponseSize();
    if (maxResponseSize != 0 &&
        buf->computeChainDataLength() > maxResponseSize) {
      req_->sendErrorWrapped(
          folly::make_exception_wrapper<TApplicationException>(
              TApplicationException::TApplicationExceptionType::INTERNAL_ERROR,
              "Response size too big"),
          kResponseTooBigErrorCode,
          reqContext_.getMethodName(),
          reqContext_.getProtoSeqId(),
          prepareSendCallback(sendCallback, observer));
    } else {
      req_->sendReply(
          std::move(buf), prepareSendCallback(sendCallback, observer));
    }
    cancelTimeout();
    if (observer) {
      observer->sentReply();
    }
  }
}

void Cpp2Connection::Cpp2Request::sendErrorWrapped(
    folly::exception_wrapper ew,
    std::string exCode,
    MessageChannel::SendCallback* sendCallback) {
  if (req_->isActive()) {
    setServerHeaders();
    markProcessEnd();
    auto* observer = connection_->getWorker()->getServer()->getObserver();
    req_->sendErrorWrapped(
        std::move(ew),
        std::move(exCode),
        reqContext_.getMethodName(),
        reqContext_.getProtoSeqId(),
        prepareSendCallback(sendCallback, observer));
    cancelTimeout();
  }
}

void Cpp2Connection::Cpp2Request::sendTimeoutResponse(
    HeaderServerChannel::HeaderRequest::TimeoutResponseType responseType) {
  auto* observer = connection_->getWorker()->getServer()->getObserver();
  std::map<std::string, std::string> headers;
  setServerHeaders();
  markProcessEnd(&headers);
  req_->sendTimeoutResponse(
      reqContext_.getMethodName(),
      reqContext_.getProtoSeqId(),
      prepareSendCallback(nullptr, observer),
      headers,
      responseType);
  cancelTimeout();
}

void Cpp2Connection::Cpp2Request::TaskTimeout::timeoutExpired() noexcept {
  request_->req_->cancel();
  request_->sendTimeoutResponse(
      HeaderServerChannel::HeaderRequest::TimeoutResponseType::TASK);
  request_->connection_->requestTimeoutExpired();
}

void Cpp2Connection::Cpp2Request::QueueTimeout::timeoutExpired() noexcept {
  if (!request_->reqContext_.getStartedProcessing()) {
    request_->req_->cancel();
    request_->sendTimeoutResponse(
        HeaderServerChannel::HeaderRequest::TimeoutResponseType::QUEUE);
    request_->connection_->queueTimeoutExpired();
  }
}

void Cpp2Connection::Cpp2Request::markProcessEnd(
    std::map<std::string, std::string>* newHeaders) {
  auto samplingStatus = req_->timestamps_.getSamplingStatus();
  if (samplingStatus.isEnabled()) {
    req_->timestamps_.processEnd =
        apache::thrift::concurrency::Util::currentTimeUsec();
    if (samplingStatus.isEnabledByClient()) {
      // Latency headers are set after processEnd itself. Can't be
      // done after write, since headers transform happens during write.
      setLatencyHeaders(req_->getTimestamps(), newHeaders);
    }
  }
}

void Cpp2Connection::Cpp2Request::setLatencyHeaders(
    const apache::thrift::server::TServerObserver::CallTimestamps& timestamps,
    std::map<std::string, std::string>* newHeaders) const {
  setLatencyHeader(
      kReadLatencyHeader.str(),
      folly::to<std::string>(timestamps.readEnd - timestamps.readBegin),
      newHeaders);
  setLatencyHeader(
      kQueueLatencyHeader.str(),
      folly::to<std::string>(timestamps.processBegin - timestamps.readEnd),
      newHeaders);
  setLatencyHeader(
      kProcessLatencyHeader.str(),
      folly::to<std::string>(timestamps.processEnd - timestamps.processBegin),
      newHeaders);
}

void Cpp2Connection::Cpp2Request::setLatencyHeader(
    const std::string& key,
    const std::string& value,
    std::map<std::string, std::string>* newHeaders) const {
  // newHeaders is used timeout exceptions, where req->header cannot be mutated.
  if (newHeaders) {
    (*newHeaders)[key] = value;
  } else {
    req_->getHeader()->setHeader(key, value);
  }
}

Cpp2Connection::Cpp2Request::~Cpp2Request() {
  connection_->removeRequest(this);
  cancelTimeout();
  if (--connection_->getWorker()->activeRequests_ == 0 &&
      connection_->getWorker()->stopping_) {
    connection_->getWorker()->stopBaton_.post();
  }
  connection_->getWorker()->getServer()->decActiveRequests();
}

// Cancel request is usually called from a different thread than sendReply.
void Cpp2Connection::Cpp2Request::cancelRequest() {
  cancelTimeout();
  req_->cancel();
}

Cpp2Connection::Cpp2Sample::Cpp2Sample(
    apache::thrift::server::TServerObserver::CallTimestamps&& timestamps,
    apache::thrift::server::TServerObserver* observer,
    MessageChannel::SendCallback* chainedCallback)
    : timestamps_(timestamps),
      observer_(observer),
      chainedCallback_(chainedCallback) {
  DCHECK(observer != nullptr);
}

void Cpp2Connection::Cpp2Sample::sendQueued() {
  if (chainedCallback_ != nullptr) {
    chainedCallback_->sendQueued();
  }
  timestamps_.writeBegin = apache::thrift::concurrency::Util::currentTimeUsec();
}

void Cpp2Connection::Cpp2Sample::messageSent() {
  if (chainedCallback_ != nullptr) {
    chainedCallback_->messageSent();
  }
  timestamps_.writeEnd = apache::thrift::concurrency::Util::currentTimeUsec();
  delete this;
}

void Cpp2Connection::Cpp2Sample::messageSendError(
    folly::exception_wrapper&& e) {
  if (chainedCallback_ != nullptr) {
    chainedCallback_->messageSendError(std::move(e));
  }
  timestamps_.writeEnd = apache::thrift::concurrency::Util::currentTimeUsec();
  delete this;
}

Cpp2Connection::Cpp2Sample::~Cpp2Sample() {
  if (observer_) {
    observer_->callCompleted(timestamps_);
  }
}

} // namespace thrift
} // namespace apache
