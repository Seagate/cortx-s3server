/*
 * Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
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
 *
 * For any questions about this software or licensing,
 * please email opensource@seagate.com or cortx-questions@seagate.com.
 *
 */

#pragma once

#ifndef __S3_SERVER_ATEXIT_H__

#include <functional>

class AtExit {
 private:
  std::function<void()> handler;
  bool enabled = true;

 public:
  AtExit(std::function<void()> handler_) : handler(handler_) {}
  ~AtExit() { call_now(); }
  void cancel() { enabled = false; }
  void reenable() { enabled = true; }
  void call_now() {
    if (enabled && handler) handler();
    enabled = false;
  }
};

#endif
