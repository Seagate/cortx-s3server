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

#include <assert.h>
#include <unistd.h>

#include "s3_common.h"

#include "s3_clovis_kvs_reader.h"
#include "s3_clovis_rw_common.h"
#include "s3_option.h"
#include "s3_uri_to_mero_oid.h"

extern struct m0_clovis_realm clovis_uber_realm;
extern struct m0_clovis_container clovis_container;

S3ClovisKVSReader::S3ClovisKVSReader(std::shared_ptr<S3RequestObject> req,
                                     std::shared_ptr<ClovisAPI> s3clovis_api)
    : request(req),
      s3_clovis_api(s3clovis_api),
      state(S3ClovisKVSReaderOpState::start),
      last_value(""),
      iteration_index(0) {
  s3_log(S3_LOG_DEBUG, "Constructor\n");
  last_result_keys_values.clear();
}

//
// Commenting this function
// get_keyval will be issued with index's oid, at top level if the oid is 0 then
// it will be
// interpreted as there is no object available
// void S3ClovisKVSReader::get_keyval(std::string index_name, std::string key,
// std::function<void(void)> on_success, std::function<void(void)> on_failed) {
//  std::vector<std::string> keys;
//  keys.push_back(key);

//  get_keyval(index_name, keys, on_success, on_failed);
//}

void S3ClovisKVSReader::get_keyval(struct m0_uint128 oid, std::string key,
                                   std::function<void(void)> on_success,
                                   std::function<void(void)> on_failed) {
  std::vector<std::string> keys;
  keys.push_back(key);

  get_keyval(oid, keys, on_success, on_failed);
}

//
// Commenting this function
// get_keyval will be issued with index's oid, at top level if the oid is 0 then
// it will be
// interpreted as there is no object available
//
// void S3ClovisKVSReader::get_keyval(std::string index_name,
// std::vector<std::string> keys, std::function<void(void)> on_success,
// std::function<void(void)> on_failed) {
//  s3_log(S3_LOG_DEBUG, "Entering\n");
//  s3_log(S3_LOG_DEBUG, "index_name = %s\n", index_name.c_str());
//  S3UriToMeroOID(index_name.c_str(), &id, S3ClovisEntityType::index);
//  get_keyval(id, keys, on_success, on_failed);
//}

void S3ClovisKVSReader::get_keyval(struct m0_uint128 oid,
                                   std::vector<std::string> keys,
                                   std::function<void(void)> on_success,
                                   std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  for (auto key : keys) {
    s3_log(S3_LOG_DEBUG, "key = %s\n", key.c_str());
  }

  id = oid;

  int rc = 0;
  last_result_keys_values.clear();
  last_value = "";

  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;

  reader_context.reset(new S3ClovisKVSReaderContext(
      request, std::bind(&S3ClovisKVSReader::get_keyval_successful, this),
      std::bind(&S3ClovisKVSReader::get_keyval_failed, this)));

  reader_context->init_kvs_read_op_ctx(keys.size());

  struct s3_clovis_idx_op_context *idx_ctx =
      reader_context->get_clovis_idx_op_ctx();
  struct s3_clovis_kvs_op_context *kvs_ctx =
      reader_context->get_clovis_kvs_op_ctx();

  // Remember, so buffers can be iterated.
  clovis_kvs_op_context = kvs_ctx;

  struct s3_clovis_context_obj *op_ctx = (struct s3_clovis_context_obj *)calloc(
      1, sizeof(struct s3_clovis_context_obj));

  op_ctx->op_index_in_launch = 0;
  op_ctx->application_context = (void *)reader_context.get();

  idx_ctx->cbs->oop_executed = NULL;
  idx_ctx->cbs->oop_stable = s3_clovis_op_stable;
  idx_ctx->cbs->oop_failed = s3_clovis_op_failed;

  int i = 0;
  for (auto key : keys) {
    kvs_ctx->keys->ov_vec.v_count[i] = key.length();
    kvs_ctx->keys->ov_buf[i] = malloc(key.length());
    memcpy(kvs_ctx->keys->ov_buf[i], (void *)key.c_str(), key.length());
    ++i;
  }

  s3_clovis_api->clovis_idx_init(idx_ctx->idx, &clovis_container.co_realm, &id);

  rc = s3_clovis_api->clovis_idx_op(idx_ctx->idx, M0_CLOVIS_IC_GET,
                                    kvs_ctx->keys, kvs_ctx->values,
                                    kvs_ctx->rcs, 0, &(idx_ctx->ops[0]));
  if (rc != 0) {
    s3_log(S3_LOG_ERROR, "m0_clovis_idx_op failed\n");
  } else {
    s3_log(S3_LOG_DEBUG, "m0_clovis_idx_op suceeded\n");
  }

  idx_ctx->ops[0]->op_datum = (void *)op_ctx;
  s3_clovis_api->clovis_op_setup(idx_ctx->ops[0], idx_ctx->cbs, 0);

  reader_context->start_timer_for("get_keyval");

  s3_clovis_api->clovis_op_launch(idx_ctx->ops, 1, ClovisOpType::getkv);
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ClovisKVSReader::get_keyval_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  state = S3ClovisKVSReaderOpState::present;
  // remember the response
  struct s3_clovis_kvs_op_context *kvs_ctx =
      reader_context->get_clovis_kvs_op_ctx();
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
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ClovisKVSReader::get_keyval_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (reader_context->get_errno_for(0) == -ENOENT) {
    s3_log(S3_LOG_DEBUG, "The key doesn't exist\n");
    state = S3ClovisKVSReaderOpState::missing;
  } else {
    s3_log(S3_LOG_ERROR, "Getting the value for a key failed\n");
    state = S3ClovisKVSReaderOpState::failed;
  }
  this->handler_on_failed();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

// NOTE: https://jts.seagate.com/browse/MERO-2309 tracks changes planned to
// M0_CLOVIS_IC_NEXT to allow skipping rquested key in reply list. This would
// affect all call sites and should be verified for corner cases when integrated
void S3ClovisKVSReader::next_keyval(struct m0_uint128 idx_oid, std::string key,
                                    size_t nr_kvp,
                                    std::function<void(void)> on_success,
                                    std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  s3_log(S3_LOG_DEBUG, "key = %s and count = %zu\n", key.c_str(), nr_kvp);
  id = idx_oid;
  int rc = 0;
  last_result_keys_values.clear();

  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;

  reader_context.reset(new S3ClovisKVSReaderContext(
      request, std::bind(&S3ClovisKVSReader::next_keyval_successful, this),
      std::bind(&S3ClovisKVSReader::next_keyval_failed, this)));

  reader_context->init_kvs_read_op_ctx(nr_kvp);

  struct s3_clovis_idx_op_context *idx_ctx =
      reader_context->get_clovis_idx_op_ctx();
  struct s3_clovis_kvs_op_context *kvs_ctx =
      reader_context->get_clovis_kvs_op_ctx();

  // Remember, so buffers can be iterated.
  clovis_kvs_op_context = kvs_ctx;

  struct s3_clovis_context_obj *op_ctx = (struct s3_clovis_context_obj *)calloc(
      1, sizeof(struct s3_clovis_context_obj));

  op_ctx->op_index_in_launch = 0;
  op_ctx->application_context = (void *)reader_context.get();

  idx_ctx->cbs->oop_executed = NULL;
  idx_ctx->cbs->oop_stable = s3_clovis_op_stable;
  idx_ctx->cbs->oop_failed = s3_clovis_op_failed;

  if (key.empty()) {
    kvs_ctx->keys->ov_vec.v_count[0] = 0;
    kvs_ctx->keys->ov_buf[0] = NULL;
  } else {
    kvs_ctx->keys->ov_vec.v_count[0] = key.length();
    kvs_ctx->keys->ov_buf[0] = malloc(key.length());
    memcpy(kvs_ctx->keys->ov_buf[0], (void *)key.c_str(), key.length());
  }

  s3_clovis_api->clovis_idx_init(idx_ctx->idx, &clovis_container.co_realm,
                                 &idx_oid);
  rc = s3_clovis_api->clovis_idx_op(idx_ctx->idx, M0_CLOVIS_IC_NEXT,
                                    kvs_ctx->keys, kvs_ctx->values,
                                    kvs_ctx->rcs, 0, &(idx_ctx->ops[0]));
  if (rc != 0) {
    s3_log(S3_LOG_ERROR, "m0_clovis_idx_op failed\n");
  } else {
    s3_log(S3_LOG_DEBUG, "m0_clovis_idx_op suceeded\n");
  }

  idx_ctx->ops[0]->op_datum = (void *)op_ctx;
  s3_clovis_api->clovis_op_setup(idx_ctx->ops[0], idx_ctx->cbs, 0);

  reader_context->start_timer_for("get_keyval");

  s3_clovis_api->clovis_op_launch(idx_ctx->ops, 1, ClovisOpType::getkv);
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ClovisKVSReader::next_keyval(std::string index_name, std::string key,
                                    size_t nr_kvp,
                                    std::function<void(void)> on_success,
                                    std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  s3_log(S3_LOG_DEBUG, "Index_name = %s, key = %s and count = %zu\n",
         index_name.c_str(), key.c_str(), nr_kvp);

  S3UriToMeroOID(index_name.c_str(), &id, S3ClovisEntityType::index);

  next_keyval(id, key, nr_kvp, on_success, on_failed);

  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ClovisKVSReader::next_keyval_successful() {
  s3_log(S3_LOG_DEBUG, "S3ClovisKVSReader::next_keyval_successful called\n");
  state = S3ClovisKVSReaderOpState::present;
  // remember the response
  struct s3_clovis_kvs_op_context *kvs_ctx =
      reader_context->get_clovis_kvs_op_ctx();
  std::string key;
  std::string val;
  for (size_t i = 0; i < kvs_ctx->keys->ov_vec.v_nr; i++) {
    if (kvs_ctx->keys->ov_buf[i] == NULL) {
      break;
    }
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
  if (last_result_keys_values.empty()) {
    // no keys retrieved
    reader_context->set_op_errno_for(0, -ENOENT);
    reader_context->set_op_status_for(0, S3AsyncOpStatus::failed,
                                      "Operation Failed.");
    next_keyval_failed();
  } else {
    // at least one key successfully retrieved
    this->handler_on_success();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ClovisKVSReader::next_keyval_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (reader_context->get_errno_for(0) == -ENOENT) {
    s3_log(S3_LOG_DEBUG, "The key doesn't exist in metadata\n");
    state = S3ClovisKVSReaderOpState::missing;
  } else {
    s3_log(S3_LOG_ERROR, "fetching of next set of key values failed\n");
    state = S3ClovisKVSReaderOpState::failed;
  }
  this->handler_on_failed();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}
