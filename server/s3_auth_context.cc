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

#include <event2/thread.h>

#include "s3_auth_context.h"
#include "s3_log.h"
#include "s3_option.h"

extern evhtp_ssl_ctx_t *g_ssl_auth_ctx;

struct s3_auth_op_context *create_basic_auth_op_ctx(
    struct event_base *eventbase) {
  s3_log(S3_LOG_DEBUG, "", "Entering\n");
  S3Option *option_instance = S3Option::get_instance();
  struct s3_auth_op_context *ctx =
      (struct s3_auth_op_context *)calloc(1, sizeof(struct s3_auth_op_context));
  ctx->evbase = eventbase;
  if (option_instance->is_s3_ssl_auth_enabled()) {
    ctx->conn = evhtp_connection_ssl_new(
        ctx->evbase, option_instance->get_auth_ip_addr().c_str(),
        option_instance->get_auth_port(), g_ssl_auth_ctx);
  } else {
    ctx->conn = evhtp_connection_new(
        ctx->evbase, option_instance->get_auth_ip_addr().c_str(),
        option_instance->get_auth_port());
  }

  ctx->auth_request = evhtp_request_new(NULL, ctx->evbase);

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  return ctx;
}

int free_basic_auth_client_op_ctx(struct s3_auth_op_context *ctx) {
  s3_log(S3_LOG_DEBUG, "", "Called\n");
  free(ctx);
  ctx = NULL;
  return 0;
}
