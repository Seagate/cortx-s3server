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
#include "s3_log.h"

// This is run on main thread.
void clovis_op_done_on_main_thread(evutil_socket_t, short events, void *user_data) {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (user_data == NULL) {
    s3_log(S3_LOG_ERROR, "Input argument user_data is NULL\n");
  }

  struct user_event_context * user_context = (struct user_event_context *)user_data;
  S3AsyncOpContextBase *context = (S3AsyncOpContextBase *)user_context->app_ctx;
  if (context == NULL) {
    s3_log(S3_LOG_ERROR, "context pointer is NULL\n");
  }
  struct event * s3user_event = (struct event *)user_context->user_event;
  if (s3user_event == NULL) {
    s3_log(S3_LOG_ERROR, "User event is NULL\n");
  }
  context->log_timer();

  if (context->is_at_least_one_op_successful()) {
    context->on_success_handler()();  // Invoke the handler.
  } else {
    context->on_failed_handler()();  // Invoke the handler.
  }
  free(user_data);
  // Free user event
  event_free(s3user_event);
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

// Clovis callbacks, run in clovis thread
void s3_clovis_op_stable(struct m0_clovis_op *op) {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  s3_log(S3_LOG_DEBUG, "Return code = %d\n", op->op_sm.sm_rc);

  struct s3_clovis_context_obj* ctx = (struct s3_clovis_context_obj*)op->op_datum;

  S3AsyncOpContextBase *app_ctx = (S3AsyncOpContextBase*)ctx->application_context;
  s3_log(S3_LOG_DEBUG, "op_index_in_launch = %d\n", ctx->op_index_in_launch);

  app_ctx->set_op_errno_for(ctx->op_index_in_launch, op->op_sm.sm_rc);
  app_ctx->set_op_status_for(ctx->op_index_in_launch, S3AsyncOpStatus::success, "Success.");

  free(ctx);
  if(app_ctx->incr_response_count() == app_ctx->get_ops_count()) {
    struct user_event_context *user_ctx = (struct user_event_context *)calloc(1, sizeof(struct user_event_context));
    user_ctx->app_ctx = app_ctx;
    app_ctx->stop_timer();
    S3PostToMainLoop((void*)user_ctx)(clovis_op_done_on_main_thread);
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void s3_clovis_op_failed(struct m0_clovis_op *op) {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  s3_log(S3_LOG_DEBUG, "Error code = %d\n", op->op_sm.sm_rc);

  struct s3_clovis_context_obj* ctx = (struct s3_clovis_context_obj*)op->op_datum;

  S3AsyncOpContextBase *app_ctx = (S3AsyncOpContextBase*)ctx->application_context;
  s3_log(S3_LOG_DEBUG, "op_index_in_launch = %d\n", ctx->op_index_in_launch);

  app_ctx->set_op_errno_for(ctx->op_index_in_launch, op->op_sm.sm_rc);
  app_ctx->set_op_status_for(ctx->op_index_in_launch, S3AsyncOpStatus::failed, "Operation Failed.");
  // If we faked failure reset clovis internal code.
  if (ctx->is_fake_failure) {
    op->op_sm.sm_rc = 0;
    ctx->is_fake_failure = 0;
  }
  free(ctx);
  if(app_ctx->incr_response_count() == app_ctx->get_ops_count()) {
    struct user_event_context *user_ctx = (struct user_event_context *)calloc(1,sizeof(struct user_event_context));
    user_ctx->app_ctx = app_ctx;
    app_ctx->stop_timer(false);
    S3PostToMainLoop((void*)user_ctx)(clovis_op_done_on_main_thread);
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void s3_clovis_dummy_op_stable(evutil_socket_t, short events, void *user_data) {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  struct user_event_context * user_context = (struct user_event_context *)user_data;
  struct m0_clovis_op *op = (struct m0_clovis_op *)user_context->app_ctx;
  op->op_sm.sm_rc = 0;  // fake success

  free(user_data);
  // Free user event
  event_free((struct event *)user_context->user_event);
  s3_clovis_op_stable(op);
}

void s3_clovis_dummy_op_failed(evutil_socket_t, short events, void *user_data) {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  struct user_event_context *user_context =
      (struct user_event_context *)user_data;
  struct m0_clovis_op *op = (struct m0_clovis_op *)user_context->app_ctx;
  struct s3_clovis_context_obj *ctx =
      (struct s3_clovis_context_obj *)op->op_datum;

  op->op_sm.sm_rc = -ETIMEDOUT;  // fake network failure
  ctx->is_fake_failure = 1;
  free(user_data);
  // Free user event
  event_free((struct event *)user_context->user_event);
  s3_clovis_op_failed(op);
}
