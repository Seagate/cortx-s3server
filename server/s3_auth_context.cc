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

#include "s3_auth_context.h"

extern const char *auth_ip_addr;
extern uint16_t auth_port;

extern "C" void
debug_auth_request_cb(evhtp_request_t * req, void * arg) {
    printf("Callback debug_auth_request_cb for new auth request called\n");
    printf("length %zu\n", evbuffer_get_length(req->buffer_in));
}

//To Create a auth client operation
struct s3_auth_op_context *
create_basic_auth_op_ctx(struct event_base* eventbase) {
  printf("Called create_basic_auth_op_ctx\n");

 struct s3_auth_op_context *ctx = (struct s3_auth_op_context *)calloc(1, sizeof(struct s3_auth_op_context));

 ctx->evbase = eventbase;
 // TODO do we really need this?
 if(evthread_make_base_notifiable(ctx->evbase) < 0)
   printf("evthread_make_base_notifiable failed\n");

 ctx->conn = evhtp_connection_new(ctx->evbase, auth_ip_addr, auth_port);
 ctx->authrequest = evhtp_request_new(debug_auth_request_cb, ctx->evbase);
 ctx->isfirstpass = true;

 return ctx;
}


int free_basic_auth_client_op_ctx(struct s3_auth_op_context *ctx) {
  printf("Called free_basic_auth_client_op_ctx\n");
  free(ctx);
  return 0;
}
