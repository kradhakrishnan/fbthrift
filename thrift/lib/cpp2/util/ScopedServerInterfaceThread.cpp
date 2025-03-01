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

#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>

#include <folly/SocketAddress.h>

#include <thrift/lib/cpp2/server/BaseThriftServer.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/transport/rsocket/server/RSRoutingHandler.h>

using namespace std;
using namespace folly;
using namespace apache::thrift::concurrency;

namespace apache {
namespace thrift {

ScopedServerInterfaceThread::ScopedServerInterfaceThread(
    shared_ptr<AsyncProcessorFactory> apf,
    SocketAddress const& addr,
    ServerConfigCb configCb) {
  auto tf = make_shared<PosixThreadFactory>(PosixThreadFactory::ATTACHED);
  auto tm = ThreadManager::newSimpleThreadManager(1, false);
  tm->threadFactory(move(tf));
  tm->start();
  auto ts = make_shared<ThriftServer>();
  ts->setAddress(addr);
  ts->setProcessorFactory(move(apf));
  ts->setNumIOWorkerThreads(1);
  ts->setThreadManager(tm);
  ts->enableRocketServer(true);
  ts->addRoutingHandler(std::make_unique<RSRoutingHandler>());
  if (configCb) {
    configCb(*ts);
  }
  ts_ = ts;
  sst_.start(ts_);
  ioExecutor_ = ts->getIOThreadPool();
}

ScopedServerInterfaceThread::ScopedServerInterfaceThread(
    shared_ptr<AsyncProcessorFactory> apf,
    const string& host,
    uint16_t port,
    ServerConfigCb configCb)
    : ScopedServerInterfaceThread(
          move(apf),
          SocketAddress(host, port),
          move(configCb)) {}

ScopedServerInterfaceThread::ScopedServerInterfaceThread(
    shared_ptr<BaseThriftServer> bts) {
  ts_ = bts;
  sst_.start(ts_);
  if (auto ts = dynamic_cast<ThriftServer*>(bts.get())) {
    ioExecutor_ = ts->getIOThreadPool();
  } else {
    ioExecutor_ = std::make_shared<IOThreadPoolExecutor>(1);
  }
}

BaseThriftServer& ScopedServerInterfaceThread::getThriftServer() const {
  return *ts_;
}

const SocketAddress& ScopedServerInterfaceThread::getAddress() const {
  return *sst_.getAddress();
}

uint16_t ScopedServerInterfaceThread::getPort() const {
  return getAddress().getPort();
}

} // namespace thrift
} // namespace apache
