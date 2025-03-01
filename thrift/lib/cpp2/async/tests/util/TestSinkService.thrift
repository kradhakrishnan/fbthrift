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

namespace cpp2 testutil.testservice
exception MyException {
  1: string reason;
}

service TestSinkService {
  i32 test();
  // Generate numbers between `from` to `to`, return ture for the final
  // response
  sink<i32, bool> range(1: i32 from, 2: i32 to);
  sink<i32, bool> rangeThrow(1: i32 from, 2: i32 to);
  sink<i32, bool> rangeFinalResponseThrow(1: i32 from, 2: i32 to);
  sink<i32, i32> rangeEarlyResponse(1: i32 from, 2: i32 to, 3: i32 early);
  sink<i32, bool> unimplemented();
  sink<i32, bool> unSubscribedSink();
  bool, sink<i32, bool> initialThrow() throws (1: MyException e);
  bool isSinkUnSubscribed();

  sink<i32, bool> rangeChunkTimeout();
}
