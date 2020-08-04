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

#ifndef __S3_SERVER_S3_AUTH_CONTEXT_H__
#define __S3_SERVER_S3_AUTH_CONTEXT_H__

#include "s3_common.h"

EXTERN_C_BLOCK_BEGIN

#include <evhtp.h>

struct s3_auth_op_context {
  evbase_t* evbase;
  evhtp_connection_t* conn;
  evhtp_request_t* auth_request;
};

struct s3_auth_op_context* create_basic_auth_op_ctx(
    struct event_base* eventbase);

int free_basic_auth_client_op_ctx(struct s3_auth_op_context* ctx);

EXTERN_C_BLOCK_END

#endif
