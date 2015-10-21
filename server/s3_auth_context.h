
#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_AUTH_CONTEXT_H__
#define __MERO_FE_S3_SERVER_S3_AUTH_CONTEXT_H__

#include "s3_common.h"

EXTERN_C_BLOCK_BEGIN

#include <evhtp.h>

struct s3_auth_op_context {
  evbase_t                * evbase;
  evhtp_connection_t      * conn;
  evhtp_request_t         * authrequest;
  evhtp_hook                auth_callback;
  bool                      isfirstpass;
};

struct s3_auth_op_context * create_basic_auth_op_ctx(struct event_base* eventbase);
int free_basic_auth_client_op_ctx(struct s3_auth_op_context *ctx);

EXTERN_C_BLOCK_END

#endif
