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

#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_CLOVIS_KVS_WRITER_H__
#define __MERO_FE_S3_SERVER_S3_CLOVIS_KVS_WRITER_H__

#include <memory>
#include <functional>
#include <gtest/gtest_prod.h>

#include "s3_request_object.h"
#include "s3_clovis_context.h"
#include "s3_asyncop_context_base.h"
#include "s3_clovis_wrapper.h"
#include "s3_log.h"

class S3ClovisKVSWriterContext : public S3AsyncOpContextBase {
  // Basic Operation context.
  struct s3_clovis_idx_op_context *clovis_idx_op_context;
  bool has_clovis_idx_op_context;

  // Read/Write Operation context.
  struct s3_clovis_kvs_op_context* clovis_kvs_op_context;
  bool has_clovis_kvs_op_context;

public:
  S3ClovisKVSWriterContext(std::shared_ptr<S3RequestObject> req,std::function<void()> success_callback, std::function<void()> failed_callback, int ops_count = 1) : S3AsyncOpContextBase(req, success_callback, failed_callback, ops_count) {
    s3_log(S3_LOG_DEBUG, "Constructor\n");
    // Create or write, we need op context
    clovis_idx_op_context = create_basic_idx_op_ctx(ops_count);
    has_clovis_idx_op_context = true;
    clovis_kvs_op_context = NULL;
    has_clovis_kvs_op_context = false;
  }

  ~S3ClovisKVSWriterContext() {
    s3_log(S3_LOG_DEBUG, "Destructor\n");
    if(has_clovis_idx_op_context) {
      free_basic_idx_op_ctx(clovis_idx_op_context);
    }
    if (has_clovis_kvs_op_context) {
      free_basic_kvs_op_ctx(clovis_kvs_op_context);
    }
  }

  struct s3_clovis_idx_op_context* get_clovis_idx_op_ctx() {
    return clovis_idx_op_context;
  }

  // Call this when you want to do write op.
  void init_kvs_write_op_ctx(int no_of_keys) {
    clovis_kvs_op_context = create_basic_kvs_op_ctx(no_of_keys);
    has_clovis_kvs_op_context = true;
  }

  struct s3_clovis_kvs_op_context* get_clovis_kvs_op_ctx() {
    return clovis_kvs_op_context;
  }

};

enum class S3ClovisKVSWriterOpState {
  failed,
  start,
  created,
  saved,
  deleted,
  exists,  // Object already exists
  missing,  // Object does not exists
};

class S3ClovisKVSWriter {
private:
  struct m0_uint128 id;

  std::shared_ptr<S3RequestObject> request;
  std::shared_ptr<ClovisAPI> s3_clovis_api;
  std::unique_ptr<S3ClovisKVSWriterContext> writer_context;
  std::string kvs_key;
  std::string kvs_value;

  int ops_count;

  // Used to report to caller
  std::function<void()> handler_on_success;
  std::function<void()> handler_on_failed;

  S3ClovisKVSWriterOpState state;

  // These functions will be removed once Mero KVS Supports Metadata update in
  // PUT
  void del_put_keyval(std::string key, std::string val,
                      std::function<void(void)> on_success,
                      std::function<void(void)> on_failed);
  void del_put_keyval_successful();
  void del_put_keyval_failed();
  void create_keyval();
  //--

 public:
  S3ClovisKVSWriter(std::shared_ptr<S3RequestObject> req, std::shared_ptr<ClovisAPI> s3_clovis_api);
  virtual ~S3ClovisKVSWriter();

  S3ClovisKVSWriterOpState get_state() {
    return state;
  }

  struct m0_uint128 get_oid() {
    return id;
  }

  // async create
  void create_index(std::string index_name,std::function<void(void)> on_success, std::function<void(void)> on_failed);
  void create_index_successful();
  void create_index_failed();

  void create_index_with_oid(struct m0_uint128 idx_id,
                             std::function<void(void)> on_success,
                             std::function<void(void)> on_failed);

  // async delete
  void delete_index(struct m0_uint128 idx_oid, std::function<void(void)> on_success, std::function<void(void)> on_failed);
  void delete_index(std::string index_name,std::function<void(void)> on_success, std::function<void(void)> on_failed);
  void delete_index_successful();
  void delete_index_failed();

  void delete_indexes(std::vector<struct m0_uint128> oids, std::function<void(void)> on_success, std::function<void(void)> on_failed);
  void delete_indexes_successful();
  void delete_indexes_failed();


  // Async save operation.
  void put_keyval(std::string index_name, std::string key, std::string val, std::function<void(void)> on_success, std::function<void(void)> on_failed);
  void put_keyval(struct m0_uint128 oid, std::string key, std::string  val, std::function<void(void)> on_success, std::function<void(void)> on_failed);
  void put_keyval_successful();
  void put_keyval_failed();

  // Async delete operation.
  void delete_keyval(std::string index_name, std::string key, std::function<void(void)> on_success, std::function<void(void)> on_failed);
  void delete_keyval(struct m0_uint128 oid, std::string key, std::function<void(void)> on_success, std::function<void(void)> on_failed);

  void delete_keyval(std::string index_name, std::vector<std::string> keys, std::function<void(void)> on_success, std::function<void(void)> on_failed);
  void delete_keyval(struct m0_uint128 oid, std::vector<std::string> keys, std::function<void(void)> on_success, std::function<void(void)> on_failed);

  void delete_keyval_successful();
  void delete_keyval_failed();

  void set_up_key_value_store(struct s3_clovis_kvs_op_context* kvs_ctx, std::string key, std::string val);

  int get_op_ret_code_for(int index) {
    return writer_context->get_errno_for(index);
  }


  // For Testing purpose
  FRIEND_TEST(S3ClovisKvsWritterTest, Constructor);
  FRIEND_TEST(S3ClovisKvsWritterTest, CreateIndexSuccess);
  FRIEND_TEST(S3ClovisKvsWritterTest, CreateIndexSuccessCallback);
  FRIEND_TEST(S3ClovisKvsWritterTest, CreateIndexFail);
  FRIEND_TEST(S3ClovisKvsWritterTest, CreateIndexFailCallback);
  FRIEND_TEST(S3ClovisKvsWritterTest, CreateIndexFailEmpty);
  FRIEND_TEST(S3ClovisKvsWritterTest, PutKeyValSuccess);
  FRIEND_TEST(S3ClovisKvsWritterTest, PutKeyValSuccessCallback);
  FRIEND_TEST(S3ClovisKvsWritterTest, PutKeyValFail);
  FRIEND_TEST(S3ClovisKvsWritterTest, PutKeyValFailCallback);
  FRIEND_TEST(S3ClovisKvsWritterTest, PutKeyValFailEmpty);
  FRIEND_TEST(S3ClovisKvsWritterTest, DelKeyValSuccess);
  FRIEND_TEST(S3ClovisKvsWritterTest, DelKeyValSuccessCallback);
  FRIEND_TEST(S3ClovisKvsWritterTest, DelKeyValFail);
  FRIEND_TEST(S3ClovisKvsWritterTest, DelKeyValFailCallback);
  FRIEND_TEST(S3ClovisKvsWritterTest, DelKeyValFailEmpty);
};

#endif
