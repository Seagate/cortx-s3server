
#include "s3_clovis_rw_common.h"
#include "s3_asyncop_context_base.h"
#include "s3_post_to_main_loop.h"

// This is run on main thread.
void clovis_op_done_on_main_thread(evutil_socket_t, short events, void *user_data) {
  printf("clovis_op_done_on_main_thread\n");
  S3AsyncOpContextBase *context = (S3AsyncOpContextBase*) user_data;
  if (context->get_op_status() == S3AsyncOpStatus::success) {
    context->on_success_handler()();  // Invoke the handler.
  } else {
    context->on_failed_handler()();  // Invoke the handler.
  }
  delete context;
}

// Clovis callbacks, run in clovis thread
void s3_clovis_op_stable(struct m0_clovis_op *op) {
  printf("s3_clovis_op_stable\n");

  S3AsyncOpContextBase *ctx = (S3AsyncOpContextBase*)op->op_cbs->ocb_arg;
  ctx->set_op_status(S3AsyncOpStatus::success, "Success.");
  S3PostToMainLoop(ctx->get_request(), ctx)(clovis_op_done_on_main_thread);
}

void s3_clovis_op_failed(struct m0_clovis_op *op) {
  printf("s3_clovis_op_failed\n");
  S3AsyncOpContextBase *ctx = (struct S3AsyncOpContextBase*)op->op_cbs->ocb_arg;
  ctx->set_op_status(S3AsyncOpStatus::failed, "Operation Failed.");
  S3PostToMainLoop(ctx->get_request(), ctx)(clovis_op_done_on_main_thread);
}
