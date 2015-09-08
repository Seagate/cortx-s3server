
#include "s3_clovis_writer.h"

// This is run on main thread.
extern "C" void object_cw_done_on_main_thread(evutil_socket_t, short events, void *user_data) {
  S3ClovisWriterContext *context = (S3ClovisWriterContext*) user_data;
  context->handler()();  // Invoke the handler.
  free_basic_op_ctx(context->clovis_op_ctx());
  if (ctx->clovis_writer()->state() == S3ClovisWriterOpState::saved()) {
    free_basic_rw_op_ctx(context->clovis_op_ctx());    
  }
  delete context;
}

// Clovis callbacks, run in clovis thread
extern "C" static void s3_clovis_op_stable(struct m0_clovis_op *op) {
  struct S3ClovisWriterContext *ctx = (struct S3ClovisWriterContext*)op->op_cbs->ocb_arg;

  ctx->set_op_status(S3AsyncOpStatus::success, "Success.");
  ctx->clovis_writer()->advance_state();
  S3PostToMainLoop(context->get_request(), context)(object_cw_done_on_main_thread);
}

extern "C" static void s3_clovis_op_failed(struct m0_clovis_op *op) {
  S3ClovisWriterContext *ctx = (struct S3ClovisWriterContext*)op->op_cbs->ocb_arg;
  ctx->clovis_writer()->mark_failed();
  ctx->set_op_status(S3AsyncOpStatus::failed, "Operation Failed.");
  S3PostToMainLoop(context->get_request(), context)(object_cw_done_on_main_thread);
}

S3ClovisWriter::S3ClovisWriter(S3RequestObject* req) : request(req), state(S3ClovisWriterOpState::start) {

}

void S3ClovisWriter::advance_state() {
  state++;
}

void S3ClovisWriter::mark_failed() {
  state = S3ClovisWriterOpState::failed;
}

void S3ClovisWriter::save(std::function<void(void)> handler) {
  // Load from clovis.
  this->handler = handler;

  create_object();
}

void S3ClovisWriter::create_object() {
  int                  rc;
  struct s3_clovis_op_context ctx;

  create_basic_op_ctx(&ctx, 1);
  S3ClovisWriterContext context = new S3ClovisWriterContext(request, std::bind( &S3ClovisWriter::save_content, this), ctx);

  ctx.cbs->ocb_arg = (void *)context;
  ctx.cbs->ocb_executed = NULL;
  ctx.cbs->ocb_stable = s3_clovis_op_stable;
  ctx.cbs->ocb_failed = s3_clovis_op_failed;

  struct m0_uint128 id = S3UriToMeroOID(request);

  m0_clovis_obj_init(ctx.obj, &clovis_uber_scope, &id);

  m0_clovis_entity_create(&ctx.obj->ob_entity, &ctx->ops[0]);

  m0_clovis_op_setup(ctx.ops[0], cbs, 0);
  m0_clovis_op_launch(ctx.ops, ARRAY_SIZE(ops));
}

void S3ClovisWriter::save_content() {

}

void S3ClovisWriter::set_status() {

}
