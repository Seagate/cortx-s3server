
#include "s3_common.h"
#include<unistd.h>

#include "s3_clovis_rw_common.h"
#include "s3_clovis_config.h"
#include "s3_clovis_reader.h"
#include "s3_uri_to_mero_oid.h"

extern struct m0_clovis_scope     clovis_uber_scope;

S3ClovisReader::S3ClovisReader(std::shared_ptr<S3RequestObject> req) : request(req), state(S3ClovisReaderOpState::start), object_size(0), clovis_block_size(0), clovis_block_count(0) {
  clovis_rw_op_context = NULL;
}

void S3ClovisReader::read_object(size_t obj_size, std::function<void(void)> on_success, std::function<void(void)> on_failed) {

  object_size = obj_size;
  this->handler_on_success = on_success;
  this->handler_on_failed  = on_failed;

  clovis_block_size = ClovisConfig::get_instance()->get_clovis_block_size();
  /* Count Data blocks from data size */
  clovis_block_count = (obj_size + (clovis_block_size - 1)) / clovis_block_size;

  reader_context.reset(new S3ClovisReaderContext(request, std::bind( &S3ClovisReader::read_object_successful, this), std::bind( &S3ClovisReader::read_object_failed, this)));

  reader_context->init_read_op_ctx(clovis_block_count, clovis_block_size);

  struct s3_clovis_op_context *ctx = reader_context->get_clovis_op_ctx();
  struct s3_clovis_rw_op_context *rw_ctx = reader_context->get_clovis_rw_op_ctx();

  // Remember, so buffers can be iterated.
  clovis_rw_op_context = rw_ctx;
  iteration_index = 0;

  ctx->cbs->ocb_arg = (void *)reader_context.get();
  ctx->cbs->ocb_executed = NULL;
  ctx->cbs->ocb_stable = s3_clovis_op_stable;
  ctx->cbs->ocb_failed = s3_clovis_op_failed;

  S3UriToMeroOID(request->get_object_uri().c_str(), &id);

  /* Read the requisite number of blocks from the entity */
  m0_clovis_obj_init(ctx->obj, &clovis_uber_scope, &id);

  /* Create the read request */
  m0_clovis_obj_op(ctx->obj, M0_CLOVIS_OC_READ, rw_ctx->ext, rw_ctx->data, rw_ctx->attr, 0, &ctx->ops[0]);

  m0_clovis_op_setup(ctx->ops[0], ctx->cbs, 0);
  m0_clovis_op_launch(ctx->ops, 1);
}

void S3ClovisReader::read_object_successful() {
  state = S3ClovisReaderOpState::complete;
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
  size_t data_read = 0;
  if (iteration_index == clovis_block_count) {
    return 0;
  }

  *data =  (char*)clovis_rw_op_context->data->ov_buf[iteration_index];
  data_read = clovis_rw_op_context->data->ov_vec.v_count[iteration_index];
  iteration_index++;
  if (iteration_index == clovis_block_count) {
    /* We just read last chunk and its size can be smaller than block size */
    data_read = object_size % clovis_block_size;
    if (data_read == 0) {
        /* mean last chunk size was multiple of block size */
        data_read = clovis_block_size;
    }
  }
  return data_read;
}
