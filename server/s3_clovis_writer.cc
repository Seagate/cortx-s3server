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
#include<unistd.h>

#include "s3_clovis_rw_common.h"
#include "s3_clovis_config.h"
#include "s3_clovis_writer.h"
#include "s3_uri_to_mero_oid.h"
#include "s3_timer.h"
#include "s3_perf_logger.h"

extern struct m0_clovis_realm     clovis_uber_realm;

S3ClovisWriter::S3ClovisWriter(std::shared_ptr<S3RequestObject> req) : request(req), state(S3ClovisWriterOpState::start) {
  last_index = 0;
  total_written = 0;
  S3UriToMeroOID(request->get_object_uri().c_str(), &id);
}

void S3ClovisWriter::create_object(std::function<void(void)> on_success, std::function<void(void)> on_failed) {
printf("S3ClovisWriter::create_object\n");
  this->handler_on_success = on_success;
  this->handler_on_failed  = on_failed;

  writer_context.reset(new S3ClovisWriterContext(request, std::bind( &S3ClovisWriter::create_object_successful, this), std::bind( &S3ClovisWriter::create_object_failed, this)));

  struct s3_clovis_op_context *ctx = writer_context->get_clovis_op_ctx();

  ctx->cbs->ocb_arg = (void *)writer_context.get();
  ctx->cbs->ocb_executed = NULL;
  ctx->cbs->ocb_stable = s3_clovis_op_stable;
  ctx->cbs->ocb_failed = s3_clovis_op_failed;

  m0_clovis_obj_init(ctx->obj, &clovis_uber_realm, &id);

  m0_clovis_entity_create(&(ctx->obj->ob_entity), &(ctx->ops[0]));

  m0_clovis_op_setup(ctx->ops[0], ctx->cbs, 0);
  m0_clovis_op_launch(ctx->ops, 1);

}

void S3ClovisWriter::create_object_successful() {
  printf("S3ClovisWriter::create_object_successful\n");
  state = S3ClovisWriterOpState::created;
  this->handler_on_success();
}

void S3ClovisWriter::create_object_failed() {
  printf("S3ClovisWriter::create_object_failed\n");
  if (writer_context->get_errno() == -EEXIST) {
    state = S3ClovisWriterOpState::exists;
  } else {
    state = S3ClovisWriterOpState::failed;
  }
  this->handler_on_failed();
}

void S3ClovisWriter::write_content(std::function<void(void)> on_success, std::function<void(void)> on_failed, S3AsyncBufferContainer& buffer) {
  printf("Called S3ClovisWriter::write_content\n");
  this->handler_on_success = on_success;
  this->handler_on_failed  = on_failed;

  size_t clovis_block_size = S3ClovisConfig::get_instance()->get_clovis_block_size();

  size_t estimated_write_length = 0;
  if (buffer.is_freezed() && buffer.length() < S3ClovisConfig::get_instance()->get_clovis_write_payload_size()) {
    estimated_write_length = buffer.length();
  } else {
    estimated_write_length = (buffer.length() / S3ClovisConfig::get_instance()->get_clovis_write_payload_size()) *  S3ClovisConfig::get_instance()->get_clovis_write_payload_size();
  }

  if (estimated_write_length > S3ClovisConfig::get_instance()->get_clovis_write_payload_size()) {
      // TODO : we should just write whatever is buffered, but mero has error where if we
      // have high block count, it fails, failure seen around 800k+ data.
      estimated_write_length = S3ClovisConfig::get_instance()->get_clovis_write_payload_size();
  }

  printf("Called S3ClovisWriter::write_content -> estimated_write_length = %zu\n", estimated_write_length);

  std::deque< std::tuple<void*, size_t> > data_items = buffer.get_buffers_ref(estimated_write_length);
  printf("Called S3ClovisWriter::write_content -> data_items size = %zu\n", data_items.size());

  size_t clovis_block_count = (estimated_write_length + (clovis_block_size - 1)) / clovis_block_size;

  writer_context.reset(new S3ClovisWriterContext(request, std::bind( &S3ClovisWriter::write_content_successful, this), std::bind( &S3ClovisWriter::write_content_failed, this)));

  writer_context->init_write_op_ctx(clovis_block_count, clovis_block_size);

  struct s3_clovis_op_context *ctx = writer_context->get_clovis_op_ctx();
  struct s3_clovis_rw_op_context *rw_ctx = writer_context->get_clovis_rw_op_ctx();

  ctx->cbs->ocb_arg = (void *)writer_context.get();
  ctx->cbs->ocb_executed = NULL;
  ctx->cbs->ocb_stable = s3_clovis_op_stable;
  ctx->cbs->ocb_failed = s3_clovis_op_failed;

  S3Timer copy_timer;
  size_t last_w_cnt = total_written;
  copy_timer.start();
  set_up_clovis_data_buffers(rw_ctx, data_items, clovis_block_count);
  copy_timer.stop();
  LOG_PERF(("copy_to_clovis_buf_" + std::to_string(total_written - last_w_cnt) + "_bytes_ns").c_str(),
    copy_timer.elapsed_time_in_nanosec());

  // We have copied data to clovis buffers.
  buffer.mark_size_of_data_consumed(estimated_write_length);

  m0_clovis_obj_init(ctx->obj, &clovis_uber_realm, &id);

  /* Create the write request */
  m0_clovis_obj_op(ctx->obj, M0_CLOVIS_OC_WRITE,
       rw_ctx->ext, rw_ctx->data, rw_ctx->attr, 0, &ctx->ops[0]);

  m0_clovis_op_setup(ctx->ops[0], ctx->cbs, 0);
  writer_context->start_timer_for("write_to_clovis_op_" + std::to_string(total_written - last_w_cnt) + "_bytes");
  m0_clovis_op_launch(ctx->ops, 1);
}

void S3ClovisWriter::write_content_successful() {
  state = S3ClovisWriterOpState::saved;
  this->handler_on_success();
}

void S3ClovisWriter::write_content_failed() {
  state = S3ClovisWriterOpState::failed;
  this->handler_on_failed();
}

void S3ClovisWriter::delete_object(std::function<void(void)> on_success, std::function<void(void)> on_failed) {
printf("S3ClovisWriter::delete_object\n");
  this->handler_on_success = on_success;
  this->handler_on_failed  = on_failed;

  writer_context.reset(new S3ClovisWriterContext(request, std::bind( &S3ClovisWriter::delete_object_successful, this), std::bind( &S3ClovisWriter::delete_object_failed, this)));

  struct s3_clovis_op_context *ctx = writer_context->get_clovis_op_ctx();

  ctx->cbs->ocb_arg = (void *)writer_context.get();
  ctx->cbs->ocb_executed = NULL;
  ctx->cbs->ocb_stable = s3_clovis_op_stable;
  ctx->cbs->ocb_failed = s3_clovis_op_failed;

  m0_clovis_obj_init(ctx->obj, &clovis_uber_realm, &id);

  m0_clovis_entity_delete(&(ctx->obj->ob_entity), &(ctx->ops[0]));

  m0_clovis_op_setup(ctx->ops[0], ctx->cbs, 0);

  writer_context->start_timer_for("delete_object_from_clovis");

  m0_clovis_op_launch(ctx->ops, 1);
}

void S3ClovisWriter::delete_object_successful() {
  printf("S3ClovisWriter::delete_object_successful\n");
  state = S3ClovisWriterOpState::deleted;
  this->handler_on_success();
}

void S3ClovisWriter::delete_object_failed() {
  printf("S3ClovisWriter::delete_object_failed\n");
  state = S3ClovisWriterOpState::failed;
  this->handler_on_failed();
}

void S3ClovisWriter::set_up_clovis_data_buffers(struct s3_clovis_rw_op_context* rw_ctx,
    std::deque< std::tuple<void*, size_t> >& data_items, size_t clovis_block_count) {
  size_t clovis_block_size = S3ClovisConfig::get_instance()->get_clovis_block_size();

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
    // printf("processing extent = %d of total %d\n", i, data_extents);
    pending_from_current_extent = data_len;
    /* KD xxx - we need to avoid this copy */
    while (1) /* consume all data in extent */
    {
      if (pending_from_current_extent == data_to_consume)
      {
        /* consume all */
        // printf("Writing @ %d bytes from extent = [%d] in block [%d] at index [%d]\n", pending_from_current_extent, i, current_block_idx, idx_within_block);
        // printf("memcpy(%p, %p, %d)\n", data.ov_buf[current_block_idx] + idx_within_block, data_ptr + idx_within_extent, pending_from_current_extent);
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
        // printf("Writing @ %d bytes from extent = [%d] in block [%d] at index [%d]\n", pending_from_current_extent, i, current_block_idx, idx_within_block);
        // printf("memcpy(%p, %p, %d)\n", data.ov_buf[current_block_idx] + idx_within_block, data_ptr + idx_within_extent, pending_from_current_extent);
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
        // printf("Writing @ %d bytes from extent = [%d] in block [%d] at index [%d]\n", data_to_consume, i, current_block_idx, idx_within_block);
        // printf("memcpy(%p, %p, %d)\n", data.ov_buf[current_block_idx] + idx_within_block, data_ptr + idx_within_extent, data_to_consume);
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
  printf("total_written = %zu\n", total_written);

  // Init clovis buffer attrs.
  for(size_t i = 0; i < clovis_block_count; i++)
  {
    rw_ctx->ext->iv_index[i] = last_index;
    rw_ctx->ext->iv_vec.v_count[i] = /*data_len*/clovis_block_size;
    last_index += /*data_len*/clovis_block_size;

    /* we don't want any attributes */
    rw_ctx->attr->ov_vec.v_count[i] = 0;
  }

}
