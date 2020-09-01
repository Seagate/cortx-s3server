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

#pragma once

#ifndef __S3_SERVER_S3_MOTR_KVS_WRITER_H__
#define __S3_SERVER_S3_MOTR_KVS_WRITER_H__

#include <gtest/gtest_prod.h>
#include <functional>
#include <memory>

#include "s3_asyncop_context_base.h"
#include "s3_motr_context.h"
#include "s3_motr_wrapper.h"
#include "s3_log.h"
#include "s3_request_object.h"

class S3SyncMotrKVSWriterContext {
  // Basic Operation context.
  struct s3_motr_idx_op_context* motr_idx_op_context;
  bool has_motr_idx_op_context;

  // Read/Write Operation context.
  struct s3_motr_kvs_op_context* motr_kvs_op_context;
  bool has_motr_kvs_op_context;

  std::string request_id;
  int ops_count = 1;

 public:
  S3SyncMotrKVSWriterContext(std::string req_id, int ops_cnt)
      : request_id(std::move(req_id)), ops_count(ops_cnt) {

    s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");
    // Create or write, we need op context
    motr_idx_op_context = create_basic_idx_op_ctx(ops_count);
    has_motr_idx_op_context = true;
    motr_kvs_op_context = NULL;
    has_motr_kvs_op_context = false;
  }

  ~S3SyncMotrKVSWriterContext() {
    s3_log(S3_LOG_DEBUG, request_id, "Destructor\n");
    if (has_motr_idx_op_context) {
      free_basic_idx_op_ctx(motr_idx_op_context);
    }
    if (has_motr_kvs_op_context) {
      free_basic_kvs_op_ctx(motr_kvs_op_context);
    }
  }

  struct s3_motr_idx_op_context* get_motr_idx_op_ctx() {
    return motr_idx_op_context;
  }

  // Call this when you want to do write op.
  void init_kvs_write_op_ctx(int no_of_keys) {
    motr_kvs_op_context = create_basic_kvs_op_ctx(no_of_keys);
    has_motr_kvs_op_context = true;
  }

  struct s3_motr_kvs_op_context* get_motr_kvs_op_ctx() {
    return motr_kvs_op_context;
  }
};

// Async Motr context is inherited from Sync Context & S3AsyncOpContextBase

class S3AsyncMotrKVSWriterContext : public S3SyncMotrKVSWriterContext,
                                    public S3AsyncOpContextBase {

  std::string request_id = "";

 public:
  S3AsyncMotrKVSWriterContext(std::shared_ptr<RequestObject> req,
                              std::function<void()> success_callback,
                              std::function<void()> failed_callback,
                              int ops_count = 1,
                              std::shared_ptr<MotrAPI> motr_api = nullptr)
      : S3SyncMotrKVSWriterContext(req ? req->get_request_id() : "", ops_count),
        S3AsyncOpContextBase(req, success_callback, failed_callback, ops_count,
                             motr_api) {
    request_id = req ? req->get_request_id() : "";
    s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");
  }

  ~S3AsyncMotrKVSWriterContext() {
    s3_log(S3_LOG_DEBUG, request_id, "Destructor\n");
  }
};

enum class S3MotrKVSWriterOpState {
  start,
  failed_to_launch,
  failed,
  created,
  deleted,
  exists,   // Key already exists
  missing,  // Key does not exists
  deleting  // Key is being deleted.
};

class S3MotrKVSWriter {
 private:
  std::vector<struct m0_uint128> oid_list;
  std::vector<std::string> keys_list;  // used in delete multiple KV

  std::shared_ptr<RequestObject> request;
  std::shared_ptr<MotrAPI> s3_motr_api;
  std::unique_ptr<S3AsyncMotrKVSWriterContext> writer_context;
  std::unique_ptr<S3SyncMotrKVSWriterContext> sync_writer_context;
  std::unique_ptr<S3AsyncMotrKVSWriterContext> sync_context;
  std::string kvs_key;
  std::string kvs_value;

  std::string request_id;
  // Used to report to caller
  std::function<void()> handler_on_success;
  std::function<void()> handler_on_failed;

  S3MotrKVSWriterOpState state;

  struct s3_motr_idx_context* idx_ctx;

  void clean_up_contexts();

 public:
  S3MotrKVSWriter(std::shared_ptr<RequestObject> req,
                  std::shared_ptr<MotrAPI> motr_api = nullptr);

  S3MotrKVSWriter(std::string request_id,
                  std::shared_ptr<MotrAPI> motr_api = nullptr);
  virtual ~S3MotrKVSWriter();

  virtual S3MotrKVSWriterOpState get_state() { return state; }

  struct m0_uint128 get_oid() { return oid_list[0]; }

  // async create
  virtual void create_index(std::string index_name,
                            std::function<void(void)> on_success,
                            std::function<void(void)> on_failed);
  void create_index_successful();
  void create_index_failed();

  virtual void create_index_with_oid(struct m0_uint128 idx_id,
                                     std::function<void(void)> on_success,
                                     std::function<void(void)> on_failed);

  // Sync motr is currently done using motr_idx_op

  // void sync_index(std::function<void(void)> on_success,
  //                 std::function<void(void)> on_failed, int index_count = 1);
  // void sync_index_successful();
  // void sync_index_failed();

  // void sync_keyval(std::function<void(void)> on_success,
  //                  std::function<void(void)> on_failed);
  // void sync_keyval_successful();
  // void sync_keyval_failed();

  // async delete
  virtual void delete_index(struct m0_uint128 idx_oid,
                            std::function<void(void)> on_success,
                            std::function<void(void)> on_failed);
  // void delete_index(std::string index_name,
  //                   std::function<void(void)> on_success,
  //                   std::function<void(void)> on_failed);
  void delete_index_successful();
  void delete_index_failed();

  virtual void delete_indexes(std::vector<struct m0_uint128> oids,
                              std::function<void(void)> on_success,
                              std::function<void(void)> on_failed);
  void delete_indexes_successful();
  void delete_indexes_failed();

  virtual void put_keyval(struct m0_uint128 oid,
                          const std::map<std::string, std::string>& kv_list,
                          std::function<void(void)> on_success,
                          std::function<void(void)> on_failed);
  // Async save operation.
  virtual void put_keyval(struct m0_uint128 oid, std::string key,
                          std::string val, std::function<void(void)> on_success,
                          std::function<void(void)> on_failed);
  virtual int put_keyval_impl(const std::map<std::string, std::string>& kv_list,
                              bool is_async);

  void put_keyval_successful();
  void put_keyval_failed();

  // Sync save operation.
  virtual int put_keyval_sync(
      struct m0_uint128 oid, const std::map<std::string, std::string>& kv_list);
  // Async delete operation.
  void delete_keyval(struct m0_uint128 oid, std::string key,
                     std::function<void(void)> on_success,
                     std::function<void(void)> on_failed);

  virtual void delete_keyval(struct m0_uint128 oid,
                             std::vector<std::string> keys,
                             std::function<void(void)> on_success,
                             std::function<void(void)> on_failed);

  void delete_keyval_successful();
  void delete_keyval_failed();

  void set_up_key_value_store(struct s3_motr_kvs_op_context* kvs_ctx,
                              const std::string& key, const std::string& val,
                              size_t pos = 0);

  virtual int get_op_ret_code_for(int index) {
    return writer_context->get_errno_for(index);
  }

  virtual int get_op_ret_code_for_del_kv(int key_i) {
    return writer_context->get_motr_kvs_op_ctx()->rcs[key_i];
  }

  // For Testing purpose
  FRIEND_TEST(S3MotrKVSWritterTest, Constructor);
  FRIEND_TEST(S3MotrKVSWritterTest, CleanupContexts);
  FRIEND_TEST(S3MotrKVSWritterTest, CreateIndexIdxPresent);
  FRIEND_TEST(S3MotrKVSWritterTest, CreateIndex);
  FRIEND_TEST(S3MotrKVSWritterTest, CreateIndexSuccessful);
  FRIEND_TEST(S3MotrKVSWritterTest, CreateIndexEntityCreateFailed);
  FRIEND_TEST(S3MotrKVSWritterTest, CreateIndexFail);
  FRIEND_TEST(S3MotrKVSWritterTest, CreateIndexFailExists);
  FRIEND_TEST(S3MotrKVSWritterTest, SyncIndex);
  FRIEND_TEST(S3MotrKVSWritterTest, SyncIndexSuccessful);
  FRIEND_TEST(S3MotrKVSWritterTest, SyncIndexFailedMissingMetadata);
  FRIEND_TEST(S3MotrKVSWritterTest, SyncIndexFailedFailedMetadata);
  FRIEND_TEST(S3MotrKVSWritterTest, PutKeyVal);
  FRIEND_TEST(S3MotrKVSWritterTest, PutKeyValSuccessful);
  FRIEND_TEST(S3MotrKVSWritterTest, PutKeyValFailed);
  FRIEND_TEST(S3MotrKVSWritterTest, PutKeyValEmpty);
  FRIEND_TEST(S3MotrKVSWritterTest, DelIndexIdxPresent);
  FRIEND_TEST(S3MotrKVSWritterTest, DelIndexEntityDeleteFailed);
  FRIEND_TEST(S3MotrKVSWritterTest, DelIndexFailed);
  FRIEND_TEST(S3MotrKVSWritterTest, SyncKeyVal);
  FRIEND_TEST(S3MotrKVSWritterTest, SyncKeyvalSuccessful);
  FRIEND_TEST(S3MotrKVSWritterTest, SyncKeyValFailed);
  FRIEND_TEST(S3MotrKVSWritterTest, DelKeyVal);
  FRIEND_TEST(S3MotrKVSWritterTest, DelKeyValSuccess);
  FRIEND_TEST(S3MotrKVSWritterTest, DelKeyValFailed);
  FRIEND_TEST(S3MotrKVSWritterTest, DelKeyValEmpty);
  FRIEND_TEST(S3BucketMetadataV1Test, CreateBucketListIndexSuccessful);
  FRIEND_TEST(S3PartMetadataTest, CreatePartIndexSuccessful);
  FRIEND_TEST(S3PartMetadataTest, CreatePartIndexSuccessfulOnlyCreateIndex);
  FRIEND_TEST(S3PartMetadataTest, CreatePartIndexSuccessfulSaveMetadata);
  FRIEND_TEST(S3NewAccountRegisterNotifyActionTest,
              CreateBucketListIndexSuccessful);
};

#endif
