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
#include "s3_option.h"
#include "s3_clovis_writer.h"
#include "s3_timer.h"
#include "s3_perf_logger.h"
#include "s3_uri_to_mero_oid.h"

extern struct m0_clovis_realm     clovis_uber_realm;

S3ClovisWriter::S3ClovisWriter(std::shared_ptr<S3RequestObject> req, struct m0_uint128 object_id, uint64_t offset) : oid(object_id), request(req), state(S3ClovisWriterOpState::start) {
  s3_log(S3_LOG_DEBUG, "Constructor\n");
  last_index = offset;
  total_written = 0;
  ops_count = 0;
}

S3ClovisWriter::S3ClovisWriter(std::shared_ptr<S3RequestObject> req, uint64_t offset) : request(req), state(S3ClovisWriterOpState::start) {
  s3_log(S3_LOG_DEBUG, "Constructor\n");
  last_index = offset;
  total_written = 0;
  ops_count = 0;
  S3UriToMeroOID(request->get_object_uri().c_str(), &oid);
}

void S3ClovisWriter::create_object(std::function<void(void)> on_success, std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  this->handler_on_success = on_success;
  this->handler_on_failed  = on_failed;

  writer_context.reset(new S3ClovisWriterContext(request, std::bind( &S3ClovisWriter::create_object_successful, this), std::bind( &S3ClovisWriter::create_object_failed, this)));

  struct s3_clovis_context_obj *op_ctx = (struct s3_clovis_context_obj*)calloc(1, sizeof(struct s3_clovis_context_obj));

  struct s3_clovis_op_context *ctx = writer_context->get_clovis_op_ctx();

  op_ctx->op_index_in_launch = 0;
  op_ctx->application_context = (void *)writer_context.get();

  ctx->cbs[0].oop_executed = NULL;
  ctx->cbs[0].oop_stable = s3_clovis_op_stable;
  ctx->cbs[0].oop_failed = s3_clovis_op_failed;

  m0_clovis_obj_init(&ctx->obj[0], &clovis_uber_realm, &oid);

  m0_clovis_entity_create(&(ctx->obj[0].ob_entity), &(ctx->ops[0]));

  ctx->ops[0]->op_datum = (void *)op_ctx;
  m0_clovis_op_setup(ctx->ops[0], &ctx->cbs[0], 0);
  m0_clovis_op_launch(ctx->ops, 1);
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ClovisWriter::create_object_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  state = S3ClovisWriterOpState::created;
  this->handler_on_success();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ClovisWriter::create_object_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (writer_context->get_errno_for(0) == -EEXIST) {
    state = S3ClovisWriterOpState::exists;
    s3_log(S3_LOG_DEBUG, "Object already exists\n");
  } else {
    state = S3ClovisWriterOpState::failed;
    s3_log(S3_LOG_ERROR, "Object creation failed\n");
  }
  this->handler_on_failed();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ClovisWriter::write_content(std::function<void(void)> on_success, std::function<void(void)> on_failed, S3AsyncBufferContainer& buffer) {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  this->handler_on_success = on_success;
  this->handler_on_failed  = on_failed;

  size_t clovis_block_size = S3Option::get_instance()->get_clovis_block_size();
  size_t clovis_write_payload_size = S3Option::get_instance()->get_clovis_write_payload_size();

  size_t estimated_write_length = 0;
  if (buffer.is_freezed() && buffer.length() < clovis_write_payload_size) {
    estimated_write_length = buffer.length();
  } else {
    estimated_write_length = (buffer.length() / clovis_write_payload_size) *  clovis_write_payload_size;
  }

  if (estimated_write_length > clovis_write_payload_size) {
      // TODO : we should just write whatever is buffered, but mero has error where if we
      // have high block count, it fails, failure seen around 800k+ data.
      estimated_write_length = clovis_write_payload_size;
  }

  std::deque< std::tuple<void*, size_t> > data_items = buffer.get_buffers_ref(estimated_write_length);

  size_t clovis_block_count = (estimated_write_length + (clovis_block_size - 1)) / clovis_block_size;

  writer_context.reset(new S3ClovisWriterContext(request, std::bind( &S3ClovisWriter::write_content_successful, this), std::bind( &S3ClovisWriter::write_content_failed, this)));

  writer_context->init_write_op_ctx(clovis_block_count, clovis_block_size);

  struct s3_clovis_op_context *ctx = writer_context->get_clovis_op_ctx();
  struct s3_clovis_rw_op_context *rw_ctx = writer_context->get_clovis_rw_op_ctx();

  struct s3_clovis_context_obj *op_ctx = (struct s3_clovis_context_obj*)calloc(1, sizeof(struct s3_clovis_context_obj));

  op_ctx->op_index_in_launch = 0;
  op_ctx->application_context = (void *)writer_context.get();

  ctx->cbs[0].oop_executed = NULL;
  ctx->cbs[0].oop_stable = s3_clovis_op_stable;
  ctx->cbs[0].oop_failed = s3_clovis_op_failed;

  S3Timer copy_timer;
  size_t last_w_cnt = total_written;
  copy_timer.start();
  set_up_clovis_data_buffers(rw_ctx, data_items, clovis_block_count);
  copy_timer.stop();
  LOG_PERF(("copy_to_clovis_buf_" + std::to_string(total_written - last_w_cnt) + "_bytes_ns").c_str(),
    copy_timer.elapsed_time_in_nanosec());

  // We have copied data to clovis buffers.
  buffer.mark_size_of_data_consumed(estimated_write_length);

  m0_clovis_obj_init(&ctx->obj[0], &clovis_uber_realm, &oid);

  /* Create the write request */
  m0_clovis_obj_op(&ctx->obj[0], M0_CLOVIS_OC_WRITE,
       rw_ctx->ext, rw_ctx->data, rw_ctx->attr, 0, &ctx->ops[0]);

  ctx->ops[0]->op_datum = (void *)op_ctx;
  m0_clovis_op_setup(ctx->ops[0], &ctx->cbs[0], 0);
  writer_context->start_timer_for("write_to_clovis_op_" + std::to_string(total_written - last_w_cnt) + "_bytes");
  m0_clovis_op_launch(ctx->ops, 1);
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ClovisWriter::write_content_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  state = S3ClovisWriterOpState::saved;
  this->handler_on_success();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ClovisWriter::write_content_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  s3_log(S3_LOG_ERROR, "Write to object failed\n");
  state = S3ClovisWriterOpState::failed;
  this->handler_on_failed();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ClovisWriter::delete_object(std::function<void(void)> on_success, std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  this->handler_on_success = on_success;
  this->handler_on_failed  = on_failed;

  writer_context.reset(new S3ClovisWriterContext(request, std::bind( &S3ClovisWriter::delete_object_successful, this), std::bind( &S3ClovisWriter::delete_object_failed, this)));

  struct s3_clovis_op_context *ctx = writer_context->get_clovis_op_ctx();

  struct s3_clovis_context_obj *op_ctx = (struct s3_clovis_context_obj*)calloc(1, sizeof(struct s3_clovis_context_obj));

  op_ctx->op_index_in_launch = 0;
  op_ctx->application_context = (void *)writer_context.get();

  ctx->cbs[0].oop_executed = NULL;
  ctx->cbs[0].oop_stable = s3_clovis_op_stable;
  ctx->cbs[0].oop_failed = s3_clovis_op_failed;

  m0_clovis_obj_init(&ctx->obj[0], &clovis_uber_realm, &oid);

  m0_clovis_entity_delete(&(ctx->obj[0].ob_entity), &(ctx->ops[0]));

  ctx->ops[0]->op_datum = (void *)op_ctx;
  m0_clovis_op_setup(ctx->ops[0], &ctx->cbs[0], 0);

  writer_context->start_timer_for("delete_object_from_clovis");

  m0_clovis_op_launch(ctx->ops, 1);
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ClovisWriter::delete_object_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  state = S3ClovisWriterOpState::deleted;
  this->handler_on_success();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ClovisWriter::delete_object_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  s3_log(S3_LOG_ERROR, "Object deletion failed\n");
  if (writer_context->get_errno_for(0) == -ENOENT) {
    state = S3ClovisWriterOpState::missing;
  } else {
    state = S3ClovisWriterOpState::failed;
  }
  this->handler_on_failed();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ClovisWriter::delete_objects(std::vector<struct m0_uint128> oids, std::function<void(void)> on_success, std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  this->handler_on_success = on_success;
  this->handler_on_failed  = on_failed;

  writer_context.reset(new S3ClovisWriterContext(request, std::bind( &S3ClovisWriter::delete_objects_successful, this), std::bind( &S3ClovisWriter::delete_objects_failed, this), oids.size()));

  struct s3_clovis_op_context *ctx = writer_context->get_clovis_op_ctx();

  ops_count = oids.size();
  struct m0_uint128 id;
  for (int i = 0; i < ops_count; ++i) {
    id = oids[i];
    struct s3_clovis_context_obj *op_ctx = (struct s3_clovis_context_obj*)calloc(1, sizeof(struct s3_clovis_context_obj));

    op_ctx->op_index_in_launch = i;
    op_ctx->application_context = (void *)writer_context.get();

    ctx->cbs[i].oop_executed = NULL;
    ctx->cbs[i].oop_stable = s3_clovis_op_stable;
    ctx->cbs[i].oop_failed = s3_clovis_op_failed;

    m0_clovis_obj_init(&ctx->obj[i], &clovis_uber_realm, &id);

    m0_clovis_entity_delete(&(ctx->obj[i].ob_entity), &(ctx->ops[i]));

    ctx->ops[i]->op_datum = (void *)op_ctx;
    m0_clovis_op_setup(ctx->ops[i], &ctx->cbs[i], 0);
  }

  writer_context->start_timer_for("delete_objects_from_clovis");

  m0_clovis_op_launch(ctx->ops, oids.size());
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ClovisWriter::delete_objects_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  state = S3ClovisWriterOpState::deleted;
  this->handler_on_success();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ClovisWriter::delete_objects_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  s3_log(S3_LOG_ERROR, "Objects deletion failed\n");
  state = S3ClovisWriterOpState::failed;
  this->handler_on_failed();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ClovisWriter::set_up_clovis_data_buffers(struct s3_clovis_rw_op_context* rw_ctx,
    std::deque< std::tuple<void*, size_t> >& data_items, size_t clovis_block_count) {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  size_t clovis_block_size = S3Option::get_instance()->get_clovis_block_size();

  size_t data_to_consume = clovis_block_size;
  size_t pending_from_current_extent = 0;
  int idx_within_block = 0;
  int idx_within_extent = 0;
  int current_block_idx = 0;

  char *destination = NULL;
  char *source = NULL;

  while (!data_items.empty()) {
    std::tuple<void*, size_t> item = data_items.front();
    data_items.pop_front();
    void *data_ptr = std::get<0>(item);
    size_t data_len = std::get<1>(item);
    // s3_log(S3_LOG_DEBUG, "processing extent = %d of total %d\n", i, data_extents);
    pending_from_current_extent = data_len;
    /* KD xxx - we need to avoid this copy */
    while (1) /* consume all data in extent */
    {
      if (pending_from_current_extent == data_to_consume)
      {
        /* consume all */
        // s3_log(S3_LOG_DEBUG, "Writing @ %d bytes from extent = [%d] in block [%d] at index [%d]\n", pending_from_current_extent, i, current_block_idx, idx_within_block);
        // s3_log(S3_LOG_DEBUG, "memcpy(%p, %p, %d)\n", data.ov_buf[current_block_idx] + idx_within_block, data_ptr + idx_within_extent, pending_from_current_extent);
        source = (char*)data_ptr + idx_within_extent;
        destination = (char*) rw_ctx->data->ov_buf[current_block_idx] + idx_within_block;

        memcpy(destination, source, pending_from_current_extent);
        total_written += pending_from_current_extent;
        md5crypt.Update((const char *)source, pending_from_current_extent);

        idx_within_block = idx_within_extent = 0;
        data_to_consume = clovis_block_size;
        current_block_idx++;
        break; /* while(1) */
      } else if (pending_from_current_extent < data_to_consume)
      {
        // s3_log(S3_LOG_DEBUG, "Writing @ %d bytes from extent = [%d] in block [%d] at index [%d]\n", pending_from_current_extent, i, current_block_idx, idx_within_block);
        // s3_log(S3_LOG_DEBUG, "memcpy(%p, %p, %d)\n", data.ov_buf[current_block_idx] + idx_within_block, data_ptr + idx_within_extent, pending_from_current_extent);
        source = (char*)data_ptr + idx_within_extent;
        destination = (char*)rw_ctx->data->ov_buf[current_block_idx] + idx_within_block;

        memcpy(destination, source, pending_from_current_extent);
        total_written += pending_from_current_extent;
        md5crypt.Update((const char *)source, pending_from_current_extent);

        idx_within_block = idx_within_block + pending_from_current_extent;
        idx_within_extent = 0;
        data_to_consume = data_to_consume - pending_from_current_extent;
        break; /* while(1) */
      } else /* if (pending_from_current_extent > data_to_consume) */
      {
        // s3_log(S3_LOG_DEBUG, "Writing @ %d bytes from extent = [%d] in block [%d] at index [%d]\n", data_to_consume, i, current_block_idx, idx_within_block);
        // s3_log(S3_LOG_DEBUG, "memcpy(%p, %p, %d)\n", data.ov_buf[current_block_idx] + idx_within_block, data_ptr + idx_within_extent, data_to_consume);
        source = (char*)data_ptr + idx_within_extent;
        destination = (char*)rw_ctx->data->ov_buf[current_block_idx] + idx_within_block;

        memcpy(destination, source, data_to_consume);
        total_written += data_to_consume;
        md5crypt.Update((const char *)source, data_to_consume);

        idx_within_block = 0;
        idx_within_extent = idx_within_extent + data_to_consume;
        pending_from_current_extent = pending_from_current_extent - data_to_consume;
        data_to_consume = clovis_block_size;
        current_block_idx++;
      }
    }
  }
  s3_log(S3_LOG_DEBUG, "total_written = %zu\n", total_written);

  // Init clovis buffer attrs.
  for(size_t i = 0; i < clovis_block_count; i++)
  {
    rw_ctx->ext->iv_index[i] = last_index;
    rw_ctx->ext->iv_vec.v_count[i] = /*data_len*/clovis_block_size;
    last_index += /*data_len*/clovis_block_size;

    /* we don't want any attributes */
    rw_ctx->attr->ov_vec.v_count[i] = 0;
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}
