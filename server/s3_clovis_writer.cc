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

#include <unistd.h>
#include "s3_common.h"

#include "s3_clovis_rw_common.h"
#include "s3_clovis_writer.h"
#include "s3_memory_pool.h"
#include "s3_option.h"
#include "s3_perf_logger.h"
#include "s3_stats.h"
#include "s3_timer.h"
#include "s3_uri_to_mero_oid.h"

extern struct m0_clovis_realm clovis_uber_realm;
extern S3Option *g_option_instance;
extern MemoryPoolHandle g_clovis_read_mem_pool_handle;

S3ClovisWriter::S3ClovisWriter(std::shared_ptr<S3RequestObject> req,
                               struct m0_uint128 object_id, uint64_t offset)
    : request(req),
      oid(object_id),
      state(S3ClovisWriterOpState::start),
      last_index(offset),
      size_in_current_write(0),
      total_written(0),
      ops_count(0) {
  s3_log(S3_LOG_DEBUG, "Constructor\n");
  s3_clovis_api = std::make_shared<ConcreteClovisAPI>();
  place_holder_for_last_unit = (void *)mempool_getbuffer(
      g_clovis_read_mem_pool_handle, ZEROED_ALLOCATION);
}

S3ClovisWriter::S3ClovisWriter(std::shared_ptr<S3RequestObject> req,
                               uint64_t offset)
    : request(req), state(S3ClovisWriterOpState::start) {
  s3_log(S3_LOG_DEBUG, "Constructor\n");
  last_index = offset;
  size_in_current_write = 0;
  total_written = 0;
  oid = {0ULL, 0ULL};
  ops_count = 0;
  S3UriToMeroOID(request->get_object_uri().c_str(), &oid);
  s3_clovis_api = std::make_shared<ConcreteClovisAPI>();
  place_holder_for_last_unit = (void *)mempool_getbuffer(
      g_clovis_read_mem_pool_handle, ZEROED_ALLOCATION);
}

S3ClovisWriter::~S3ClovisWriter() {
  mempool_releasebuffer(g_clovis_read_mem_pool_handle,
                        place_holder_for_last_unit);
}

// helper to set mock clovis apis, only used in tests.
void S3ClovisWriter::reset_clovis_api(std::shared_ptr<ClovisAPI> api) {
  s3_clovis_api = api;
}

void S3ClovisWriter::create_object(std::function<void(void)> on_success,
                                   std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;

  writer_context.reset(new S3ClovisWriterContext(
      request, std::bind(&S3ClovisWriter::create_object_successful, this),
      std::bind(&S3ClovisWriter::create_object_failed, this)));

  struct s3_clovis_context_obj *op_ctx = (struct s3_clovis_context_obj *)calloc(
      1, sizeof(struct s3_clovis_context_obj));

  struct s3_clovis_op_context *ctx = writer_context->get_clovis_op_ctx();

  op_ctx->op_index_in_launch = 0;
  op_ctx->application_context = (void *)writer_context.get();

  ctx->cbs[0].oop_executed = NULL;
  ctx->cbs[0].oop_stable = s3_clovis_op_stable;
  ctx->cbs[0].oop_failed = s3_clovis_op_failed;

  s3_clovis_api->clovis_obj_init(&ctx->obj[0], &clovis_uber_realm, &oid);

  s3_clovis_api->clovis_entity_create(&(ctx->obj[0].ob_entity), &(ctx->ops[0]));

  ctx->ops[0]->op_datum = (void *)op_ctx;
  s3_clovis_api->clovis_op_setup(ctx->ops[0], &ctx->cbs[0], 0);
  s3_clovis_api->clovis_op_launch(ctx->ops, 1, ClovisOpType::createobj);
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

void S3ClovisWriter::write_content(
    std::function<void(void)> on_success, std::function<void(void)> on_failed,
    std::shared_ptr<S3AsyncBufferOptContainer> buffer) {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;

  size_t clovis_write_payload_size =
      g_option_instance->get_clovis_write_payload_size();
  size_t clovis_unit_size = g_option_instance->get_clovis_unit_size();
  size_t size_of_each_buf = g_option_instance->get_libevent_pool_buffer_size();

  size_t estimated_write_length = 0;
  if (buffer->is_freezed() &&
      buffer->get_content_length() < clovis_write_payload_size) {
    estimated_write_length = buffer->get_content_length();
  } else {
    estimated_write_length =
        (buffer->get_content_length() / clovis_write_payload_size) *
        clovis_write_payload_size;
  }

  if (estimated_write_length > clovis_write_payload_size) {
    // TODO : we should just write whatever is buffered, but mero has error
    // where if we have high block count, it fails, failure seen around 800k+
    // data.
    estimated_write_length = clovis_write_payload_size;
  }

  auto ret = buffer->get_buffers(estimated_write_length);
  std::deque<evbuffer *> data_items = ret.first;

  size_t clovis_buf_count = ret.second / size_of_each_buf;
  if (buffer->is_freezed() && (ret.second % size_of_each_buf != 0)) {
    clovis_buf_count++;
  }
  assert(data_items.size() == clovis_buf_count);

  // bump the count so we write at least multiple of clovis_unit_size
  s3_log(S3_LOG_DEBUG, "clovis_buf_count without padding: %zu\n",
         clovis_buf_count);
  int buffers_per_unit = clovis_unit_size / size_of_each_buf;
  int buffers_in_last_unit =
      clovis_buf_count % buffers_per_unit;  // marked to send till now
  if (buffers_in_last_unit > 0) {
    int pad_buf_count = buffers_per_unit - buffers_in_last_unit;
    clovis_buf_count += pad_buf_count;
  }
  s3_log(S3_LOG_DEBUG, "clovis_buf_count after padding: %zu\n",
         clovis_buf_count);

  writer_context.reset(new S3ClovisWriterContext(
      request, std::bind(&S3ClovisWriter::write_content_successful, this),
      std::bind(&S3ClovisWriter::write_content_failed, this)));

  writer_context->init_write_op_ctx(clovis_buf_count);
  writer_context->remember_async_buf(buffer);  // so we can free

  struct s3_clovis_op_context *ctx = writer_context->get_clovis_op_ctx();
  struct s3_clovis_rw_op_context *rw_ctx =
      writer_context->get_clovis_rw_op_ctx();

  struct s3_clovis_context_obj *op_ctx = (struct s3_clovis_context_obj *)calloc(
      1, sizeof(struct s3_clovis_context_obj));

  op_ctx->op_index_in_launch = 0;
  op_ctx->application_context = (void *)writer_context.get();

  ctx->cbs[0].oop_executed = NULL;
  ctx->cbs[0].oop_stable = s3_clovis_op_stable;
  ctx->cbs[0].oop_failed = s3_clovis_op_failed;

  set_up_clovis_data_buffers(rw_ctx, data_items, clovis_buf_count);

  s3_clovis_api->clovis_obj_init(&ctx->obj[0], &clovis_uber_realm, &oid);

  /* Create the write request */
  s3_clovis_api->clovis_obj_op(&ctx->obj[0], M0_CLOVIS_OC_WRITE, rw_ctx->ext,
                               rw_ctx->data, rw_ctx->attr, 0, &ctx->ops[0]);

  ctx->ops[0]->op_datum = (void *)op_ctx;
  s3_clovis_api->clovis_op_setup(ctx->ops[0], &ctx->cbs[0], 0);
  writer_context->start_timer_for("write_to_clovis_op");
  s3_clovis_api->clovis_op_launch(ctx->ops, 1, ClovisOpType::writeobj);
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ClovisWriter::write_content_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  total_written += size_in_current_write;
  s3_log(S3_LOG_DEBUG, "total_written = %zu\n", total_written);
  // We have copied data to clovis buffers.
  writer_context->get_write_async_buffer()->flush_used_buffers();

  state = S3ClovisWriterOpState::saved;
  this->handler_on_success();

  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ClovisWriter::write_content_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  s3_log(S3_LOG_ERROR, "Write to object failed after writing %zu\n",
         total_written);
  // We have failed coping data to clovis buffers.
  writer_context->get_write_async_buffer()->flush_used_buffers();

  state = S3ClovisWriterOpState::failed;
  this->handler_on_failed();

  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ClovisWriter::delete_object(std::function<void(void)> on_success,
                                   std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;

  writer_context.reset(new S3ClovisWriterContext(
      request, std::bind(&S3ClovisWriter::delete_object_successful, this),
      std::bind(&S3ClovisWriter::delete_object_failed, this)));

  struct s3_clovis_op_context *ctx = writer_context->get_clovis_op_ctx();

  struct s3_clovis_context_obj *op_ctx = (struct s3_clovis_context_obj *)calloc(
      1, sizeof(struct s3_clovis_context_obj));

  op_ctx->op_index_in_launch = 0;
  op_ctx->application_context = (void *)writer_context.get();

  ctx->cbs[0].oop_executed = NULL;
  ctx->cbs[0].oop_stable = s3_clovis_op_stable;
  ctx->cbs[0].oop_failed = s3_clovis_op_failed;

  s3_clovis_api->clovis_obj_init(&ctx->obj[0], &clovis_uber_realm, &oid);

  s3_clovis_api->clovis_entity_delete(&(ctx->obj[0].ob_entity), &(ctx->ops[0]));

  ctx->ops[0]->op_datum = (void *)op_ctx;
  s3_clovis_api->clovis_op_setup(ctx->ops[0], &ctx->cbs[0], 0);

  writer_context->start_timer_for("delete_object_from_clovis");

  s3_clovis_api->clovis_op_launch(ctx->ops, 1, ClovisOpType::deleteobj);
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
  if (writer_context->get_errno_for(0) == -ENOENT) {
    s3_log(S3_LOG_ERROR, "ENOENT: Object missing\n");
    state = S3ClovisWriterOpState::missing;
  } else {
    s3_log(S3_LOG_ERROR, "Object deletion failed\n");
    state = S3ClovisWriterOpState::failed;
  }
  this->handler_on_failed();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ClovisWriter::delete_objects(std::vector<struct m0_uint128> oids,
                                    std::function<void(void)> on_success,
                                    std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;

  writer_context.reset(new S3ClovisWriterContext(
      request, std::bind(&S3ClovisWriter::delete_objects_successful, this),
      std::bind(&S3ClovisWriter::delete_objects_failed, this), oids.size()));

  struct s3_clovis_op_context *ctx = writer_context->get_clovis_op_ctx();

  ops_count = oids.size();
  struct m0_uint128 id;
  for (int i = 0; i < ops_count; ++i) {
    id = oids[i];
    struct s3_clovis_context_obj *op_ctx =
        (struct s3_clovis_context_obj *)calloc(
            1, sizeof(struct s3_clovis_context_obj));

    op_ctx->op_index_in_launch = i;
    op_ctx->application_context = (void *)writer_context.get();

    ctx->cbs[i].oop_executed = NULL;
    ctx->cbs[i].oop_stable = s3_clovis_op_stable;
    ctx->cbs[i].oop_failed = s3_clovis_op_failed;

    s3_clovis_api->clovis_obj_init(&ctx->obj[i], &clovis_uber_realm, &id);

    s3_clovis_api->clovis_entity_delete(&(ctx->obj[i].ob_entity),
                                        &(ctx->ops[i]));

    ctx->ops[i]->op_datum = (void *)op_ctx;
    s3_clovis_api->clovis_op_setup(ctx->ops[i], &ctx->cbs[i], 0);
  }

  writer_context->start_timer_for("delete_objects_from_clovis");

  s3_clovis_api->clovis_op_launch(ctx->ops, oids.size(),
                                  ClovisOpType::deleteobj);
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
  int missing_count = 0;

  for (missing_count = 0; missing_count < ops_count; missing_count++) {
    if (writer_context->get_errno_for(missing_count) != -ENOENT) break;
  }

  if (missing_count == ops_count) {
    s3_log(S3_LOG_ERROR, "ENOENT: All Objects missing\n");
    state = S3ClovisWriterOpState::missing;
  } else {
    s3_log(S3_LOG_ERROR, "Objects deletion failed\n");
    state = S3ClovisWriterOpState::failed;
  }
  this->handler_on_failed();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ClovisWriter::set_up_clovis_data_buffers(
    struct s3_clovis_rw_op_context *rw_ctx, std::deque<evbuffer *> &data_items,
    size_t clovis_buf_count) {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  size_in_current_write = 0;
  size_t size_of_each_buf = g_option_instance->get_libevent_pool_buffer_size();
  size_t buf_count = 0;
  while (!data_items.empty()) {
    evbuffer *buf = data_items.front();
    data_items.pop_front();
    size_t len_in_buf = evbuffer_get_length(buf);
    s3_log(S3_LOG_DEBUG, "evbuffer_get_length(buf) = %zu\n", len_in_buf);

    size_t num_of_extents = evbuffer_peek(buf, len_in_buf, NULL, NULL, 0);
    s3_log(S3_LOG_DEBUG, "num_of_extents = %zu\n", num_of_extents);

    /* do the actual peek */
    struct evbuffer_iovec *vec_in = (struct evbuffer_iovec *)calloc(
        num_of_extents, sizeof(struct evbuffer_iovec));

    /* do the actual peek at data */
    evbuffer_peek(buf, len_in_buf, NULL /*start of buffer*/, vec_in,
                  num_of_extents);

    // Give the buffer references to Clovis
    for (size_t i = 0; i < num_of_extents; ++i) {
      s3_log(S3_LOG_DEBUG, "To Clovis: address(%p), iter(%zu)\n",
             vec_in[i].iov_base, buf_count);
      s3_log(S3_LOG_DEBUG, "To Clovis: len(%zu) at last_index(%zu)\n",
             vec_in[i].iov_len, last_index);

      rw_ctx->data->ov_buf[buf_count] = vec_in[i].iov_base;
      rw_ctx->data->ov_vec.v_count[buf_count] = size_of_each_buf;

      // Here we use actual length to get md5
      md5crypt.Update((const char *)vec_in[i].iov_base, vec_in[i].iov_len);

      // Init clovis buffer attrs.
      rw_ctx->ext->iv_index[buf_count] = last_index;
      rw_ctx->ext->iv_vec.v_count[buf_count] = /*data_len*/ size_of_each_buf;
      last_index += /*data_len*/ size_of_each_buf;

      /* we don't want any attributes */
      rw_ctx->attr->ov_vec.v_count[buf_count] = 0;

      ++buf_count;
    }

    size_in_current_write += len_in_buf;
    free(vec_in);
  }

  while (buf_count < clovis_buf_count) {
    s3_log(S3_LOG_DEBUG, "To Clovis: address(%p), iter(%zu)\n",
           place_holder_for_last_unit, buf_count);
    s3_log(S3_LOG_DEBUG, "To Clovis: len(%zu) at last_index(%zu)\n",
           size_of_each_buf, last_index);

    rw_ctx->data->ov_buf[buf_count] = place_holder_for_last_unit;
    rw_ctx->data->ov_vec.v_count[buf_count] = size_of_each_buf;

    // Init clovis buffer attrs.
    rw_ctx->ext->iv_index[buf_count] = last_index;
    rw_ctx->ext->iv_vec.v_count[buf_count] = /*data_len*/ size_of_each_buf;
    last_index += /*data_len*/ size_of_each_buf;

    /* we don't want any attributes */
    rw_ctx->attr->ov_vec.v_count[buf_count] = 0;

    ++buf_count;
  }
  s3_log(S3_LOG_DEBUG, "size_in_current_write = %zu\n", size_in_current_write);

  s3_log(S3_LOG_DEBUG, "Exiting\n");
}
