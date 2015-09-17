
#include "s3_common.h"
#include<unistd.h>

EXTERN_C_BLOCK_BEGIN
#include "module/instance.h"
#include "mero/init.h"

#include "clovis/clovis.h"
EXTERN_C_BLOCK_END

#include "s3_clovis_config.h"
#include "s3_clovis_reader.h"
#include "s3_uri_to_mero_oid.h"
#include "s3_post_to_main_loop.h"
#include "s3_md5_hash.h"

extern struct m0_clovis_scope     clovis_uber_scope;

// This is run on main thread.
extern "C" void object_read_done_on_main_thread(evutil_socket_t, short events, void *user_data) {
  printf("object_read_done_on_main_thread\n");
  S3ClovisReaderContext *context = (S3ClovisReaderContext*) user_data;
  if (context->get_op_status() == S3AsyncOpStatus::success) {
    context->on_success_handler()();  // Invoke the handler.
  } else {
    context->on_failed_handler()();  // Invoke the handler.
  }
  delete context;
}

// Clovis callbacks, run in clovis thread
extern "C" void s3_clovis_read_op_stable(struct m0_clovis_op *op) {
  printf("s3_clovis_read_op_stable\n");

  struct S3ClovisReaderContext *ctx = (struct S3ClovisReaderContext*)op->op_cbs->ocb_arg;
  ctx->set_op_status(S3AsyncOpStatus::success, "Success.");
  S3PostToMainLoop(ctx->get_request(), ctx)(object_read_done_on_main_thread);
}

extern "C" void s3_clovis_read_op_failed(struct m0_clovis_op *op) {
  printf("s3_clovis_read_op_failed\n");
  S3ClovisReaderContext *ctx = (struct S3ClovisReaderContext*)op->op_cbs->ocb_arg;
  ctx->set_op_status(S3AsyncOpStatus::failed, "Operation Failed.");
  S3PostToMainLoop(ctx->get_request(), ctx)(object_read_done_on_main_thread);
}

S3ClovisReader::S3ClovisReader(std::shared_ptr<S3RequestObject> req) : request(req), state(S3ClovisReaderOpState::start) {
  clovis_rw_op_context = NULL;
}

void S3ClovisReader::read_object(std::function<void(void)> on_success, std::function<void(void)> on_failed) {

  this->handler_on_success = on_success;
  this->handler_on_failed  = on_failed;

  size_t clovis_block_size = ClovisConfig::get_instance()->get_clovis_block_size();
  size_t clovis_block_count = 2;  // TODO get from metadata

  S3ClovisReaderContext *context = new S3ClovisReaderContext(request, std::bind( &S3ClovisReader::read_object_successful, this), std::bind( &S3ClovisReader::read_object_failed, this));

  context->init_read_op_ctx(clovis_block_count, clovis_block_size);

  struct s3_clovis_op_context *ctx = context->get_clovis_op_ctx();
  struct s3_clovis_rw_op_context *rw_ctx = context->get_clovis_rw_op_ctx();

  // Remember, so buffers can be iterated.
  clovis_rw_op_context = rw_ctx;
  iteration_index = 0;

  ctx->cbs->ocb_arg = (void *)context;
  ctx->cbs->ocb_executed = NULL;
  ctx->cbs->ocb_stable = s3_clovis_read_op_stable;
  ctx->cbs->ocb_failed = s3_clovis_read_op_failed;

  // id = M0_CLOVIS_ID_APP;
  // id.u_lo = 7778;
  S3UriToMeroOID(request->get_object_name().c_str(), &id);

  /* Read the requisite number of blocks from the entity */
  m0_clovis_obj_init(ctx->obj, &clovis_uber_scope, &id);

  /* Create the read request */
  m0_clovis_obj_op(ctx->obj, M0_CLOVIS_OC_READ, rw_ctx->ext, rw_ctx->data, rw_ctx->attr, 0, &ctx->ops[0]);

  m0_clovis_op_setup(ctx->ops[0], ctx->cbs, 0);
  m0_clovis_op_launch(ctx->ops, 1);
}

void S3ClovisReader::read_object_successful() {
  state = S3ClovisReaderOpState::complete;

  // Compute md5 - todo - we can read this from metadata (last saved state)
  size_t clovis_block_count = 2;  // TODO get from metadata
  MD5hash  md5crypt;
  content_length = 0;
  for (size_t i = 0; i < clovis_block_count; i++) {
    md5crypt.Update((const char *)clovis_rw_op_context->data->ov_buf[i], clovis_rw_op_context->data->ov_vec.v_count[i]);
    content_length += clovis_rw_op_context->data->ov_vec.v_count[i];  // todo - read from metadata.
  }
  md5crypt.Finalize();
  content_md5 = md5crypt.get_md5_string();

  this->handler_on_success();
}

void S3ClovisReader::read_object_failed() {
  state = S3ClovisReaderOpState::failed;
  this->handler_on_failed();
}

// Returns size of data in first block and 0 if there is no content,
// and content in data.
size_t S3ClovisReader::get_first_block(char** data) {
  iteration_index = 0;
  return get_next_block(data);
}

// Returns size of data in next block and -1 if there is no content or done
size_t S3ClovisReader::get_next_block(char** data) {
  size_t clovis_block_count = 2;  // TODO get from metadata
  size_t data_read = 0;
  if (iteration_index == clovis_block_count) {
    return 0;
  }

  *data =  (char*)clovis_rw_op_context->data->ov_buf[iteration_index];
  data_read = clovis_rw_op_context->data->ov_vec.v_count[iteration_index];
  iteration_index++;
  /* TODO last chunk size can be smaller than block size */
  // For last chunk the actual valid data could be smaller.
  // data_read = ObjectSize % CLOVIS_BLOCK_SIZE;
  // if (data_read == 0)
  // {
  //     /* mean last chunk size was multiple of block size */
  //     data_read = CLOVIS_BLOCK_SIZE;
  // }
  return data_read;

}
