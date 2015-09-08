

#include "s3_clovis_kv.h"

extern "C" void reset_s3_e_kv_pair(struct s3_e_kv_pair object) {
    object.entity = NULL;
    object.entity_length = 0;
    object.key = NULL;
    object.key_length = 0;
    object.value = NULL;
    object.value_length = 0;
}

extern "C" void free_s3_e_kv_pair(struct s3_e_kv_pair object) {
    free(object.entity);
    free(object.key);
    free(object.value);
    reset_s3_e_kv_pair(object);
}

// Clovis callbacks.
extern "C" static void op_get_cb_stable(struct m0_clovis_op *op) {
  struct s3_clovis_kv_ctx *ctx = (struct s3_clovis_kv_ctx*)op->op_cbs->ocb_arg;
  // Ask the app context to consume data.
  // ctx->app_ctx->consume(data);
  struct s3_e_kv_pair kv = {0};
  // TODO - copy data to kv
  ctx->callback(S3AsyncOpStatus::success, kv, ctx->app_ctx);

  free(ctx);
}

extern "C" static void op_get_cb_failed(struct m0_clovis_op *op) {
  struct s3_clovis_kv_ctx *ctx = (struct s3_clovis_kv_ctx*)op->op_cbs->ocb_arg;
  struct s3_e_kv_pair kv = {0};
  ctx->callback(S3AsyncOpStatus::failed, kv, ctx->app_ctx);
  free(ctx);
}

void s3_get_key_val(std::string& entity, std::string& key_name, clovis_kv_rw_callback callback, void* context) {
  struct s3_clovis_kv_ctx *ctx = (struct s3_clovis_kv_ctx*) malloc(sizeof(struct s3_clovis_kv_ctx));
  ctx->app_ctx = context;
  ctx->callback = callback;

  // Make a call to clovis sending ctx
}

void s3_put_key_val(std::string& entity, std::string& key_name, std::string& value, clovis_kv_rw_callback callback, void* context) {

}

void s3_delete_key_val(std::string& entity, std::string& key_name, clovis_kv_rw_callback callback, void* context) {

}

void s3_next_key_val(std::string& entity, std::string& key_name, int count, clovis_kv_rw_callback callback, void* context) {

}
