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

#include <assert.h>
#include <unistd.h>

#include "s3_common.h"

#include "s3_motr_kvs_reader.h"
#include "s3_motr_rw_common.h"
#include "s3_option.h"
#include "s3_uri_to_motr_oid.h"
#include "s3_stats.h"

extern struct m0_realm motr_uber_realm;
extern struct m0_container motr_container;
extern std::set<struct s3_motr_idx_op_context *> global_motr_idx_ops_list;
extern std::set<struct s3_motr_idx_context *> global_motr_idx;
extern int shutdown_motr_teardown_called;

S3MotrKVSReader::S3MotrKVSReader(std::shared_ptr<RequestObject> req,
                                 std::shared_ptr<MotrAPI> motr_api)
    : request(req),
      state(S3MotrKVSReaderOpState::start),
      last_value(""),
      iteration_index(0),
      idx_ctx(nullptr),
      key_ref_copy(nullptr) {
  request_id = request->get_request_id();
  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");
  last_result_keys_values.clear();
  if (motr_api) {
    s3_motr_api = motr_api;
  } else {
    s3_motr_api = std::make_shared<ConcreteMotrAPI>();
  }
}

S3MotrKVSReader::~S3MotrKVSReader() { clean_up_contexts(); }

void S3MotrKVSReader::clean_up_contexts() {
  reader_context = nullptr;
  if (!shutdown_motr_teardown_called) {
    global_motr_idx.erase(idx_ctx);
    if (idx_ctx) {
      for (size_t i = 0; i < idx_ctx->n_initialized_contexts; i++) {
        s3_motr_api->motr_idx_fini(&idx_ctx->idx[i]);
      }
      free_idx_context(idx_ctx);
      idx_ctx = nullptr;
    }
  }
}

void S3MotrKVSReader::get_keyval(struct m0_uint128 oid, std::string key,
                                 std::function<void(void)> on_success,
                                 std::function<void(void)> on_failed) {
  std::vector<std::string> keys;
  keys.push_back(key);

  get_keyval(oid, keys, on_success, on_failed);
}

void S3MotrKVSReader::get_keyval(struct m0_uint128 oid,
                                 std::vector<std::string> keys,
                                 std::function<void(void)> on_success,
                                 std::function<void(void)> on_failed) {
  int rc = 0;
  s3_log(S3_LOG_INFO, request_id,
         "Entering with oid %" SCNx64 " : %" SCNx64 " and %zu keys\n", oid.u_hi,
         oid.u_lo, keys.size());
  for (auto key : keys) {
    s3_log(S3_LOG_DEBUG, request_id, "key = %s\n", key.c_str());
  }

  id = oid;

  last_result_keys_values.clear();
  last_value = "";

  if (idx_ctx) {
    // clean up any old allocations
    clean_up_contexts();
  }
  idx_ctx = create_idx_context(1);

  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;

  reader_context.reset(new S3MotrKVSReaderContext(
      request, std::bind(&S3MotrKVSReader::get_keyval_successful, this),
      std::bind(&S3MotrKVSReader::get_keyval_failed, this), s3_motr_api));

  reader_context->init_kvs_read_op_ctx(keys.size());

  struct s3_motr_idx_op_context *idx_op_ctx =
      reader_context->get_motr_idx_op_ctx();
  struct s3_motr_kvs_op_context *kvs_ctx =
      reader_context->get_motr_kvs_op_ctx();

  // Remember, so buffers can be iterated.
  motr_kvs_op_context = kvs_ctx;

  struct s3_motr_context_obj *op_ctx = (struct s3_motr_context_obj *)calloc(
      1, sizeof(struct s3_motr_context_obj));

  op_ctx->op_index_in_launch = 0;
  op_ctx->application_context = (void *)reader_context.get();

  idx_op_ctx->cbs->oop_executed = NULL;
  idx_op_ctx->cbs->oop_stable = s3_motr_op_stable;
  idx_op_ctx->cbs->oop_failed = s3_motr_op_failed;

  int i = 0;
  for (auto key : keys) {
    kvs_ctx->keys->ov_vec.v_count[i] = key.length();
    kvs_ctx->keys->ov_buf[i] = malloc(key.length());
    memcpy(kvs_ctx->keys->ov_buf[i], (void *)key.c_str(), key.length());
    ++i;
  }

  s3_motr_api->motr_idx_init(&idx_ctx->idx[0], &motr_container.co_realm, &id);
  idx_ctx->n_initialized_contexts = 1;

  rc = s3_motr_api->motr_idx_op(&idx_ctx->idx[0], M0_IC_GET, kvs_ctx->keys,
                                kvs_ctx->values, kvs_ctx->rcs, 0,
                                &(idx_op_ctx->ops[0]));
  if (rc != 0) {
    s3_log(S3_LOG_ERROR, request_id, "m0_idx_op failed\n");
    state = S3MotrKVSReaderOpState::failed_to_launch;
    s3_motr_op_pre_launch_failure(op_ctx->application_context, rc);
    return;
  } else {
    s3_log(S3_LOG_DEBUG, request_id, "m0_idx_op suceeded\n");
  }

  idx_op_ctx->ops[0]->op_datum = (void *)op_ctx;
  s3_motr_api->motr_op_setup(idx_op_ctx->ops[0], idx_op_ctx->cbs, 0);

  reader_context->start_timer_for("get_keyval");

  s3_motr_api->motr_op_launch(request->addb_request_id, idx_op_ctx->ops, 1,
                              MotrOpType::getkv);
  global_motr_idx_ops_list.insert(idx_op_ctx);
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  return;
}

void S3MotrKVSReader::lookup_index(struct m0_uint128 oid,
                                   std::function<void(void)> on_success,
                                   std::function<void(void)> on_failed) {
  int rc = 0;
  s3_log(S3_LOG_INFO, request_id,
         "Entering with oid %" SCNx64 " : %" SCNx64 "\n", oid.u_hi, oid.u_lo);

  id = oid;

  if (idx_ctx) {
    // clean up any old allocations
    clean_up_contexts();
  }
  idx_ctx = create_idx_context(1);

  this->handler_on_success = std::move(on_success);
  this->handler_on_failed = std::move(on_failed);

  reader_context.reset(new S3MotrKVSReaderContext(
      request, std::bind(&S3MotrKVSReader::lookup_index_successful, this),
      std::bind(&S3MotrKVSReader::lookup_index_failed, this), s3_motr_api));

  struct s3_motr_idx_op_context *idx_op_ctx =
      reader_context->get_motr_idx_op_ctx();

  struct s3_motr_context_obj *op_ctx = (struct s3_motr_context_obj *)calloc(
      1, sizeof(struct s3_motr_context_obj));

  // op_ctx->op_index_in_launch = 0;
  op_ctx->application_context = (void *)reader_context.get();

  if (idx_op_ctx && idx_op_ctx->cbs) {
    idx_op_ctx->cbs->oop_executed = NULL;
    idx_op_ctx->cbs->oop_stable = s3_motr_op_stable;
    idx_op_ctx->cbs->oop_failed = s3_motr_op_failed;
  }

  s3_motr_api->motr_idx_init(&idx_ctx->idx[0], &motr_container.co_realm, &id);
  idx_ctx->n_initialized_contexts = 1;

  rc = s3_motr_api->motr_idx_op(&idx_ctx->idx[0], M0_IC_LOOKUP, NULL, NULL,
                                NULL, 0, &(idx_op_ctx->ops[0]));
  if (rc != 0) {
    s3_log(S3_LOG_ERROR, request_id, "m0_idx_op failed\n");
    state = S3MotrKVSReaderOpState::failed_to_launch;
    s3_motr_op_pre_launch_failure(op_ctx->application_context, rc);
    return;
  } else {
    s3_log(S3_LOG_DEBUG, request_id, "m0_idx_op suceeded\n");
  }

  idx_op_ctx->ops[0]->op_datum = (void *)op_ctx;
  s3_motr_api->motr_op_setup(idx_op_ctx->ops[0], idx_op_ctx->cbs, 0);

  reader_context->start_timer_for("lookup_index");

  s3_motr_api->motr_op_launch(request->addb_request_id, idx_op_ctx->ops, 1,
                              MotrOpType::headidx);
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  return;
}

void S3MotrKVSReader::lookup_index_successful() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  state = S3MotrKVSReaderOpState::present;
  this->handler_on_success();
}

void S3MotrKVSReader::lookup_index_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (state != S3MotrKVSReaderOpState::failed_to_launch) {
    if (reader_context->get_errno_for(0) == -ENOENT) {
      s3_log(S3_LOG_DEBUG, request_id, "The index id doesn't exist\n");
      state = S3MotrKVSReaderOpState::missing;
    } else {
      s3_log(S3_LOG_ERROR, request_id, "index lookup failed\n");
      state = S3MotrKVSReaderOpState::failed;
    }
  }
  this->handler_on_failed();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3MotrKVSReader::get_keyval_successful() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  s3_stats_inc("get_keyval_success_count");
  state = S3MotrKVSReaderOpState::present;
  // remember the response
  struct s3_motr_kvs_op_context *kvs_ctx =
      reader_context->get_motr_kvs_op_ctx();
  int rcs;
  std::string key;
  std::string val;
  bool keys_retrieved = false;
  for (size_t i = 0; i < kvs_ctx->keys->ov_vec.v_nr; i++) {
    assert(kvs_ctx->keys->ov_buf[i] != NULL);
    key = std::string((char *)kvs_ctx->keys->ov_buf[i],
                      kvs_ctx->keys->ov_vec.v_count[i]);
    if (kvs_ctx->rcs[i] == 0) {
      rcs = 0;
      keys_retrieved = true;  // atleast one key successfully retrieved
      if (kvs_ctx->values->ov_buf[i] != NULL) {
        val = std::string((char *)kvs_ctx->values->ov_buf[i],
                          kvs_ctx->values->ov_vec.v_count[i]);
      } else {
        val = "";
      }
    } else {
      rcs = kvs_ctx->rcs[i];
      val = "";
    }
    last_result_keys_values[key] = std::make_pair(rcs, val);
  }
  if (kvs_ctx->keys->ov_vec.v_nr == 1) {
    last_value = val;
  }
  if (keys_retrieved) {
    // at least one key successfully retrieved
    this->handler_on_success();
  } else {
    // no keys successfully retrieved
    reader_context->set_op_errno_for(0, -ENOENT);
    reader_context->set_op_status_for(0, S3AsyncOpStatus::failed,
                                      "Operation Failed.");
    get_keyval_failed();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3MotrKVSReader::get_keyval_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (state != S3MotrKVSReaderOpState::failed_to_launch) {
    if (reader_context->get_errno_for(0) == -ENOENT) {
      s3_log(S3_LOG_DEBUG, request_id, "The key doesn't exist\n");
      state = S3MotrKVSReaderOpState::missing;
    } else if (reader_context->get_errno_for(0) == -E2BIG) {
      s3_log(S3_LOG_WARN, request_id,
             "Motr failed to retrieve metadata as RPC threshold exceeded\n");
      state = S3MotrKVSReaderOpState::failed_e2big;
    } else {
      s3_log(S3_LOG_ERROR, request_id, "Getting the value for a key failed\n");
      state = S3MotrKVSReaderOpState::failed;
    }
  }
  this->handler_on_failed();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3MotrKVSReader::next_keyval(struct m0_uint128 idx_oid, std::string key,
                                  size_t nr_kvp,
                                  std::function<void(void)> on_success,
                                  std::function<void(void)> on_failed,
                                  unsigned int flag) {
  s3_log(S3_LOG_INFO, request_id, "Entering with idx_oid = %" SCNx64
                                  " : %" SCNx64 " key = %s and count = %zu\n",
         idx_oid.u_hi, idx_oid.u_lo, key.c_str(), nr_kvp);
  id = idx_oid;
  int rc = 0;
  last_result_keys_values.clear();

  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;

  if (idx_ctx) {
    // clean up any old allocations
    clean_up_contexts();
  }
  idx_ctx = create_idx_context(1);

  reader_context.reset(new S3MotrKVSReaderContext(
      request, std::bind(&S3MotrKVSReader::next_keyval_successful, this),
      std::bind(&S3MotrKVSReader::next_keyval_failed, this), s3_motr_api));

  reader_context->init_kvs_read_op_ctx(nr_kvp);

  struct s3_motr_idx_op_context *idx_op_ctx =
      reader_context->get_motr_idx_op_ctx();
  struct s3_motr_kvs_op_context *kvs_ctx =
      reader_context->get_motr_kvs_op_ctx();

  // Remember, so buffers can be iterated.
  motr_kvs_op_context = kvs_ctx;

  struct s3_motr_context_obj *op_ctx = (struct s3_motr_context_obj *)calloc(
      1, sizeof(struct s3_motr_context_obj));

  op_ctx->op_index_in_launch = 0;
  op_ctx->application_context = (void *)reader_context.get();

  idx_op_ctx->cbs->oop_executed = NULL;
  idx_op_ctx->cbs->oop_stable = s3_motr_op_stable;
  idx_op_ctx->cbs->oop_failed = s3_motr_op_failed;

  if (key.empty()) {
    kvs_ctx->keys->ov_vec.v_count[0] = 0;
    kvs_ctx->keys->ov_buf[0] = NULL;
  } else {
    kvs_ctx->keys->ov_vec.v_count[0] = key.length();
    key_ref_copy = kvs_ctx->keys->ov_buf[0] = malloc(key.length());
    memcpy(kvs_ctx->keys->ov_buf[0], (void *)key.c_str(), key.length());
  }

  s3_motr_api->motr_idx_init(&idx_ctx->idx[0], &motr_container.co_realm,
                             &idx_oid);
  idx_ctx->n_initialized_contexts = 1;

  rc = s3_motr_api->motr_idx_op(&idx_ctx->idx[0], M0_IC_NEXT, kvs_ctx->keys,
                                kvs_ctx->values, kvs_ctx->rcs, flag,
                                &(idx_op_ctx->ops[0]));
  if (rc != 0) {
    s3_log(S3_LOG_ERROR, request_id, "m0_idx_op failed\n");
    state = S3MotrKVSReaderOpState::failed_to_launch;
    s3_motr_op_pre_launch_failure(op_ctx->application_context, rc);
    return;
  } else {
    s3_log(S3_LOG_DEBUG, request_id, "m0_idx_op suceeded\n");
  }

  idx_op_ctx->ops[0]->op_datum = (void *)op_ctx;
  s3_motr_api->motr_op_setup(idx_op_ctx->ops[0], idx_op_ctx->cbs, 0);

  reader_context->start_timer_for("get_keyval");

  s3_motr_api->motr_op_launch(request->addb_request_id, idx_op_ctx->ops, 1,
                              MotrOpType::getkv);
  global_motr_idx_ops_list.insert(idx_op_ctx);
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  return;
}

void S3MotrKVSReader::next_keyval_successful() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  state = S3MotrKVSReaderOpState::present;

  // remember the response
  struct s3_motr_kvs_op_context *kvs_ctx =
      reader_context->get_motr_kvs_op_ctx();

  std::string key;
  std::string val;
  for (size_t i = 0; i < kvs_ctx->keys->ov_vec.v_nr; i++) {
    if (kvs_ctx->keys->ov_buf[i] == NULL) {
      break;
    }
    if (kvs_ctx->rcs[i] == 0) {
      key = std::string((char *)kvs_ctx->keys->ov_buf[i],
                        kvs_ctx->keys->ov_vec.v_count[i]);
      if (kvs_ctx->values->ov_buf[i] != NULL) {
        val = std::string((char *)kvs_ctx->values->ov_buf[i],
                          kvs_ctx->values->ov_vec.v_count[i]);
      } else {
        val = "";
      }
      last_result_keys_values[key] = std::make_pair(0, val);
    }
  }
  if (last_result_keys_values.empty()) {
    // no keys retrieved
    reader_context->set_op_errno_for(0, -ENOENT);
    reader_context->set_op_status_for(0, S3AsyncOpStatus::failed,
                                      "Operation Failed.");
    next_keyval_failed();
  } else {
    // at least one key successfully retrieved
    if (key_ref_copy) {
      free(key_ref_copy);
      key_ref_copy = nullptr;
    }
    this->handler_on_success();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3MotrKVSReader::next_keyval_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (state != S3MotrKVSReaderOpState::failed_to_launch) {
    if (reader_context->get_errno_for(0) == -ENOENT) {
      s3_log(S3_LOG_DEBUG, request_id, "The key doesn't exist in metadata\n");
      state = S3MotrKVSReaderOpState::missing;
    } else if (reader_context->get_errno_for(0) == -E2BIG) {
      s3_log(S3_LOG_WARN, request_id,
             "Motr failed to retrieve metadata as RPC threshold exceeded\n");
      state = S3MotrKVSReaderOpState::failed_e2big;
    } else {
      s3_log(S3_LOG_ERROR, request_id,
             "fetching of next set of key values failed\n");
      state = S3MotrKVSReaderOpState::failed;
    }
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  this->handler_on_failed();
}
