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

#include "s3_clovis_layout.h"
#include "s3_clovis_rw_common.h"
#include "s3_clovis_writer.h"
#include "s3_mem_pool_manager.h"
#include "s3_memory_pool.h"
#include "s3_option.h"
#include "s3_perf_logger.h"
#include "s3_stats.h"
#include "s3_timer.h"
#include "s3_uri_to_mero_oid.h"

extern struct m0_clovis_realm clovis_uber_realm;
extern S3Option *g_option_instance;

S3ClovisWriter::S3ClovisWriter(std::shared_ptr<S3RequestObject> req,
                               uint64_t offset,
                               std::shared_ptr<ClovisAPI> clovis_api)
    : request(req),
      state(S3ClovisWriterOpState::start),
      last_index(offset),
      size_in_current_write(0),
      total_written(0),
      is_object_opened(false),
      obj_ctx(nullptr) {
  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");

  struct m0_uint128 oid = {0ULL, 0ULL};

  if (clovis_api) {
    s3_clovis_api = clovis_api;
  } else {
    s3_clovis_api = std::make_shared<ConcreteClovisAPI>();
  }
  S3UriToMeroOID(s3_clovis_api, request->get_object_uri().c_str(), request_id,
                 &oid);

  oid_list.clear();
  oid_list.push_back(oid);
  layout_ids.clear();

  place_holder_for_last_unit = NULL;
}

S3ClovisWriter::S3ClovisWriter(std::shared_ptr<S3RequestObject> req,
                               struct m0_uint128 object_id, uint64_t offset,
                               std::shared_ptr<ClovisAPI> clovis_api)
    : S3ClovisWriter(req, offset, clovis_api) {
  request_id = request->get_request_id();
  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");

  oid_list.clear();
  oid_list.push_back(object_id);
}

S3ClovisWriter::~S3ClovisWriter() {
  if (place_holder_for_last_unit) {
    S3MempoolManager::get_instance()->release_buffer_for_unit_size(
        place_holder_for_last_unit,
        S3ClovisLayoutMap::get_instance()->get_unit_size_for_layout(
            layout_ids[0]));
  }
  clean_up_contexts();
}

void S3ClovisWriter::clean_up_contexts() {
  // op contexts need to be free'ed before object
  open_context = nullptr;
  create_context = nullptr;
  writer_context = nullptr;
  delete_context = nullptr;
  if (obj_ctx) {
    for (size_t i = 0; i < obj_ctx->obj_count; i++) {
      s3_clovis_api->clovis_obj_fini(&obj_ctx->objs[i]);
    }
    free_obj_context(obj_ctx);
    obj_ctx = nullptr;
  }
}

void S3ClovisWriter::open_objects() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

  is_object_opened = false;
  if (obj_ctx) {
    // clean up any old allocations
    clean_up_contexts();
  }
  obj_ctx = create_obj_context(oid_list.size());

  open_context.reset(new S3ClovisWriterContext(
      request, std::bind(&S3ClovisWriter::open_objects_successful, this),
      std::bind(&S3ClovisWriter::open_objects_failed, this), oid_list.size(),
      s3_clovis_api));

  struct s3_clovis_op_context *ctx = open_context->get_clovis_op_ctx();

  std::ostringstream oid_list_stream;
  size_t ops_count = oid_list.size();
  for (size_t i = 0; i < ops_count; ++i) {
    struct s3_clovis_context_obj *op_ctx =
        (struct s3_clovis_context_obj *)calloc(
            1, sizeof(struct s3_clovis_context_obj));

    op_ctx->op_index_in_launch = i;
    op_ctx->application_context = (void *)open_context.get();

    ctx->cbs[i].oop_executed = NULL;
    ctx->cbs[i].oop_stable = s3_clovis_op_stable;
    ctx->cbs[i].oop_failed = s3_clovis_op_failed;

    oid_list_stream << "(" << oid_list[i].u_hi << " " << oid_list[i].u_lo
                    << ") ";
    s3_clovis_api->clovis_obj_init(&obj_ctx->objs[i], &clovis_uber_realm,
                                   &oid_list[i], layout_ids[i]);

    s3_clovis_api->clovis_entity_open(&(obj_ctx->objs[i].ob_entity),
                                      &(ctx->ops[i]));

    ctx->ops[i]->op_datum = (void *)op_ctx;
    s3_clovis_api->clovis_op_setup(ctx->ops[i], &ctx->cbs[i], 0);
  }

  s3_log(S3_LOG_INFO, request_id, "Clovis API: openobj(oid: %s)\n",
         oid_list_stream.str().c_str());
  s3_clovis_api->clovis_op_launch(ctx->ops, ops_count, ClovisOpType::openobj);
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3ClovisWriter::open_objects_successful() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

  is_object_opened = true;
  if (state == S3ClovisWriterOpState::writing) {
    s3_log(
        S3_LOG_INFO, request_id,
        "Clovis API Successful: openobj(S3ClovisWriterOpState: (writing))\n");
    write_content();
  } else if (state == S3ClovisWriterOpState::deleting) {
    s3_log(
        S3_LOG_INFO, request_id,
        "Clovis API Successful: openobj(S3ClovisWriterOpState: (deleting))\n");
    delete_objects();
  } else {
    s3_log(S3_LOG_ERROR, request_id,
           "FATAL: open_objects_successful called from unknown op");
  }

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3ClovisWriter::open_objects_failed() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

  is_object_opened = false;
  size_t missing_count = 0;
  for (missing_count = 0; missing_count < oid_list.size(); missing_count++) {
    if (open_context->get_errno_for(missing_count) != -ENOENT) break;
  }

  if (missing_count == oid_list.size()) {
    s3_log(S3_LOG_ERROR, request_id, "ENOENT: All Objects missing\n");
    state = S3ClovisWriterOpState::missing;
  } else {
    s3_log(S3_LOG_ERROR, request_id, "Objects opening failed\n");
    state = S3ClovisWriterOpState::failed;
  }
  this->handler_on_failed();

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3ClovisWriter::create_object(std::function<void(void)> on_success,
                                   std::function<void(void)> on_failed,
                                   int layoutid) {
  s3_log(S3_LOG_DEBUG, request_id, "Entering with layoutid = %d\n", layoutid);

  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;

  layout_ids.clear();
  layout_ids.push_back(layoutid);
  is_object_opened = false;

  assert(oid_list.size() == 1);

  if (obj_ctx) {
    // clean up any old allocations
    clean_up_contexts();
  }
  obj_ctx = create_obj_context(1);

  state = S3ClovisWriterOpState::creating;

  create_context.reset(new S3ClovisWriterContext(
      request, std::bind(&S3ClovisWriter::create_object_successful, this),
      std::bind(&S3ClovisWriter::create_object_failed, this)));

  struct s3_clovis_op_context *ctx = create_context->get_clovis_op_ctx();

  struct s3_clovis_context_obj *op_ctx = (struct s3_clovis_context_obj *)calloc(
      1, sizeof(struct s3_clovis_context_obj));
  op_ctx->op_index_in_launch = 0;
  op_ctx->application_context = (void *)create_context.get();

  ctx->cbs[0].oop_executed = NULL;
  ctx->cbs[0].oop_stable = s3_clovis_op_stable;
  ctx->cbs[0].oop_failed = s3_clovis_op_failed;

  s3_clovis_api->clovis_obj_init(&obj_ctx->objs[0], &clovis_uber_realm,
                                 &oid_list[0], layout_ids[0]);

  s3_clovis_api->clovis_entity_create(&(obj_ctx->objs[0].ob_entity),
                                      &(ctx->ops[0]));

  ctx->ops[0]->op_datum = (void *)op_ctx;

  s3_clovis_api->clovis_op_setup(ctx->ops[0], &ctx->cbs[0], 0);

  s3_log(S3_LOG_INFO, request_id,
         "Clovis API: createobj(oid: ("
         "%" SCNx64 " : %" SCNx64 "))\n",
         oid_list[0].u_hi, oid_list[0].u_lo);
  s3_clovis_api->clovis_op_launch(ctx->ops, 1, ClovisOpType::createobj);

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3ClovisWriter::create_object_successful() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  is_object_opened = true;  // created object is also open
  state = S3ClovisWriterOpState::created;
  s3_log(S3_LOG_INFO, request_id,
         "Clovis API Successful: createobj(oid: ("
         "%" SCNx64 " : %" SCNx64 "))\n",
         oid_list[0].u_hi, oid_list[0].u_lo);
  this->handler_on_success();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3ClovisWriter::create_object_failed() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering with errno = %d\n",
         create_context->get_errno_for(0));

  is_object_opened = false;
  if (create_context->get_errno_for(0) == -EEXIST) {
    state = S3ClovisWriterOpState::exists;
    s3_log(S3_LOG_DEBUG, request_id, "Object already exists\n");
  } else {
    state = S3ClovisWriterOpState::failed;
    s3_log(S3_LOG_ERROR, request_id, "Object creation failed\n");
  }
  this->handler_on_failed();

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3ClovisWriter::write_content(
    std::function<void(void)> on_success, std::function<void(void)> on_failed,
    std::shared_ptr<S3AsyncBufferOptContainer> buffer) {
  s3_log(S3_LOG_DEBUG, request_id, "Entering with layout_id = %d\n",
         layout_ids[0]);

  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;
  this->write_async_buffer = buffer;

  state = S3ClovisWriterOpState::writing;

  // We should already have an OID
  assert(oid_list.size() == 1);

  if (is_object_opened) {
    write_content();
  } else {
    open_objects();
  }

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3ClovisWriter::write_content() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering with layout_id = %d\n",
         layout_ids[0]);

  assert(is_object_opened);

  size_t clovis_write_payload_size =
      g_option_instance->get_clovis_write_payload_size(layout_ids[0]);
  size_t clovis_unit_size =
      S3ClovisLayoutMap::get_instance()->get_unit_size_for_layout(
          layout_ids[0]);
  size_t size_of_each_buf = g_option_instance->get_libevent_pool_buffer_size();

  size_t estimated_write_length = 0;
  if (write_async_buffer->is_freezed() &&
      write_async_buffer->get_content_length() < clovis_write_payload_size) {
    estimated_write_length = write_async_buffer->get_content_length();
  } else {
    estimated_write_length =
        (write_async_buffer->get_content_length() / clovis_write_payload_size) *
        clovis_write_payload_size;
  }

  if (estimated_write_length > clovis_write_payload_size) {
    // TODO : we should just write whatever is buffered, but mero has error
    // where if we have high block count, it fails, failure seen around 800k+
    // data.
    estimated_write_length = clovis_write_payload_size;
  }

  auto ret = write_async_buffer->get_buffers(estimated_write_length);
  std::deque<evbuffer *> data_items = ret.first;

  size_t clovis_buf_count = ret.second / size_of_each_buf;
  if (write_async_buffer->is_freezed() &&
      (ret.second % size_of_each_buf != 0)) {
    clovis_buf_count++;
  }
  assert(data_items.size() == clovis_buf_count);

  // bump the count so we write at least multiple of clovis_unit_size
  s3_log(S3_LOG_DEBUG, request_id, "clovis_buf_count without padding: %zu\n",
         clovis_buf_count);
  int buffers_per_unit = clovis_unit_size / size_of_each_buf;
  if (buffers_per_unit == 0) {
    buffers_per_unit = 1;
  }
  int buffers_in_last_unit =
      clovis_buf_count % buffers_per_unit;  // marked to send till now
  if (buffers_in_last_unit > 0) {
    int pad_buf_count = buffers_per_unit - buffers_in_last_unit;
    clovis_buf_count += pad_buf_count;
  }
  s3_log(S3_LOG_DEBUG, request_id, "clovis_buf_count after padding: %zu\n",
         clovis_buf_count);

  writer_context.reset(new S3ClovisWriterContext(
      request, std::bind(&S3ClovisWriter::write_content_successful, this),
      std::bind(&S3ClovisWriter::write_content_failed, this)));

  writer_context->init_write_op_ctx(clovis_buf_count);

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

  /* Create the write request */
  s3_clovis_api->clovis_obj_op(&obj_ctx->objs[0], M0_CLOVIS_OC_WRITE,
                               rw_ctx->ext, rw_ctx->data, rw_ctx->attr, 0,
                               &ctx->ops[0]);

  ctx->ops[0]->op_datum = (void *)op_ctx;
  s3_clovis_api->clovis_op_setup(ctx->ops[0], &ctx->cbs[0], 0);
  writer_context->start_timer_for("write_to_clovis_op");

  s3_log(S3_LOG_INFO, request_id,
         "Clovis API: Write (operation: M0_CLOVIS_OC_WRITE, oid: ("
         "%" SCNx64 " : %" SCNx64
         " start_offset_in_object(%zu), total_bytes_written_at_offset(%zu))\n",
         oid_list[0].u_hi, oid_list[0].u_lo, rw_ctx->ext->iv_index[0],
         size_in_current_write);
  s3_clovis_api->clovis_op_launch(ctx->ops, 1, ClovisOpType::writeobj);

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3ClovisWriter::write_content_successful() {
  total_written += size_in_current_write;
  s3_log(S3_LOG_INFO, request_id,
         "Clovis API sucessful: write(total_written = %zu)\n", total_written);

  // We have copied data to clovis buffers.
  write_async_buffer->flush_used_buffers();

  state = S3ClovisWriterOpState::saved;
  this->handler_on_success();

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3ClovisWriter::write_content_failed() {
  s3_log(S3_LOG_ERROR, request_id, "Write to object failed after writing %zu\n",
         total_written);
  // We have failed coping data to clovis buffers.
  write_async_buffer->flush_used_buffers();

  state = S3ClovisWriterOpState::failed;
  this->handler_on_failed();

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3ClovisWriter::delete_object(std::function<void(void)> on_success,
                                   std::function<void(void)> on_failed,
                                   int layoutid) {
  s3_log(S3_LOG_DEBUG, request_id, "Entering with layoutid = %d\n", layoutid);
  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;

  assert(oid_list.size() == 1);
  layout_ids.clear();
  layout_ids.push_back(layoutid);

  state = S3ClovisWriterOpState::deleting;

  // We should already have an OID
  assert(oid_list.size() == 1);

  if (is_object_opened) {
    delete_objects();
  } else {
    open_objects();
  }

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3ClovisWriter::delete_objects() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

  assert(is_object_opened);

  delete_context.reset(new S3ClovisWriterContext(
      request, std::bind(&S3ClovisWriter::delete_objects_successful, this),
      std::bind(&S3ClovisWriter::delete_objects_failed, this), oid_list.size(),
      s3_clovis_api));

  struct s3_clovis_op_context *ctx = delete_context->get_clovis_op_ctx();

  std::ostringstream oid_list_stream;
  size_t ops_count = oid_list.size();
  for (size_t i = 0; i < ops_count; ++i) {
    struct s3_clovis_context_obj *op_ctx =
        (struct s3_clovis_context_obj *)calloc(
            1, sizeof(struct s3_clovis_context_obj));

    op_ctx->op_index_in_launch = i;
    op_ctx->application_context = (void *)delete_context.get();

    ctx->cbs[i].oop_executed = NULL;
    ctx->cbs[i].oop_stable = s3_clovis_op_stable;
    ctx->cbs[i].oop_failed = s3_clovis_op_failed;

    s3_clovis_api->clovis_entity_delete(&(obj_ctx->objs[i].ob_entity),
                                        &(ctx->ops[i]));

    ctx->ops[i]->op_datum = (void *)op_ctx;
    s3_clovis_api->clovis_op_setup(ctx->ops[i], &ctx->cbs[i], 0);
    oid_list_stream << "(" << oid_list[i].u_hi << " " << oid_list[i].u_lo
                    << ") ";
  }

  delete_context->start_timer_for("delete_objects_from_clovis");

  s3_log(S3_LOG_INFO, request_id, "Clovis API: deleteobj(oid: %s)\n",
         oid_list_stream.str().c_str());
  s3_clovis_api->clovis_op_launch(ctx->ops, ops_count, ClovisOpType::deleteobj);
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3ClovisWriter::delete_objects_successful() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  state = S3ClovisWriterOpState::deleted;
  s3_log(S3_LOG_INFO, request_id, "Clovis API Successful: deleteobj\n");
  this->handler_on_success();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3ClovisWriter::delete_objects_failed() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

  size_t missing_count = 0;
  for (missing_count = 0; missing_count < oid_list.size(); missing_count++) {
    if (delete_context->get_errno_for(missing_count) != -ENOENT) break;
  }

  if (missing_count == oid_list.size()) {
    s3_log(S3_LOG_ERROR, request_id, "ENOENT: All Objects missing\n");
    state = S3ClovisWriterOpState::missing;
  } else {
    s3_log(S3_LOG_ERROR, request_id, "Objects deletion failed\n");
    state = S3ClovisWriterOpState::failed;
  }
  this->handler_on_failed();

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3ClovisWriter::delete_objects(std::vector<struct m0_uint128> oids,
                                    std::vector<int> layoutids,
                                    std::function<void(void)> on_success,
                                    std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;
  oid_list.clear();
  oid_list = oids;
  layout_ids.clear();
  layout_ids = layoutids;

  state = S3ClovisWriterOpState::deleting;

  // Force open all objects
  open_objects();

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3ClovisWriter::set_up_clovis_data_buffers(
    struct s3_clovis_rw_op_context *rw_ctx, std::deque<evbuffer *> &data_items,
    size_t clovis_buf_count) {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

  if (place_holder_for_last_unit == NULL) {
    place_holder_for_last_unit =
        (void *)S3MempoolManager::get_instance()->get_buffer_for_unit_size(
            S3ClovisLayoutMap::get_instance()->get_unit_size_for_layout(
                layout_ids[0]));
  }

  size_in_current_write = 0;
  size_t size_of_each_buf = g_option_instance->get_libevent_pool_buffer_size();
  size_t buf_count = 0;
  while (!data_items.empty()) {
    evbuffer *buf = data_items.front();
    data_items.pop_front();
    size_t len_in_buf = evbuffer_get_length(buf);
    s3_log(S3_LOG_DEBUG, request_id, "evbuffer_get_length(buf) = %zu\n",
           len_in_buf);

    size_t num_of_extents = evbuffer_peek(buf, len_in_buf, NULL, NULL, 0);
    s3_log(S3_LOG_DEBUG, request_id, "num_of_extents = %zu\n", num_of_extents);

    /* do the actual peek */
    struct evbuffer_iovec *vec_in = (struct evbuffer_iovec *)calloc(
        num_of_extents, sizeof(struct evbuffer_iovec));

    /* do the actual peek at data */
    evbuffer_peek(buf, len_in_buf, NULL /*start of buffer*/, vec_in,
                  num_of_extents);

    // Give the buffer references to Clovis
    for (size_t i = 0; i < num_of_extents; ++i) {
      s3_log(S3_LOG_DEBUG, request_id, "To Clovis: address(%p), iter(%zu)\n",
             vec_in[i].iov_base, buf_count);
      s3_log(S3_LOG_DEBUG, request_id,
             "To Clovis: len(%zu) at last_index(%zu)\n", vec_in[i].iov_len,
             last_index);

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
    s3_log(S3_LOG_DEBUG, request_id, "To Clovis: address(%p), iter(%zu)\n",
           place_holder_for_last_unit, buf_count);
    s3_log(S3_LOG_DEBUG, request_id, "To Clovis: len(%zu) at last_index(%zu)\n",
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
  s3_log(S3_LOG_DEBUG, request_id, "size_in_current_write = %zu\n",
         size_in_current_write);

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}
