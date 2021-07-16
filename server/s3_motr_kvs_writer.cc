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

#include "s3_motr_kvs_writer.h"
#include "s3_motr_rw_common.h"
#include "s3_option.h"
#include "s3_uri_to_motr_oid.h"
#include "s3_stats.h"
#include "s3_addb.h"

extern struct m0_realm motr_uber_realm;
extern struct m0_container motr_container;
extern std::set<struct s3_motr_idx_op_context *> global_motr_idx_ops_list;
extern std::set<struct s3_motr_idx_context *> global_motr_idx;
extern int shutdown_motr_teardown_called;

S3MotrKVSWriter::S3MotrKVSWriter(std::shared_ptr<RequestObject> req,
                                 std::shared_ptr<MotrAPI> motr_api)
    : request(req), state(S3MotrKVSWriterOpState::start), idx_ctx(nullptr) {
  request_id = request->get_request_id();
  stripped_request_id = request->get_stripped_request_id();
  s3_log(S3_LOG_DEBUG, request_id, "%s Ctor\n", __func__);
  if (motr_api) {
    s3_motr_api = motr_api;
  } else {
    s3_motr_api = std::make_shared<ConcreteMotrAPI>();
  }
}

S3MotrKVSWriter::S3MotrKVSWriter(std::string req_id,
                                 std::shared_ptr<MotrAPI> motr_api)
    : state(S3MotrKVSWriterOpState::start), idx_ctx(nullptr) {
  request_id = std::move(req_id);
  stripped_request_id = std::string(request_id.end() - 12, request_id.end());
  s3_log(S3_LOG_DEBUG, request_id, "%s Ctor\n", __func__);
  if (motr_api) {
    s3_motr_api = std::move(motr_api);
  } else {
    s3_motr_api = std::make_shared<ConcreteMotrAPI>();
  }
}

S3MotrKVSWriter::~S3MotrKVSWriter() {
  s3_log(S3_LOG_DEBUG, request_id, "%s\n", __func__);
  clean_up_contexts();
}

void S3MotrKVSWriter::clean_up_contexts() {
  writer_context = nullptr;
  sync_context = nullptr;
  if (!shutdown_motr_teardown_called) {
    global_motr_idx.erase(idx_ctx);
    if (idx_ctx) {
      for (size_t i = 0; i < idx_ctx->n_initialized_contexts; i++) {
        if (shutdown_motr_teardown_called) {
          break;
        }
        s3_motr_api->motr_idx_fini(&idx_ctx->idx[i]);
      }
      free_idx_context(idx_ctx);
      idx_ctx = nullptr;
    }
  }
}

void S3MotrKVSWriter::create_index(const std::string &index_name,
                                   std::function<void(void)> on_success,
                                   std::function<void(void)> on_failed) {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry with index_name = %s\n",
         __func__, index_name.c_str());

  struct m0_uint128 id = {0ULL, 0ULL};
  S3UriToMotrOID(s3_motr_api, index_name.c_str(), request_id, &id,
                 S3MotrEntityType::index);

  create_index_with_oid(id, std::move(on_success), std::move(on_failed));

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3MotrKVSWriter::create_index_with_oid(
    const struct m0_uint128 &idx_oid, std::function<void(void)> on_success,
    std::function<void(void)> on_failed) {

  s3_log(S3_LOG_INFO, stripped_request_id,
         "%s Entry with idx_oid = %" SCNx64 " : %" SCNx64 "\n", __func__,
         idx_oid.u_hi, idx_oid.u_lo);

  this->handler_on_success = std::move(on_success);
  this->handler_on_failed = std::move(on_failed);

  idx_los.clear();
  idx_los.push_back({idx_oid});

  if (idx_ctx) {
    // clean up any old allocations
    clean_up_contexts();
  }
  idx_ctx = create_idx_context(1);

  writer_context.reset(new S3AsyncMotrKVSWriterContext(
      request, std::bind(&S3MotrKVSWriter::create_index_successful, this),
      std::bind(&S3MotrKVSWriter::create_index_failed, this), 1, s3_motr_api));

  struct s3_motr_idx_op_context *idx_op_ctx =
      writer_context->get_motr_idx_op_ctx();

  struct s3_motr_context_obj *op_ctx = (struct s3_motr_context_obj *)calloc(
      1, sizeof(struct s3_motr_context_obj));

  op_ctx->op_index_in_launch = 0;
  op_ctx->application_context = (void *)writer_context.get();

  idx_op_ctx->cbs->oop_executed = NULL;
  idx_op_ctx->cbs->oop_stable = s3_motr_op_stable;
  idx_op_ctx->cbs->oop_failed = s3_motr_op_failed;

  s3_motr_api->motr_idx_init(idx_ctx->idx, &motr_uber_realm, &idx_oid);
  idx_ctx->n_initialized_contexts = 1;
  // Caller(S3) will save pv id and other attributes retuned by entity create
  // API
  // Note:
  // M0_ENF_META flag is not passed during S3 global index creation. As a
  // result, Motr fallback to older model of storing pv id
  // and other attributes for S3 global indices.
  idx_ctx->idx[0].in_entity.en_flags |= M0_ENF_META;
  int rc = s3_motr_api->motr_entity_create(&(idx_ctx->idx[0].in_entity),
                                           &(idx_op_ctx->ops[0]));
  if (rc != 0) {
    state = S3MotrKVSWriterOpState::failed_to_launch;
    s3_log(S3_LOG_ERROR, request_id,
           "motr_entity_create failed with return code: (%d)\n", rc);
    s3_motr_op_pre_launch_failure(op_ctx->application_context, rc);
    return;
  }

  idx_op_ctx->ops[0]->op_datum = (void *)op_ctx;
  s3_motr_api->motr_op_setup(idx_op_ctx->ops[0], idx_op_ctx->cbs, 0);

  writer_context->start_timer_for("create_index_op");

  s3_motr_api->motr_op_launch(request->addb_request_id, idx_op_ctx->ops, 1,
                              MotrOpType::createidx);
  global_motr_idx_ops_list.insert(idx_op_ctx);
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3MotrKVSWriter::create_index_successful() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_stats_inc("create_index_op_success_count");

  state = S3MotrKVSWriterOpState::created;

  idx_los[0].pver = idx_ctx->idx->in_attr.idx_pver;
  idx_los[0].layout_type = idx_ctx->idx->in_attr.idx_layout_type;

  s3_log(S3_LOG_DEBUG, request_id,
         "Index OID: %08zx-%08zx. PVer: %08zx-%08zx. Layout_type: 0x%x",
         idx_los[0].oid.u_hi, idx_los[0].oid.u_lo, idx_los[0].pver.f_container,
         idx_los[0].pver.f_key, idx_los[0].layout_type);

  this->handler_on_success();

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3MotrKVSWriter::create_index_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (state != S3MotrKVSWriterOpState::failed_to_launch) {
    if (writer_context->get_errno_for(0) == -EEXIST) {
      state = S3MotrKVSWriterOpState::exists;
      s3_log(S3_LOG_DEBUG, request_id, "Index already exists\n");
    } else {
      s3_log(S3_LOG_DEBUG, request_id, "Index creation failed\n");
      state = S3MotrKVSWriterOpState::failed;
    }
  }
  this->handler_on_failed();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

// Sync motr is currently done using motr_idx_op

// void S3MotrKVSWriter::sync_index(std::function<void(void)> on_success,
//                                    std::function<void(void)> on_failed,
//                                    int index_count) {
//   s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry with index_count =
// %d\n",
//     __func__, index_count);
//   int rc = 0;
//   this->handler_on_success = on_success;
//   this->handler_on_failed = on_failed;
//   sync_context.reset(new S3AsyncMotrKVSWriterContext(
//       request, std::bind(&S3MotrKVSWriter::sync_index_successful, this),
//       std::bind(&S3MotrKVSWriter::sync_index_failed, this), 1,
//       s3_motr_api));
//   struct s3_motr_idx_op_context *idx_op_ctx =
//       sync_context->get_motr_idx_op_ctx();

//   struct s3_motr_context_obj *op_ctx = (struct s3_motr_context_obj
// *)calloc(
//       1, sizeof(struct s3_motr_context_obj));
//   op_ctx->op_index_in_launch = 0;
//   op_ctx->application_context = (void *)sync_context.get();

//   idx_op_ctx->cbs->oop_executed = NULL;
//   idx_op_ctx->cbs->oop_stable = s3_motr_op_stable;
//   idx_op_ctx->cbs->oop_failed = s3_motr_op_failed;
//   rc = s3_motr_api->motr_sync_op_init(&idx_op_ctx->sync_op);
//   if (rc != 0) {
//     s3_log(S3_LOG_ERROR, request_id, "m0_sync_op_init\n");
//     state = S3MotrKVSWriterOpState::failed_to_launch;
//     s3_motr_op_pre_launch_failure(op_ctx->application_context, rc);
//     return;
//   }
//   for (int i = 0; i < index_count; i++) {
//     rc = s3_motr_api->motr_sync_entity_add(idx_op_ctx->sync_op,
//                                                &(idx_ctx->idx[i].in_entity));
//     if (rc != 0) {
//       s3_log(S3_LOG_ERROR, request_id, "m0_sync_entity_add failed\n");
//       state = S3MotrKVSWriterOpState::failed_to_launch;
//       s3_motr_op_pre_launch_failure(op_ctx->application_context, rc);
//       return;
//     }
//   }
//   idx_op_ctx->sync_op->op_datum = (void *)op_ctx;

//   s3_motr_api->motr_op_setup(idx_op_ctx->sync_op, idx_op_ctx->cbs, 0);
//   sync_context->start_timer_for("sync_index_op");
//   s3_motr_api->motr_op_launch(request->addb_request_id,
//                                   &idx_op_ctx->sync_op, 1,
//                                   MotrOpType::createidx);
//   s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
// }

// void S3MotrKVSWriter::sync_index_successful() {
//   s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
//   s3_stats_inc("sync_index_op_success_count");
//   if (state == S3MotrKVSWriterOpState::deleting) {
//     state = S3MotrKVSWriterOpState::deleted;
//   } else {
//     state = S3MotrKVSWriterOpState::saved;
//   }
//   this->handler_on_success();
//   s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
// }

// void S3MotrKVSWriter::sync_index_failed() {
//   s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
//   if (state != S3MotrKVSWriterOpState::failed_to_launch) {
//     if (sync_context->get_errno_for(0) == -ENOENT) {
//       state = S3MotrKVSWriterOpState::missing;
//       s3_log(S3_LOG_DEBUG, request_id, "Index syncing failed as its
// missing\n");
//     } else {
//       s3_log(S3_LOG_DEBUG, request_id, "Index syncing failed\n");
//       state = S3MotrKVSWriterOpState::failed;
//     }
//   }
//   this->handler_on_failed();
//   s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
// }

void S3MotrKVSWriter::delete_index(const struct s3_motr_idx_layout &idx_lo,
                                   std::function<void(void)> on_success,
                                   std::function<void(void)> on_failed) {
  s3_log(S3_LOG_INFO, stripped_request_id,
         "%s Entry with idx_oid = %" SCNx64 " : %" SCNx64 "\n", __func__,
         idx_lo.oid.u_hi, idx_lo.oid.u_lo);

  this->handler_on_success = std::move(on_success);
  this->handler_on_failed = std::move(on_failed);

  idx_los.clear();
  idx_los.push_back(idx_lo);

  state = S3MotrKVSWriterOpState::deleting;

  if (idx_ctx) {
    // clean up any old allocations
    clean_up_contexts();
  }
  idx_ctx = create_idx_context(1);

  writer_context.reset(new S3AsyncMotrKVSWriterContext(
      request, std::bind(&S3MotrKVSWriter::delete_index_successful, this),
      std::bind(&S3MotrKVSWriter::delete_index_failed, this), 1, s3_motr_api));

  struct s3_motr_idx_op_context *idx_op_ctx =
      writer_context->get_motr_idx_op_ctx();

  struct s3_motr_context_obj *op_ctx = (struct s3_motr_context_obj *)calloc(
      1, sizeof(struct s3_motr_context_obj));

  op_ctx->op_index_in_launch = 0;
  op_ctx->application_context = (void *)writer_context.get();

  idx_op_ctx->cbs->oop_executed = NULL;
  idx_op_ctx->cbs->oop_stable = s3_motr_op_stable;
  idx_op_ctx->cbs->oop_failed = s3_motr_op_failed;

  s3_motr_api->motr_idx_init(&(idx_ctx->idx[0]), &motr_uber_realm, &idx_lo.oid);
  idx_ctx->n_initialized_contexts = 1;

  idx_ctx->idx->in_attr.idx_pver = idx_lo.pver;
  idx_ctx->idx->in_attr.idx_layout_type = idx_lo.layout_type;

  int rc = s3_motr_api->motr_entity_open(&(idx_ctx->idx[0].in_entity),
                                         &(idx_op_ctx->ops[0]));
  if (rc != 0) {
    s3_log(S3_LOG_ERROR, request_id,
           "motr_entity_open failed with return code: (%d)\n", rc);
    state = S3MotrKVSWriterOpState::failed_to_launch;
    s3_motr_op_pre_launch_failure(op_ctx->application_context, rc);
    return;
  }

  rc = s3_motr_api->motr_entity_delete(&(idx_ctx->idx[0].in_entity),
                                       &(idx_op_ctx->ops[0]));
  if (rc != 0) {
    s3_log(S3_LOG_ERROR, request_id,
           "motr_entity_delete failed with return code: (%d)\n", rc);
    state = S3MotrKVSWriterOpState::failed_to_launch;
    s3_motr_op_pre_launch_failure(op_ctx->application_context, rc);
    return;
  }

  idx_op_ctx->ops[0]->op_datum = (void *)op_ctx;

  s3_motr_api->motr_op_setup(idx_op_ctx->ops[0], idx_op_ctx->cbs, 0);
  writer_context->start_timer_for("delete_index_op");

  s3_motr_api->motr_op_launch(request->addb_request_id, idx_op_ctx->ops, 1,
                              MotrOpType::deleteidx);
  global_motr_idx_ops_list.insert(idx_op_ctx);
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3MotrKVSWriter::delete_index_successful() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  state = S3MotrKVSWriterOpState::deleted;
  this->handler_on_success();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3MotrKVSWriter::delete_index_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (state != S3MotrKVSWriterOpState::failed_to_launch) {
    if (writer_context->get_errno_for(0) == -ENOENT) {
      s3_log(S3_LOG_DEBUG, request_id, "Index doesn't exists\n");
      state = S3MotrKVSWriterOpState::missing;
    } else {
      s3_log(S3_LOG_ERROR, request_id, "Deletion of Index failed\n");
      state = S3MotrKVSWriterOpState::failed;
    }
  }
  this->handler_on_failed();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3MotrKVSWriter::delete_indices(
    const std::vector<struct s3_motr_idx_layout> &idx_los,
    std::function<void(void)> on_success, std::function<void(void)> on_failed) {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry with %zu indices\n",
         __func__, idx_los.size());

  for (const auto &idx_lo : idx_los) {
    s3_log(S3_LOG_DEBUG, request_id, "oid = %" SCNx64 " : %" SCNx64 "\n",
           idx_lo.oid.u_hi, idx_lo.oid.u_lo);
  }
  this->handler_on_success = std::move(on_success);
  this->handler_on_failed = std::move(on_failed);
  this->idx_los = idx_los;

  state = S3MotrKVSWriterOpState::deleting;

  if (idx_ctx) {
    // clean up any old allocations
    clean_up_contexts();
  }
  const auto ops_count = idx_los.size();
  idx_ctx = create_idx_context(ops_count);

  writer_context.reset(new S3AsyncMotrKVSWriterContext(
      request, std::bind(&S3MotrKVSWriter::delete_indices_successful, this),
      std::bind(&S3MotrKVSWriter::delete_indices_failed, this), idx_los.size(),
      s3_motr_api));

  struct s3_motr_idx_op_context *idx_op_ctx =
      writer_context->get_motr_idx_op_ctx();

  for (size_t i = 0; i < ops_count; ++i) {
    struct s3_motr_context_obj *op_ctx = (struct s3_motr_context_obj *)calloc(
        1, sizeof(struct s3_motr_context_obj));

    op_ctx->op_index_in_launch = i;
    op_ctx->application_context = (void *)writer_context.get();

    idx_op_ctx->cbs[i].oop_executed = NULL;
    idx_op_ctx->cbs[i].oop_stable = s3_motr_op_stable;
    idx_op_ctx->cbs[i].oop_failed = s3_motr_op_failed;

    s3_motr_api->motr_idx_init(&idx_ctx->idx[i], &motr_uber_realm,
                               &idx_los[i].oid);

    idx_ctx->idx[i].in_attr.idx_pver = idx_los[i].pver;
    idx_ctx->idx[i].in_attr.idx_layout_type = idx_los[i].layout_type;

    idx_ctx->n_initialized_contexts += 1;
    int rc = s3_motr_api->motr_entity_open(&(idx_ctx->idx[i].in_entity),
                                           &(idx_op_ctx->ops[i]));
    if (rc != 0) {
      s3_log(S3_LOG_ERROR, request_id,
             "motr_entity_open failed with return code: (%d)\n", rc);
      state = S3MotrKVSWriterOpState::failed_to_launch;
      s3_motr_op_pre_launch_failure(op_ctx->application_context, rc);
      return;
    }
    rc = s3_motr_api->motr_entity_delete(&(idx_ctx->idx[i].in_entity),
                                         &(idx_op_ctx->ops[i]));
    if (rc != 0) {
      s3_log(S3_LOG_ERROR, request_id,
             "motr_entity_delete failed with return code: (%d)\n", rc);
      state = S3MotrKVSWriterOpState::failed_to_launch;
      s3_motr_op_pre_launch_failure(op_ctx->application_context, rc);
      return;
    }

    idx_op_ctx->ops[i]->op_datum = (void *)op_ctx;
    s3_motr_api->motr_op_setup(idx_op_ctx->ops[i], &idx_op_ctx->cbs[i], 0);
  }

  writer_context->start_timer_for("delete_index_op");

  s3_motr_api->motr_op_launch(request->addb_request_id, idx_op_ctx->ops,
                              ops_count, MotrOpType::deleteidx);
  global_motr_idx_ops_list.insert(idx_op_ctx);
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3MotrKVSWriter::delete_indices_successful() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  state = S3MotrKVSWriterOpState::deleted;
  this->handler_on_success();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3MotrKVSWriter::delete_indices_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_log(S3_LOG_ERROR, request_id, "Deletion of Index failed\n");
  if (state != S3MotrKVSWriterOpState::failed_to_launch) {
    state = S3MotrKVSWriterOpState::failed;
  }
  this->handler_on_failed();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3MotrKVSWriter::put_keyval(
    const struct s3_motr_idx_layout &idx_lo,
    const std::map<std::string, std::string> &kv_list,
    std::function<void(void)> on_success, std::function<void(void)> on_failed) {

  s3_log(S3_LOG_INFO, stripped_request_id,
         "%s Entry with oid = %" SCNx64 " : %" SCNx64
         " and %zu key/value pairs\n",
         __func__, idx_lo.oid.u_hi, idx_lo.oid.u_lo, kv_list.size());

  for (const auto &kv : kv_list) {
    s3_log(S3_LOG_DEBUG, request_id, "key = %s, value = %s\n", kv.first.c_str(),
           kv.second.c_str());
    assert(!kv.first.empty());
    // Add error when any key is empty
    if (kv.first.empty()) {
      s3_log(S3_LOG_ERROR, request_id, "Empty key in PUT KV\n");
    }
  }
  idx_los.clear();
  idx_los.push_back(idx_lo);

  this->handler_on_success = std::move(on_success);
  this->handler_on_failed = std::move(on_failed);

  if (idx_ctx) {
    // clean up any old allocations
    clean_up_contexts();
  }
  idx_ctx = create_idx_context(1);

  writer_context.reset(new S3AsyncMotrKVSWriterContext(
      request, std::bind(&S3MotrKVSWriter::put_keyval_successful, this),
      std::bind(&S3MotrKVSWriter::put_keyval_failed, this), 1, s3_motr_api));

  writer_context->init_kvs_write_op_ctx(kv_list.size());

  // Ret code can be ignored as its already handled in async case.
  put_keyval_impl(kv_list, true);
}

int S3MotrKVSWriter::put_keyval_impl(
    const std::map<std::string, std::string> &kv_list, bool is_async) {

  int rc = 0;
  struct s3_motr_idx_op_context *idx_op_ctx = NULL;
  struct s3_motr_kvs_op_context *kvs_ctx = NULL;
  struct s3_motr_context_obj *op_ctx = NULL;

  if (is_async) {

    idx_op_ctx = writer_context->get_motr_idx_op_ctx();
    kvs_ctx = writer_context->get_motr_kvs_op_ctx();
    op_ctx = (struct s3_motr_context_obj *)calloc(
        1, sizeof(struct s3_motr_context_obj));

    op_ctx->op_index_in_launch = 0;
    op_ctx->application_context = (void *)writer_context.get();

    // Operation callbacks are used in async mode.
    if (idx_op_ctx->cbs->oop_executed) {
      idx_op_ctx->cbs->oop_executed = NULL;
    }
    idx_op_ctx->cbs->oop_stable = s3_motr_op_stable;
    idx_op_ctx->cbs->oop_failed = s3_motr_op_failed;

  } else {

    idx_op_ctx = sync_writer_context->get_motr_idx_op_ctx();
    kvs_ctx = sync_writer_context->get_motr_kvs_op_ctx();
    op_ctx = (struct s3_motr_context_obj *)calloc(
        1, sizeof(struct s3_motr_context_obj));

    op_ctx->op_index_in_launch = 0;
    op_ctx->application_context = (void *)sync_writer_context.get();
  }

  size_t i = 0;
  for (const auto &kv : kv_list) {
    set_up_key_value_store(kvs_ctx, kv.first, kv.second, i);
    ++i;
  }
  s3_motr_api->motr_idx_init(&(idx_ctx->idx[0]), &motr_container.co_realm,
                             &idx_los[0].oid);
  idx_ctx->n_initialized_contexts = 1;

  idx_ctx->idx->in_attr.idx_pver = idx_los[0].pver;
  idx_ctx->idx->in_attr.idx_layout_type = idx_los[0].layout_type;

  rc = s3_motr_api->motr_idx_op(
      &(idx_ctx->idx[0]), M0_IC_PUT, kvs_ctx->keys, kvs_ctx->values,
      kvs_ctx->rcs, M0_OIF_OVERWRITE | M0_OIF_SYNC_WAIT, &(idx_op_ctx->ops[0]));
  if (rc != 0) {
    s3_log(S3_LOG_ERROR, request_id, "m0_idx_op failed\n");
    state = S3MotrKVSWriterOpState::failed_to_launch;
    s3_motr_op_pre_launch_failure(op_ctx->application_context, rc);
    return rc;
  } else {
    s3_log(S3_LOG_DEBUG, request_id, "m0_idx_op suceeded\n");
  }

  idx_op_ctx->ops[0]->op_datum = (void *)op_ctx;
  s3_motr_api->motr_op_setup(idx_op_ctx->ops[0], idx_op_ctx->cbs, 0);

  s3_log(S3_LOG_DEBUG, request_id, "motr_op_setup done\n");

  if (is_async) {

    writer_context->start_timer_for("put_keyval");
  }

  s3_motr_api->motr_op_launch(
      (is_async ? request->addb_request_id : S3_ADDB_STARTUP_REQUESTS_ID),
      &(idx_op_ctx->ops[0]), 1, MotrOpType::putkv);
  global_motr_idx_ops_list.insert(idx_op_ctx);
  if (!is_async) {
    s3_log(S3_LOG_DEBUG, request_id, "Waiting for motr put KV to complete\n");
    rc = s3_motr_api->motr_op_wait(
        (idx_op_ctx->ops[0]), M0_BITS(M0_OS_FAILED, M0_OS_STABLE),
        m0_time_from_now(S3Option::get_instance()->get_motr_op_wait_period(),
                         0));
    if (rc < 0) {
      return rc;
    }
  }

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
  return 0;
}

int S3MotrKVSWriter::put_keyval_sync(
    const struct s3_motr_idx_layout &idx_lo,
    const std::map<std::string, std::string> &kv_list) {
  s3_log(S3_LOG_INFO, stripped_request_id,
         "%s Entry with oid = %" SCNx64 " : %" SCNx64
         " and %zu key/value pairs\n",
         __func__, idx_lo.oid.u_hi, idx_lo.oid.u_lo, kv_list.size());

  /* For the moment sync kvs operation for fake kvs is not supported */
  assert(S3Option::get_instance()->is_sync_kvs_allowed());

  {
    for (const auto &kv : kv_list) {
      s3_log(S3_LOG_DEBUG, request_id, "key = %s, value = %s\n",
             kv.first.c_str(), kv.second.c_str());
    }
  }
  idx_los.clear();
  idx_los.push_back(idx_lo);

  if (idx_ctx) {
    // clean up any old allocations
    clean_up_contexts();
  }
  idx_ctx = create_idx_context(1);

  sync_writer_context.reset(new S3SyncMotrKVSWriterContext(request_id, 1));

  sync_writer_context->init_kvs_write_op_ctx(kv_list.size());

  return put_keyval_impl(kv_list, false);
}

void S3MotrKVSWriter::put_keyval(const struct s3_motr_idx_layout &idx_lo,
                                 const std::string &key, const std::string &val,
                                 std::function<void(void)> on_success,
                                 std::function<void(void)> on_failed) {
  s3_log(S3_LOG_INFO, stripped_request_id,
         "%s Entry with oid = %" SCNx64 " : %" SCNx64
         " key = %s and value = %s\n",
         __func__, idx_lo.oid.u_hi, idx_lo.oid.u_lo, key.c_str(), val.c_str());

  assert(!key.empty());
  // Add error when any key is empty
  if (key.empty()) {
    s3_log(S3_LOG_ERROR, request_id, "Empty key in PUT KV\n");
  }
  int rc = 0;
  idx_los.clear();
  idx_los.push_back(idx_lo);

  this->handler_on_success = std::move(on_success);
  this->handler_on_failed = std::move(on_failed);

  if (idx_ctx) {
    // clean up any old allocations
    clean_up_contexts();
  }
  idx_ctx = create_idx_context(1);

  writer_context.reset(new S3AsyncMotrKVSWriterContext(
      request, std::bind(&S3MotrKVSWriter::put_keyval_successful, this),
      std::bind(&S3MotrKVSWriter::put_keyval_failed, this), 1, s3_motr_api));

  // Only one key value passed
  writer_context->init_kvs_write_op_ctx(1);

  struct s3_motr_idx_op_context *idx_op_ctx =
      writer_context->get_motr_idx_op_ctx();
  struct s3_motr_kvs_op_context *kvs_ctx =
      writer_context->get_motr_kvs_op_ctx();

  struct s3_motr_context_obj *op_ctx = (struct s3_motr_context_obj *)calloc(
      1, sizeof(struct s3_motr_context_obj));

  op_ctx->op_index_in_launch = 0;
  op_ctx->application_context = (void *)writer_context.get();

  idx_op_ctx->cbs->oop_executed = NULL;
  idx_op_ctx->cbs->oop_stable = s3_motr_op_stable;
  idx_op_ctx->cbs->oop_failed = s3_motr_op_failed;

  set_up_key_value_store(kvs_ctx, key, val);

  s3_motr_api->motr_idx_init(&(idx_ctx->idx[0]), &motr_container.co_realm,
                             &idx_los[0].oid);
  idx_ctx->n_initialized_contexts = 1;

  idx_ctx->idx->in_attr.idx_pver = idx_los[0].pver;
  idx_ctx->idx->in_attr.idx_layout_type = idx_los[0].layout_type;

  rc = s3_motr_api->motr_idx_op(
      &(idx_ctx->idx[0]), M0_IC_PUT, kvs_ctx->keys, kvs_ctx->values,
      kvs_ctx->rcs, M0_OIF_OVERWRITE | M0_OIF_SYNC_WAIT, &(idx_op_ctx->ops[0]));
  if (rc != 0) {
    s3_log(S3_LOG_ERROR, request_id, "m0_idx_op failed\n");
    state = S3MotrKVSWriterOpState::failed_to_launch;
    s3_motr_op_pre_launch_failure(op_ctx->application_context, rc);
    return;
  } else {
    s3_log(S3_LOG_DEBUG, request_id, "m0_idx_op suceeded\n");
  }

  idx_op_ctx->ops[0]->op_datum = (void *)op_ctx;
  s3_motr_api->motr_op_setup(idx_op_ctx->ops[0], idx_op_ctx->cbs, 0);

  writer_context->start_timer_for("put_keyval");

  s3_motr_api->motr_op_launch(request->addb_request_id, idx_op_ctx->ops, 1,
                              MotrOpType::putkv);
  global_motr_idx_ops_list.insert(idx_op_ctx);
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3MotrKVSWriter::put_keyval_successful() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_stats_inc("put_keyval_success_count");
  // todo: Add check, verify if (kvs_ctx->rcs == 0)
  // do this when cassandra + motr-kvs rcs implementation completed
  // in motr
  state = S3MotrKVSWriterOpState::created;
  this->handler_on_success();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3MotrKVSWriter::put_keyval_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_log(S3_LOG_ERROR, request_id, "Writing of key value failed\n");
  if (state != S3MotrKVSWriterOpState::failed_to_launch) {
    state = S3MotrKVSWriterOpState::failed;
  }
  this->handler_on_failed();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

// Sync motr is currently done using motr_idx_op

// void S3MotrKVSWriter::sync_keyval(std::function<void(void)> on_success,
//                                     std::function<void(void)> on_failed) {
//   s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
//   int rc = 0;
//   this->handler_on_success = on_success;
//   this->handler_on_failed = on_failed;

//   sync_context.reset(new S3AsyncMotrKVSWriterContext(
//       request, std::bind(&S3MotrKVSWriter::sync_keyval_successful, this),
//       std::bind(&S3MotrKVSWriter::sync_keyval_failed, this), 1,
//       s3_motr_api));

//   struct s3_motr_idx_op_context *idx_op_ctx =
//       sync_context->get_motr_idx_op_ctx();

//   struct s3_motr_context_obj *op_ctx = (struct s3_motr_context_obj
// *)calloc(
//       1, sizeof(struct s3_motr_context_obj));

//   op_ctx->op_index_in_launch = 0;
//   op_ctx->application_context = (void *)sync_context.get();

//   idx_op_ctx->cbs->oop_executed = NULL;
//   idx_op_ctx->cbs->oop_stable = s3_motr_op_stable;
//   idx_op_ctx->cbs->oop_failed = s3_motr_op_failed;

//   rc = s3_motr_api->motr_sync_op_init(&idx_op_ctx->sync_op);
//   if (rc != 0) {
//     s3_log(S3_LOG_ERROR, request_id, "m0_sync_op_init failed\n");
//     state = S3MotrKVSWriterOpState::failed_to_launch;
//     s3_motr_op_pre_launch_failure(op_ctx->application_context, rc);
//     return;
//   }

//   rc = s3_motr_api->motr_sync_op_add(
//       idx_op_ctx->sync_op, writer_context->get_motr_idx_op_ctx()->ops[0]);
//   if (rc != 0) {
//     s3_log(S3_LOG_ERROR, request_id, "m0_sync_entity_add failed\n");
//     state = S3MotrKVSWriterOpState::failed_to_launch;
//     s3_motr_op_pre_launch_failure(op_ctx->application_context, rc);
//     return;
//   }

//   idx_op_ctx->sync_op->op_datum = (void *)op_ctx;
//   s3_motr_api->motr_op_setup(idx_op_ctx->sync_op, idx_op_ctx->cbs, 0);
//   sync_context->start_timer_for("sync_keyval_op");
//   if (state == S3MotrKVSWriterOpState::deleting) {
//     s3_motr_api->motr_op_launch(request->addb_request_id,
//                                     &idx_op_ctx->sync_op, 1,
//                                     MotrOpType::deletekv);
//   } else {
//     s3_motr_api->motr_op_launch(
//         request->addb_request_id, &idx_op_ctx->sync_op, 1,
// MotrOpType::putkv);
//   }

//   s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
// }

// void S3MotrKVSWriter::sync_keyval_successful() {
//   s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
//   s3_stats_inc("sync_keyval_op_success_count");
//   if (state == S3MotrKVSWriterOpState::deleting) {
//     state = S3MotrKVSWriterOpState::deleted;
//   } else {
//     state = S3MotrKVSWriterOpState::saved;
//   }
//   s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
// }

// void S3MotrKVSWriter::sync_keyval_failed() {
//   s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
//   if (state != S3MotrKVSWriterOpState::failed_to_launch) {
//     state = S3MotrKVSWriterOpState::failed;
//   }
//   this->handler_on_failed();
//   s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
// }

void S3MotrKVSWriter::delete_keyval(const struct s3_motr_idx_layout &idx_lo,
                                    const std::string &key,
                                    std::function<void(void)> on_success,
                                    std::function<void(void)> on_failed) {
  s3_log(S3_LOG_INFO, stripped_request_id,
         "%s Entry with oid %" SCNx64 " : %" SCNx64 " and key %s\n", __func__,
         idx_lo.oid.u_hi, idx_lo.oid.u_lo, key.c_str());

  std::vector<std::string> keys;
  keys.push_back(key);

  delete_keyval(idx_lo, keys, on_success, on_failed);
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3MotrKVSWriter::delete_keyval(const struct s3_motr_idx_layout &idx_lo,
                                    const std::vector<std::string> &keys,
                                    std::function<void(void)> on_success,
                                    std::function<void(void)> on_failed) {
  s3_log(S3_LOG_INFO, stripped_request_id,
         "%s Entry with oid %" SCNx64 " : %" SCNx64 " and %zu keys\n", __func__,
         idx_lo.oid.u_hi, idx_lo.oid.u_lo, keys.size());

  for (const auto &key : keys) {
    s3_log(S3_LOG_DEBUG, request_id, "key = %s\n", key.c_str());
    assert(!key.empty());
    // Add error when any key is empty
    if (key.empty()) {
      s3_log(S3_LOG_ERROR, request_id, "Empty key in DEL KV\n");
    }
  }
  state = S3MotrKVSWriterOpState::deleting;

  idx_los.clear();
  idx_los.push_back(idx_lo);

  keys_list = keys;

  this->handler_on_success = std::move(on_success);
  this->handler_on_failed = std::move(on_failed);

  if (idx_ctx) {
    // clean up any old allocations
    clean_up_contexts();
  }
  idx_ctx = create_idx_context(1);

  writer_context.reset(new S3AsyncMotrKVSWriterContext(
      request, std::bind(&S3MotrKVSWriter::delete_keyval_successful, this),
      std::bind(&S3MotrKVSWriter::delete_keyval_failed, this), 1, s3_motr_api));

  // Only one key value passed
  writer_context->init_kvs_write_op_ctx(keys_list.size());

  struct s3_motr_idx_op_context *idx_op_ctx =
      writer_context->get_motr_idx_op_ctx();
  struct s3_motr_kvs_op_context *kvs_ctx =
      writer_context->get_motr_kvs_op_ctx();

  struct s3_motr_context_obj *op_ctx = (struct s3_motr_context_obj *)calloc(
      1, sizeof(struct s3_motr_context_obj));

  op_ctx->op_index_in_launch = 0;
  op_ctx->application_context = (void *)writer_context.get();

  idx_op_ctx->cbs->oop_executed = NULL;
  idx_op_ctx->cbs->oop_stable = s3_motr_op_stable;
  idx_op_ctx->cbs->oop_failed = s3_motr_op_failed;

  int i = 0;
  for (const auto &key : keys_list) {
    kvs_ctx->keys->ov_vec.v_count[i] = key.length();
    kvs_ctx->keys->ov_buf[i] = calloc(1, key.length());
    memcpy(kvs_ctx->keys->ov_buf[i], (void *)key.c_str(), key.length());
    ++i;
  }

  s3_motr_api->motr_idx_init(&(idx_ctx->idx[0]), &motr_container.co_realm,
                             &idx_los[0].oid);
  idx_ctx->n_initialized_contexts = 1;

  idx_ctx->idx->in_attr.idx_pver = idx_los[0].pver;
  idx_ctx->idx->in_attr.idx_layout_type = idx_los[0].layout_type;

  int rc = s3_motr_api->motr_idx_op(idx_ctx->idx, M0_IC_DEL, kvs_ctx->keys,
                                    NULL, kvs_ctx->rcs, M0_OIF_SYNC_WAIT,
                                    &(idx_op_ctx->ops[0]));
  if (rc != 0) {
    s3_log(S3_LOG_ERROR, request_id, "m0_idx_op failed\n");
    state = S3MotrKVSWriterOpState::failed_to_launch;
    s3_motr_op_pre_launch_failure(op_ctx->application_context, rc);
    return;
  } else {
    s3_log(S3_LOG_DEBUG, request_id, "m0_idx_op suceeded\n");
  }

  idx_op_ctx->ops[0]->op_datum = (void *)op_ctx;
  s3_motr_api->motr_op_setup(idx_op_ctx->ops[0], idx_op_ctx->cbs, 0);

  writer_context->start_timer_for("delete_keyval");

  s3_motr_api->motr_op_launch(request->addb_request_id, idx_op_ctx->ops, 1,
                              MotrOpType::deletekv);
  global_motr_idx_ops_list.insert(idx_op_ctx);
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3MotrKVSWriter::delete_keyval_successful() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  // todo: Add check, verify if (kvs_ctx->rcs == 0)
  // do this when cassandra + motr-kvs rcs implementation completed
  // in motr
  state = S3MotrKVSWriterOpState::deleted;
  this->handler_on_success();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3MotrKVSWriter::delete_keyval_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (state != S3MotrKVSWriterOpState::failed_to_launch) {
    if (writer_context->get_errno_for(0) == -ENOENT) {
      s3_log(S3_LOG_DEBUG, request_id, "The key doesn't exist\n");
      state = S3MotrKVSWriterOpState::missing;
    } else {
      s3_log(S3_LOG_ERROR, request_id, "Deleting the value for a key failed\n");
      state = S3MotrKVSWriterOpState::failed;
    }
  }
  this->handler_on_failed();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3MotrKVSWriter::set_up_key_value_store(
    struct s3_motr_kvs_op_context *kvs_ctx, const std::string &key,
    const std::string &val, size_t pos) {
  // TODO - clean up these buffers
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);
  kvs_ctx->keys->ov_vec.v_count[pos] = key.length();
  kvs_ctx->keys->ov_buf[pos] = (char *)malloc(key.length());
  memcpy(kvs_ctx->keys->ov_buf[pos], (void *)key.c_str(), key.length());

  kvs_ctx->values->ov_vec.v_count[pos] = val.length();
  kvs_ctx->values->ov_buf[pos] = (char *)malloc(val.length());
  memcpy(kvs_ctx->values->ov_buf[pos], (void *)val.c_str(), val.length());

  s3_log(S3_LOG_DEBUG, request_id, "Keys and value in motr buffer\n");
  s3_log(S3_LOG_DEBUG, request_id, "kvs_ctx->keys->ov_buf[%lu] = %s\n", pos,
         std::string((char *)kvs_ctx->keys->ov_buf[pos],
                     kvs_ctx->keys->ov_vec.v_count[pos]).c_str());
  s3_log(S3_LOG_DEBUG, request_id, "kvs_ctx->vals->ov_buf[%lu] = %s\n", pos,
         std::string((char *)kvs_ctx->values->ov_buf[pos],
                     kvs_ctx->values->ov_vec.v_count[pos]).c_str());
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}
