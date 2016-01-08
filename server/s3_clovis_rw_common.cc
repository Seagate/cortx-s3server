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
  S3AsyncOpContextBase *context = (S3AsyncOpContextBase*) user_data;
  context->log_timer();
  if (context->get_op_status() == S3AsyncOpStatus::success) {
    context->on_success_handler()();  // Invoke the handler.
  } else {
    context->on_failed_handler()();  // Invoke the handler.
  }
}

// Clovis callbacks, run in clovis thread
void s3_clovis_op_stable(struct m0_clovis_op *op) {
  printf("s3_clovis_op_stable with return code = %d\n", op->op_sm.sm_rc);

  S3AsyncOpContextBase *ctx = (S3AsyncOpContextBase*)op->op_cbs->ocb_arg;
  ctx->stop_timer();
  ctx->set_op_errno(op->op_sm.sm_rc);
  ctx->set_op_status(S3AsyncOpStatus::success, "Success.");
  S3PostToMainLoop(ctx->get_request(), ctx)(clovis_op_done_on_main_thread);
}

void s3_clovis_op_failed(struct m0_clovis_op *op) {
  printf("s3_clovis_op_failed with error code = %d\n", op->op_sm.sm_rc);
  S3AsyncOpContextBase *ctx = (struct S3AsyncOpContextBase*)op->op_cbs->ocb_arg;
  ctx->stop_timer(false);
  ctx->set_op_errno(op->op_sm.sm_rc);
  ctx->set_op_status(S3AsyncOpStatus::failed, "Operation Failed.");
  S3PostToMainLoop(ctx->get_request(), ctx)(clovis_op_done_on_main_thread);
}
