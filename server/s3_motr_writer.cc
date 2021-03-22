/*
 * Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For any questions about this software or licensing,
 * please email opensource@seagate.com or cortx-questions@seagate.com.
 *
 */

#include <unistd.h>
#include "s3_common.h"

#include <string.h>
#include "s3_motr_layout.h"
#include "s3_motr_rw_common.h"
#include "s3_motr_writer.h"
#include "s3_mem_pool_manager.h"
#include "s3_memory_pool.h"
#include "s3_option.h"
#include "s3_perf_logger.h"
#include "s3_stats.h"
#include "s3_timer.h"
#include "s3_uri_to_motr_oid.h"
#include "s3_addb.h"

extern struct m0_realm motr_uber_realm;
extern S3Option *g_option_instance;
extern std::set<struct s3_motr_op_context *> global_motr_object_ops_list;
extern std::set<struct s3_motr_obj_context *> global_motr_obj;
extern int shutdown_motr_teardown_called;

S3MotrWiterContext::S3MotrWiterContext(std::shared_ptr<RequestObject> req,
                                       std::function<void()> success_callback,
                                       std::function<void()> failed_callback,
                                       int ops_count,
                                       std::shared_ptr<MotrAPI> motr_api)
    : S3AsyncOpContextBase(std::move(req), std::move(success_callback),
                           std::move(failed_callback), ops_count,
                           std::move(motr_api)) {
  s3_log(S3_LOG_DEBUG, request_id, "%s Ctor\n", __func__);
}

S3MotrWiterContext::~S3MotrWiterContext() {
  s3_log(S3_LOG_DEBUG, request_id, "%s\n", __func__);

  if (motr_op_context) {
    free_basic_op_ctx(motr_op_context);
  }
  if (motr_rw_op_context) {
    free_basic_rw_op_ctx(motr_rw_op_context);
  }
}

struct s3_motr_op_context *S3MotrWiterContext::get_motr_op_ctx() {
  if (!motr_op_context) {
    // Create or write, we need op context
    motr_op_context = create_basic_op_ctx(ops_count);
  }
  return motr_op_context;
}

S3MotrWiter::S3MotrWiter(std::shared_ptr<RequestObject> req,
                         std::shared_ptr<MotrAPI> motr_api)
    : request(std::move(req)) {

  request_id = request->get_request_id();
  stripped_request_id = request->get_stripped_request_id();

  s3_log(S3_LOG_DEBUG, request_id, "%s Ctor\n", __func__);

  struct m0_uint128 oid = {0ULL, 0ULL};

  if (motr_api) {
    s3_motr_api = std::move(motr_api);
  } else {
    s3_motr_api = std::make_shared<ConcreteMotrAPI>();
  }
  std::string uri_name;

  std::shared_ptr<S3RequestObject> s3_request =
      std::dynamic_pointer_cast<S3RequestObject>(request);

  if (s3_request) {
    uri_name = s3_request->get_object_uri();
  } else {
    uri_name = request->c_get_full_path();
  }
  S3UriToMotrOID(s3_motr_api, uri_name.c_str(), request_id, &oid);

  oid_list.push_back(oid);
}

S3MotrWiter::S3MotrWiter(std::shared_ptr<RequestObject> req,
                         struct m0_uint128 object_id, struct m0_fid pv_id,
                         uint64_t offset, std::shared_ptr<MotrAPI> motr_api)
    : S3MotrWiter(std::move(req), std::move(motr_api)) {

  s3_log(S3_LOG_DEBUG, request_id, "%s Ctor\n", __func__);

  oid_list.clear();
  oid_list.push_back(object_id);

  pv_ids.push_back(pv_id);
  last_index = offset;
}

S3MotrWiter::~S3MotrWiter() {
  reset_buffers_if_any(unit_size_for_place_holder);
  clean_up_contexts();
}

void S3MotrWiter::reset_buffers_if_any(int buf_unit_sz) {
  s3_log(
      S3_LOG_DEBUG, request_id,
      "place_holder_for_last_unit(%p), last_op_was_write(%d), buf_unit_sz(%d)",
      place_holder_for_last_unit, last_op_was_write, buf_unit_sz);
  if (place_holder_for_last_unit) {
    if (last_op_was_write && (buf_unit_sz != -1)) {
      S3MempoolManager::get_instance()->release_buffer_for_unit_size(
          place_holder_for_last_unit, buf_unit_sz);
      place_holder_for_last_unit = NULL;
    } else {
      s3_log(S3_LOG_FATAL, request_id,
             "Possible bug: place_holder_for_last_unit non empty when last op "
             "was not write or buf_unit_sz = -1.");
    }
  }
}

void S3MotrWiter::clean_up_contexts() {
  // op contexts need to be free'ed before object
  open_context = nullptr;
  create_context = nullptr;
  writer_context = nullptr;
  delete_context = nullptr;
  if (!shutdown_motr_teardown_called) {
    global_motr_obj.erase(obj_ctx);
    if (obj_ctx) {
      for (size_t i = 0; i < obj_ctx->n_initialized_contexts; ++i) {
        s3_motr_api->motr_obj_fini(&obj_ctx->objs[i]);
      }
      free_obj_context(obj_ctx);
      obj_ctx = nullptr;
    }
  }
}

int S3MotrWiter::open_objects() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  is_object_opened = false;
  if (obj_ctx) {
    // clean up any old allocations
    clean_up_contexts();
  }
  obj_ctx = create_obj_context(oid_list.size());

  open_context.reset(new S3MotrWiterContext(
      request, std::bind(&S3MotrWiter::open_objects_successful, this),
      std::bind(&S3MotrWiter::open_objects_failed, this), oid_list.size(),
      s3_motr_api));

  struct s3_motr_op_context *ctx = open_context->get_motr_op_ctx();

  std::ostringstream oid_list_stream;
  const size_t ops_count = oid_list.size();

  assert(layout_ids.size() == ops_count);
  assert(pv_ids.size() == ops_count);

  for (size_t i = 0; i < ops_count; ++i) {
    struct s3_motr_context_obj *op_ctx = (struct s3_motr_context_obj *)calloc(
        1, sizeof(struct s3_motr_context_obj));

    op_ctx->op_index_in_launch = i;
    op_ctx->application_context =
        static_cast<S3AsyncOpContextBase *>(open_context.get());

    ctx->cbs[i].oop_executed = NULL;
    ctx->cbs[i].oop_stable = s3_motr_op_stable;
    ctx->cbs[i].oop_failed = s3_motr_op_failed;

    oid_list_stream << '(' << oid_list[i].u_hi << ' ' << oid_list[i].u_lo
                    << ") ";
    s3_motr_api->motr_obj_init(&obj_ctx->objs[i], &motr_uber_realm,
                               &oid_list[i], layout_ids[i]);
    if (i == 0) {
      obj_ctx->n_initialized_contexts = 1;
    } else {
      obj_ctx->n_initialized_contexts += 1;
    }

    int rc = s3_motr_api->motr_entity_open(&(obj_ctx->objs[i].ob_entity),
                                           &(ctx->ops[i]));
    if (rc != 0) {
      s3_log(S3_LOG_WARN, request_id,
             "Motr API: motr_entity_open failed with error code %d\n", rc);
      state = S3MotrWiterOpState::failed_to_launch;
      s3_motr_op_pre_launch_failure(op_ctx->application_context, rc);
      return rc;
    }
    ctx->ops[i]->op_datum = (void *)op_ctx;
    s3_motr_api->motr_op_setup(ctx->ops[i], &ctx->cbs[i], 0);

    memcpy(&obj_ctx->objs[i].ob_attr.oa_pver, &pv_ids[i],
           sizeof(struct m0_fid));
  }
  s3_log(S3_LOG_INFO, stripped_request_id, "Motr API: openobj(oid: %s)\n",
         oid_list_stream.str().c_str());
  s3_motr_api->motr_op_launch(request->addb_request_id, ctx->ops, ops_count,
                              MotrOpType::openobj);
  global_motr_object_ops_list.insert(ctx);
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
  return 0;
}

void S3MotrWiter::open_objects_successful() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  is_object_opened = true;
  if (state == S3MotrWiterOpState::writing) {
    s3_log(S3_LOG_INFO, stripped_request_id,
           "Motr API Successful: openobj(S3MotrWiterOpState: (writing))\n");
    write_content();
  } else if (state == S3MotrWiterOpState::deleting) {
    s3_log(S3_LOG_INFO, stripped_request_id,
           "Motr API Successful: openobj(S3MotrWiterOpState: (deleting))\n");
    delete_objects();
  } else {
    s3_log(S3_LOG_ERROR, request_id,
           "FATAL: open_objects_successful called from unknown op");
  }

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3MotrWiter::open_objects_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  is_object_opened = false;
  if (state != S3MotrWiterOpState::failed_to_launch) {
    size_t missing_count = 0;
    for (missing_count = 0; missing_count < oid_list.size(); missing_count++) {
      if (open_context->get_errno_for(missing_count) != -ENOENT) break;
    }

    if (missing_count == oid_list.size()) {
      s3_log(S3_LOG_ERROR, request_id, "ENOENT: All Objects missing\n");
      state = S3MotrWiterOpState::missing;
    } else {
      s3_log(S3_LOG_ERROR, request_id, "Objects opening failed\n");
      state = S3MotrWiterOpState::failed;
    }
  }
  this->handler_on_failed();

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3MotrWiter::set_oid(const struct m0_uint128 &id) {
  is_object_opened = false;
  oid_list.clear();
  oid_list.push_back(id);
}

void S3MotrWiter::set_layout_id(int id) {
  layout_ids.clear();
  layout_ids.push_back(id);
}

void S3MotrWiter::create_object(std::function<void(void)> on_success,
                                std::function<void(void)> on_failed,
                                const struct m0_uint128 &object_id,
                                int layoutid) {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry with layoutid = %d\n",
         __func__, layoutid);

  handler_on_success = std::move(on_success);
  handler_on_failed = std::move(on_failed);

  if (last_op_was_write && !layout_ids.empty()) {
    reset_buffers_if_any(unit_size_for_place_holder);
  }
  last_op_was_write = false;

  set_oid(object_id);
  set_layout_id(layoutid);

  if (obj_ctx) {
    // clean up any old allocations
    clean_up_contexts();
  }
  obj_ctx = create_obj_context(1);

  state = S3MotrWiterOpState::creating;

  create_context.reset(new S3MotrWiterContext(
      request, std::bind(&S3MotrWiter::create_object_successful, this),
      std::bind(&S3MotrWiter::create_object_failed, this)));

  struct s3_motr_op_context *ctx = create_context->get_motr_op_ctx();

  struct s3_motr_context_obj *op_ctx = (struct s3_motr_context_obj *)calloc(
      1, sizeof(struct s3_motr_context_obj));
  op_ctx->op_index_in_launch = 0;
  op_ctx->application_context =
      static_cast<S3AsyncOpContextBase *>(create_context.get());

  ctx->cbs[0].oop_executed = NULL;
  ctx->cbs[0].oop_stable = s3_motr_op_stable;
  ctx->cbs[0].oop_failed = s3_motr_op_failed;

  s3_motr_api->motr_obj_init(&obj_ctx->objs[0], &motr_uber_realm, &oid_list[0],
                             layout_ids[0]);
  obj_ctx->n_initialized_contexts = 1;

  int rc = s3_motr_api->motr_entity_create(&(obj_ctx->objs[0].ob_entity),
                                           &(ctx->ops[0]));
  if (rc != 0) {
    state = S3MotrWiterOpState::failed_to_launch;
    s3_log(S3_LOG_ERROR, request_id,
           "motr_entity_create failed with return code: (%d)\n", rc);
    s3_motr_op_pre_launch_failure(op_ctx->application_context, rc);
    return;
  }
  ctx->ops[0]->op_datum = (void *)op_ctx;

  s3_motr_api->motr_op_setup(ctx->ops[0], &ctx->cbs[0], 0);

  s3_log(S3_LOG_INFO, stripped_request_id,
         "Motr API: createobj(oid: ("
         "%" SCNx64 " : %" SCNx64 "))\n",
         oid_list[0].u_hi, oid_list[0].u_lo);

  s3_motr_api->motr_op_launch(request->addb_request_id, ctx->ops, 1,
                              MotrOpType::createobj);
  global_motr_object_ops_list.insert(ctx);
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3MotrWiter::create_object_successful() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  is_object_opened = true;  // created object is also open
  state = S3MotrWiterOpState::created;

  s3_log(S3_LOG_INFO, stripped_request_id,
         "Motr API Successful: createobj(oid: ("
         "%" SCNx64 " : %" SCNx64 "))\n",
         oid_list[0].u_hi, oid_list[0].u_lo);

  handler_on_success();

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3MotrWiter::create_object_failed() {
  auto errno_for_op = create_context->get_errno_for(0);
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry with errno = %d\n",
         __func__, errno_for_op);

  is_object_opened = false;
  if (state != S3MotrWiterOpState::failed_to_launch) {
    if (errno_for_op == -EEXIST) {
      state = S3MotrWiterOpState::exists;
      s3_log(S3_LOG_DEBUG, request_id, "Object already exists\n");
    } else {
      state = S3MotrWiterOpState::failed;
      s3_log(S3_LOG_ERROR, request_id, "Object creation failed\n");
    }
  }
  handler_on_failed();

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3MotrWiter::write_content(std::function<void(void)> on_success,
                                std::function<void(void)> on_failed,
                                S3BufferSequence buffer_sequence,
                                size_t size_of_each_buf) {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry with layout_id = %d\n",
         __func__, layout_ids[0]);

  assert(!buffer_sequence.empty());
  assert(!(size_of_each_buf & 0xFFF));  // size_of_each_buf % 4096 == 0

#ifndef NDEBUG
  if (buffer_sequence.size() > 1) {
    for (size_t i = 0, n = buffer_sequence.size() - 1; i < n; ++i) {
      assert(buffer_sequence[i].second == size_of_each_buf);
    }
  }
#endif  // NDEBUG

  handler_on_success = std::move(on_success);
  handler_on_failed = std::move(on_failed);
  this->buffer_sequence = std::move(buffer_sequence);
  this->size_of_each_buf = size_of_each_buf;

  state = S3MotrWiterOpState::writing;

  // We should already have an OID
  assert(oid_list.size() == 1);

  if (is_object_opened) {
    write_content();
  } else {
    open_objects();
  }

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3MotrWiter::write_content() {
  int rc;
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry with layout_id = %d\n",
         __func__, layout_ids[0]);

  assert(is_object_opened);

  const size_t motr_unit_size =
      S3MotrLayoutMap::get_instance()->get_unit_size_for_layout(layout_ids[0]);
  size_t motr_buf_count = buffer_sequence.size();

  // bump the count so we write at least multiple of motr_unit_size
  s3_log(S3_LOG_DEBUG, request_id, "motr_buf_count without padding: %zu\n",
         motr_buf_count);

  const size_t buffers_per_unit = motr_unit_size / size_of_each_buf;

  if (buffers_per_unit > 1) {
    size_t buffers_in_last_unit =
        motr_buf_count % buffers_per_unit;  // marked to send till now
    if (buffers_in_last_unit > 0) {
      size_t pad_buf_count = buffers_per_unit - buffers_in_last_unit;
      s3_log(S3_LOG_DEBUG, request_id, "padding with %zu buffers",
             pad_buf_count);
      motr_buf_count += pad_buf_count;
    }
  }
  writer_context.reset(new S3MotrWiterContext(
      request, std::bind(&S3MotrWiter::write_content_successful, this),
      std::bind(&S3MotrWiter::write_content_failed, this)));

  writer_context->init_write_op_ctx(motr_buf_count);

  struct s3_motr_op_context *ctx = writer_context->get_motr_op_ctx();

  struct s3_motr_rw_op_context *rw_ctx = writer_context->get_motr_rw_op_ctx();

  struct s3_motr_context_obj *op_ctx = (struct s3_motr_context_obj *)calloc(
      1, sizeof(struct s3_motr_context_obj));

  op_ctx->op_index_in_launch = 0;
  op_ctx->application_context =
      static_cast<S3AsyncOpContextBase *>(writer_context.get());

  ctx->cbs[0].oop_executed = NULL;
  ctx->cbs[0].oop_stable = s3_motr_op_stable;
  ctx->cbs[0].oop_failed = s3_motr_op_failed;

  set_up_motr_data_buffers(rw_ctx, std::move(buffer_sequence), motr_buf_count);

  if (get_s3_st_md5_error_inject_enabled()) {
    struct m0_bufvec *bv = rw_ctx->data;
    if (rw_ctx->ext->iv_index[0] == 0) {
      char first_byte = *(char *)bv->ov_buf[0];
      s3_log(S3_LOG_DEBUG, "", "%s first_byte=%d", __func__, first_byte);
      switch (first_byte) {
        case 0:
	  corrupt_fill_zero = true;
          break;
        case 1:
          // corrupt the first byte
          *(char *)bv->ov_buf[0] = ~first_byte;
          break;
      }
    }
    if (corrupt_fill_zero) {
      for (uint32_t i = 0; i < bv->ov_vec.v_nr; ++i)
        memset(bv->ov_buf[i], 0, bv->ov_vec.v_count[i]);
    }
  }

  last_op_was_write = true;

  /* Create the write request */
  rc = s3_motr_api->motr_obj_op(&obj_ctx->objs[0], M0_OC_WRITE, rw_ctx->ext,
                                rw_ctx->data, rw_ctx->attr, 0, 0, &ctx->ops[0]);
  if (rc != 0) {
    s3_log(S3_LOG_WARN, request_id,
           "Motr API: motr_obj_op failed with error code %d\n", rc);
    state = S3MotrWiterOpState::failed_to_launch;
    s3_motr_op_pre_launch_failure(op_ctx->application_context, rc);
    return;
  }

  ctx->ops[0]->op_datum = (void *)op_ctx;
  s3_motr_api->motr_op_setup(ctx->ops[0], &ctx->cbs[0], 0);
  writer_context->start_timer_for("write_to_motr_op");

  s3_log(S3_LOG_INFO, stripped_request_id,
         "Motr API: Write (operation: M0_OC_WRITE, oid: ("
         "%" SCNx64 " : %" SCNx64
         " start_offset_in_object(%zu), total_bytes_written_at_offset(%zu))\n",
         oid_list[0].u_hi, oid_list[0].u_lo, rw_ctx->ext->iv_index[0],
         size_in_current_write);
  s3_motr_api->motr_op_launch(request->addb_request_id, ctx->ops, 1,
                              MotrOpType::writeobj);
  global_motr_object_ops_list.insert(ctx);
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3MotrWiter::write_content_successful() {
  total_written += size_in_current_write;
  s3_log(S3_LOG_INFO, stripped_request_id,
         "Motr API sucessful: write(total_written = %zu)\n", total_written);
  s3_stats_inc("write_to_motr_op_success_count");

  state = S3MotrWiterOpState::saved;
  this->handler_on_success();

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3MotrWiter::write_content_failed() {
  s3_log(S3_LOG_ERROR, request_id, "Write to object failed after writing %zu\n",
         total_written);

  state = S3MotrWiterOpState::failed;
  this->handler_on_failed();

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3MotrWiter::delete_object(std::function<void(void)> on_success,
                                std::function<void(void)> on_failed,
                                const struct m0_uint128 &object_id,
                                int layoutid, const struct m0_fid &pv_id) {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry with layoutid = %d\n",
         __func__, layoutid);

  handler_on_success = std::move(on_success);
  handler_on_failed = std::move(on_failed);

  if (last_op_was_write && !layout_ids.empty()) {
    reset_buffers_if_any(unit_size_for_place_holder);
  }
  last_op_was_write = false;

  set_oid(object_id);
  set_layout_id(layoutid);

  pv_ids.clear();
  pv_ids.push_back(pv_id);

  state = S3MotrWiterOpState::deleting;

  if (is_object_opened) {
    delete_objects();
  } else {
    open_objects();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3MotrWiter::delete_objects() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  assert(is_object_opened);

  delete_context.reset(new S3MotrWiterContext(
      request, std::bind(&S3MotrWiter::delete_objects_successful, this),
      std::bind(&S3MotrWiter::delete_objects_failed, this), oid_list.size(),
      s3_motr_api));

  struct s3_motr_op_context *ctx = delete_context->get_motr_op_ctx();

  std::ostringstream oid_list_stream;
  size_t ops_count = oid_list.size();
  for (size_t i = 0; i < ops_count; ++i) {
    struct s3_motr_context_obj *op_ctx = (struct s3_motr_context_obj *)calloc(
        1, sizeof(struct s3_motr_context_obj));

    op_ctx->op_index_in_launch = i;
    op_ctx->application_context =
        static_cast<S3AsyncOpContextBase *>(delete_context.get());

    ctx->cbs[i].oop_executed = NULL;
    ctx->cbs[i].oop_stable = s3_motr_op_stable;
    ctx->cbs[i].oop_failed = s3_motr_op_failed;

    int rc = s3_motr_api->motr_entity_delete(&(obj_ctx->objs[i].ob_entity),
                                             &(ctx->ops[i]));
    if (rc != 0) {
      s3_log(S3_LOG_ERROR, request_id,
             "motr_entity_delete failed with return code: (%d)\n", rc);
      state = S3MotrWiterOpState::failed_to_launch;
      s3_motr_op_pre_launch_failure(op_ctx->application_context, rc);
      return;
    }

    ctx->ops[i]->op_datum = (void *)op_ctx;
    s3_motr_api->motr_op_setup(ctx->ops[i], &ctx->cbs[i], 0);
    oid_list_stream << '(' << oid_list[i].u_hi << ' ' << oid_list[i].u_lo
                    << ") ";
    memcpy(&obj_ctx->objs[i].ob_attr.oa_pver, &pv_ids[i],
           sizeof(struct m0_fid));
  }

  delete_context->start_timer_for("delete_objects_from_motr");

  s3_log(S3_LOG_INFO, stripped_request_id, "Motr API: deleteobj(oid: %s)\n",
         oid_list_stream.str().c_str());
  s3_motr_api->motr_op_launch(request->addb_request_id, ctx->ops, ops_count,
                              MotrOpType::deleteobj);
  global_motr_object_ops_list.insert(ctx);
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3MotrWiter::delete_objects_successful() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  state = S3MotrWiterOpState::deleted;
  s3_log(S3_LOG_INFO, stripped_request_id, "Motr API Successful: deleteobj\n");
  this->handler_on_success();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3MotrWiter::delete_objects_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (state != S3MotrWiterOpState::failed_to_launch) {
    size_t missing_count = 0;
    for (missing_count = 0; missing_count < oid_list.size(); missing_count++) {
      if (delete_context->get_errno_for(missing_count) != -ENOENT) break;
    }

    if (missing_count == oid_list.size()) {
      s3_log(S3_LOG_ERROR, request_id, "ENOENT: All Objects missing\n");
      state = S3MotrWiterOpState::missing;
    } else {
      s3_log(S3_LOG_ERROR, request_id, "Objects deletion failed\n");
      state = S3MotrWiterOpState::failed;
    }
  }
  this->handler_on_failed();

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3MotrWiter::delete_objects(std::vector<struct m0_uint128> oids,
                                 std::vector<int> layoutids,
                                 std::vector<struct m0_fid> pvids,
                                 std::function<void(void)> on_success,
                                 std::function<void(void)> on_failed) {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  handler_on_success = std::move(on_success);
  handler_on_failed = std::move(on_failed);

  oid_list = std::move(oids);
  layout_ids = std::move(layoutids);
  pv_ids = std::move(pvids);

  state = S3MotrWiterOpState::deleting;

  // Force open all objects
  open_objects();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

int S3MotrWiter::get_op_ret_code_for(int index) {
  if (writer_context) {
    return writer_context->get_errno_for(index);
  }
  s3_log(S3_LOG_ERROR, request_id, "writer_context is NULL");
  return -ENOENT;
}

int S3MotrWiter::get_op_ret_code_for_delete_op(int index) {
  if (delete_context) {
    return delete_context->get_errno_for(index);
  }
  s3_log(S3_LOG_ERROR, request_id, "delete_context is NULL");
  return -ENOENT;
}

void S3MotrWiter::set_up_motr_data_buffers(struct s3_motr_rw_op_context *rw_ctx,
                                           S3BufferSequence buffer_sequence,
                                           size_t motr_buf_count) {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);

  size_in_current_write = 0;
  size_t buf_idx = 0;

  while (!buffer_sequence.empty()) {

    const auto &ptr_n_len = buffer_sequence.front();
    const size_t len_in_buf = ptr_n_len.second;

    // Give the buffer references to Motr
    s3_log(S3_LOG_DEBUG, request_id, "To Motr: address(%p), iter(%zu)\n",
           ptr_n_len.first, buf_idx);
    s3_log(S3_LOG_DEBUG, request_id, "To Motr: len(%zu) at last_index(%zu)\n",
           len_in_buf, last_index);

    rw_ctx->data->ov_buf[buf_idx] = ptr_n_len.first;
    rw_ctx->data->ov_vec.v_count[buf_idx] = size_of_each_buf;

    // Here we use actual length to get md5
    md5crypt.Update((const char *)ptr_n_len.first, len_in_buf);

    // Init motr buffer attrs.
    rw_ctx->ext->iv_index[buf_idx] = last_index;
    rw_ctx->ext->iv_vec.v_count[buf_idx] = /*data_len*/ size_of_each_buf;
    last_index += /*data_len*/ size_of_each_buf;

    /* we don't want any attributes */
    rw_ctx->attr->ov_vec.v_count[buf_idx] = 0;

    ++buf_idx;
    size_in_current_write += len_in_buf;
    buffer_sequence.pop_front();
  }

  if (buf_idx < motr_buf_count) {
    // Allocate place_holder_for_last_unit only if its required.
    // Its required when writing last block of object.
    unit_size_for_place_holder = 16384;
    reset_buffers_if_any(unit_size_for_place_holder);
    place_holder_for_last_unit =
        (void *)S3MempoolManager::get_instance()->get_buffer_for_unit_size(
            unit_size_for_place_holder);
  }

  while (buf_idx < motr_buf_count) {
    s3_log(S3_LOG_DEBUG, request_id, "To Motr: address(%p), iter(%zu)\n",
           place_holder_for_last_unit, buf_idx);
    s3_log(S3_LOG_DEBUG, request_id, "To Motr: len(%zu) at last_index(%zu)\n",
           size_of_each_buf, last_index);

    rw_ctx->data->ov_buf[buf_idx] = place_holder_for_last_unit;
    rw_ctx->data->ov_vec.v_count[buf_idx] = size_of_each_buf;

    // Init motr buffer attrs.
    rw_ctx->ext->iv_index[buf_idx] = last_index;
    rw_ctx->ext->iv_vec.v_count[buf_idx] = /*data_len*/ size_of_each_buf;
    last_index += /*data_len*/ size_of_each_buf;

    /* we don't want any attributes */
    rw_ctx->attr->ov_vec.v_count[buf_idx] = 0;

    ++buf_idx;
  }
  assert(buf_idx == motr_buf_count);

  s3_log(S3_LOG_DEBUG, request_id, "size_in_current_write = %zu\n",
         size_in_current_write);

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

struct m0_fid *S3MotrWiter::get_ppvid() const {
  return obj_ctx && obj_ctx->n_initialized_contexts && obj_ctx->objs
             ? &obj_ctx->objs->ob_attr.oa_pver
             : nullptr;
}
