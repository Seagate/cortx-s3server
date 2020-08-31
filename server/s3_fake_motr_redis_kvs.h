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

#ifndef __S3_SERVER_FAKE_MOTR_REDIS__H__
#define __S3_SERVER_FAKE_MOTR_REDIS__H__

#include "s3_motr_context.h"
#include "s3_log.h"
#include "s3_option.h"

#include <memory>

#include <hiredis/hiredis.h>
#include <hiredis/async.h>
#include <hiredis/adapters/libevent.h>

class S3FakeMotrRedisKvs {
 private:
  redisAsyncContext *redis_ctx = nullptr;

 private:
  static std::unique_ptr<S3FakeMotrRedisKvs> inst;

  static void connect_cb(const redisAsyncContext *c, int status) {
    s3_log(S3_LOG_DEBUG, "", "Entering");
    if (status != REDIS_OK) {
      auto opts = S3Option::get_instance();
      s3_log(S3_LOG_FATAL, "", "Redis@%s:%d connect error: %s",
             opts->get_redis_srv_addr().c_str(), opts->get_redis_srv_port(),
             c->errstr);
    }

    s3_log(S3_LOG_DEBUG, "", "Exiting status %d", status);
  }

 private:
  S3FakeMotrRedisKvs() {
    s3_log(S3_LOG_DEBUG, "", "Entering");
    auto opts = S3Option::get_instance();
    redis_ctx = redisAsyncConnect(opts->get_redis_srv_addr().c_str(),
                                  opts->get_redis_srv_port());
    if (redis_ctx->err) {
      s3_log(S3_LOG_FATAL, "", "Redis alloc error %s", redis_ctx->errstr);
    }

    redisLibeventAttach(redis_ctx, opts->get_eventbase());
    redisAsyncSetConnectCallback(redis_ctx, connect_cb);
    s3_log(S3_LOG_DEBUG, "", "Exiting");
  }

  void close() {
    if (redis_ctx) {
      redisAsyncFree(redis_ctx);
      redis_ctx = nullptr;
    }
  }

 public:
  ~S3FakeMotrRedisKvs() { close(); }

  void kv_read(struct m0_op *op);

  void kv_next(struct m0_op *op);

  void kv_write(struct m0_op *op);

  void kv_del(struct m0_op *op);

 public:
  static S3FakeMotrRedisKvs *instance() {
    if (!inst) {
      inst = std::unique_ptr<S3FakeMotrRedisKvs>(new S3FakeMotrRedisKvs());
    }

    return inst.get();
  }

  static void destroy_instance() {
    if (inst) {
      inst->close();
    }
  }
};

#endif
