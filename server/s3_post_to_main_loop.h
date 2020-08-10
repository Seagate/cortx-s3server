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

#ifndef __S3_SERVER_S3_POST_TO_MAIN_LOOP_H__
#define __S3_SERVER_S3_POST_TO_MAIN_LOOP_H__

/* libevhtp */
#include <evhtp.h>

// #include "s3_asyncop_context_base.h"
#include "s3_log.h"

struct user_event_context {
  void *app_ctx;
  void *user_event;
};

extern "C" typedef void (*user_event_on_main_loop)(evutil_socket_t,
                                                   short events,
                                                   void *user_data);

class S3PostToMainLoop {
  void *context;

 public:
  S3PostToMainLoop(void *ctx) : context(ctx) {
    s3_log(S3_LOG_DEBUG, "", "Constructor\n");
  }

  void operator()(user_event_on_main_loop callback,
                  const std::string &request_id = "");
};

#endif
