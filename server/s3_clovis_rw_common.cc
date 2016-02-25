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
 * Original author:  Kaustubh Deorukhkar   <kaustubh.deorukhkar@seagate.com>
 * Original creation date: 1-Oct-2015
 */

#include "s3_clovis_rw_common.h"
#include "s3_asyncop_context_base.h"
#include "s3_post_to_main_loop.h"

// This is run on main thread.
void clovis_op_done_on_main_thread(evutil_socket_t, short events, void *user_data) {
  printf("clovis_op_done_on_main_thread\n");
  struct user_event_context * user_context = (struct user_event_context *)user_data;
  S3AsyncOpContextBase *context = (S3AsyncOpContextBase *)user_context->app_ctx;
  context->log_timer();

  // Free user event
  event_free((struct event *)user_context->user_event);

  if (context->is_at_least_one_op_successful()) {
    context->on_success_handler()();  // Invoke the handler.
  } else {
    context->on_failed_handler()();  // Invoke the handler.
  }
  free(user_data);
}

// Clovis callbacks, run in clovis thread
void s3_clovis_op_stable(struct m0_clovis_op *op) {
  printf("s3_clovis_op_stable with return code = %d\n", op->op_sm.sm_rc);

  struct s3_clovis_context_obj* ctx = (struct s3_clovis_context_obj*)op->op_cbs->ocb_arg;

  S3AsyncOpContextBase *app_ctx = (S3AsyncOpContextBase*)ctx->application_context;
  printf("s3_clovis_op_stable with op_index_in_launch = %d\n", ctx->op_index_in_launch);

  app_ctx->set_op_errno_for(ctx->op_index_in_launch, op->op_sm.sm_rc);
  app_ctx->set_op_status_for(ctx->op_index_in_launch, S3AsyncOpStatus::success, "Success.");

  free(ctx);
  if(app_ctx->incr_response_count() == app_ctx->get_ops_count()) {
    struct user_event_context *user_ctx = (struct user_event_context *)calloc(1, sizeof(struct user_event_context));
    user_ctx->app_ctx = app_ctx;
    app_ctx->stop_timer();
    S3PostToMainLoop(app_ctx->get_request(), user_ctx)(clovis_op_done_on_main_thread);
  }
}

void s3_clovis_op_failed(struct m0_clovis_op *op) {
  printf("s3_clovis_op_failed with error code = %d\n", op->op_sm.sm_rc);

  struct s3_clovis_context_obj* ctx = (struct s3_clovis_context_obj*)op->op_cbs->ocb_arg;

  S3AsyncOpContextBase *app_ctx = (S3AsyncOpContextBase*)ctx->application_context;
  printf("s3_clovis_op_failed with op_index_in_launch = %d\n", ctx->op_index_in_launch);

  app_ctx->set_op_errno_for(ctx->op_index_in_launch, op->op_sm.sm_rc);
  app_ctx->set_op_status_for(ctx->op_index_in_launch, S3AsyncOpStatus::failed, "Operation Failed.");

  free(ctx);
  if(app_ctx->incr_response_count() == app_ctx->get_ops_count()) {
    struct user_event_context *user_ctx = (struct user_event_context *)calloc(1,sizeof(struct user_event_context));
    user_ctx->app_ctx = app_ctx;
    app_ctx->stop_timer(false);
    S3PostToMainLoop(app_ctx->get_request(), user_ctx)(clovis_op_done_on_main_thread);
  }
}
