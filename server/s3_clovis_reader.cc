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
 * Original author:  Rajesh Nambiar   <rajesh.nambiar@seagate.com>
 * Original creation date: 1-Oct-2015
 */

#include "s3_common.h"
#include <unistd.h>

#include "s3_clovis_rw_common.h"
#include "s3_clovis_config.h"
#include "s3_clovis_reader.h"
#include "s3_uri_to_mero_oid.h"

extern struct m0_clovis_realm     clovis_uber_realm;

S3ClovisReader::S3ClovisReader(std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<ClovisAPI> clovis_api) : request(req), s3_clovis_api(clovis_api),
      state(S3ClovisReaderOpState::start), clovis_rw_op_context(NULL),
      iteration_index(0), clovis_block_size(0), num_of_blocks_read(0),
      last_index(0) {
    s3_log(S3_LOG_DEBUG, "Constructor\n");
    S3UriToMeroOID(request->get_object_uri().c_str(), &oid);
}

void S3ClovisReader::read_object_data(size_t num_of_blocks,
    std::function<void(void)> on_success, std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  s3_log(S3_LOG_DEBUG, "num_of_blocks = %zu from last_index = %zu\n", num_of_blocks, last_index);

  this->handler_on_success = on_success;
  this->handler_on_failed  = on_failed;
  num_of_blocks_read = num_of_blocks;

  clovis_block_size = S3ClovisConfig::get_instance()->get_clovis_block_size();

  reader_context.reset(new S3ClovisReaderContext(request, std::bind( &S3ClovisReader::read_object_data_successful, this), std::bind( &S3ClovisReader::read_object_data_failed, this)));

  last_index = reader_context->init_read_op_ctx(num_of_blocks, clovis_block_size, last_index);

  struct s3_clovis_op_context *ctx = reader_context->get_clovis_op_ctx();
  struct s3_clovis_rw_op_context *rw_ctx = reader_context->get_clovis_rw_op_ctx();

  // Remember, so buffers can be iterated.
  clovis_rw_op_context = rw_ctx;
  iteration_index = 0;

  struct s3_clovis_context_obj *op_ctx = (struct s3_clovis_context_obj*)calloc(1, sizeof(struct s3_clovis_context_obj));

  op_ctx->op_index_in_launch = 0;
  op_ctx->application_context = (void *)reader_context.get();

  ctx->cbs[0].ocb_arg = (void *)op_ctx;
  ctx->cbs[0].ocb_executed = NULL;
  ctx->cbs[0].ocb_stable = s3_clovis_op_stable;
  ctx->cbs[0].ocb_failed = s3_clovis_op_failed;

  /* Read the requisite number of blocks from the entity */
  s3_clovis_api->clovis_obj_init(&ctx->obj[0], &clovis_uber_realm, &oid);

  /* Create the read request */
  s3_clovis_api->clovis_obj_op(&ctx->obj[0], M0_CLOVIS_OC_READ, rw_ctx->ext, rw_ctx->data, rw_ctx->attr, 0, &ctx->ops[0]);

  s3_clovis_api->clovis_op_setup(ctx->ops[0], &ctx->cbs[0], 0);

  reader_context->start_timer_for("read_object_data_" + std::to_string(num_of_blocks_read * clovis_block_size) + "_bytes");

  s3_clovis_api->clovis_op_launch(ctx->ops, 1);
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ClovisReader::read_object_data_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  state = S3ClovisReaderOpState::success;
  this->handler_on_success();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ClovisReader::read_object_data_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (reader_context->get_errno_for(0) == -ENOENT) {
    s3_log(S3_LOG_DEBUG, "Object doesn't exist\n");
    state = S3ClovisReaderOpState::missing;
  } else {
    s3_log(S3_LOG_ERROR, "Reading of object failed\n");
    state = S3ClovisReaderOpState::failed;
  }
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
  s3_log(S3_LOG_DEBUG, "Entering\n");
  s3_log(S3_LOG_DEBUG, "num_of_blocks_read = %zu from iteration_index = %zu\n", num_of_blocks_read, iteration_index);
  size_t data_read = 0;
  if (iteration_index == num_of_blocks_read) {
    s3_log(S3_LOG_DEBUG, "Exiting\n");
    return 0;
  }

  *data =  (char*)clovis_rw_op_context->data->ov_buf[iteration_index];
  data_read = clovis_rw_op_context->data->ov_vec.v_count[iteration_index];
  iteration_index++;
  s3_log(S3_LOG_DEBUG, "Exiting\n");
  return data_read;
}
