/*
 * COPYRIGHT 2015 SEAGATE LLC
 *
 * THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
 * HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE TECHNOLOGY
 * LIMITED, ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
 * THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
 * BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
 * USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
 * EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.
 *
 * YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
 * THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
 * http://www.seagate.com/contact
 *
 * Original author:  Rajesh Nambiar   <rajesh.nambiar@seagate.com>
 * Original author:  Kaustubh Deorukhkar   <kaustubh.deorukhkar@seagate.com>
 * Original creation date: 1-Oct-2015
 */

#include <event2/thread.h>

#include "s3_auth_context.h"

extern const char *auth_ip_addr;
extern uint16_t auth_port;

//To Create a auth client operation
struct s3_auth_op_context *
create_basic_auth_op_ctx(struct event_base* eventbase) {
  s3_log(S3_LOG_DEBUG, "Entering\n");

 struct s3_auth_op_context *ctx = (struct s3_auth_op_context *)calloc(1, sizeof(struct s3_auth_op_context));

 ctx->evbase = eventbase;
 // TODO do we really need this?
 if(evthread_make_base_notifiable(ctx->evbase) < 0)
   s3_log(S3_LOG_ERROR, "evthread_make_base_notifiable failed\n");

 ctx->conn = evhtp_connection_new(ctx->evbase, auth_ip_addr, auth_port);
 ctx->authrequest = evhtp_request_new(NULL, ctx->evbase);
 ctx->isfirstpass = true;
 s3_log(S3_LOG_DEBUG, "Exiting\n");
 return ctx;
}


int free_basic_auth_client_op_ctx(struct s3_auth_op_context *ctx) {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  free(ctx);
  s3_log(S3_LOG_DEBUG, "Exiting\n");
  return 0;
}
