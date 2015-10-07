
#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_CLOVIS_KVS_WRITER_H__
#define __MERO_FE_S3_SERVER_S3_CLOVIS_KVS_WRITER_H__

#include <memory>
#include <functional>

#include "s3_request_object.h"
#include "s3_clovis_context.h"
#include "s3_asyncop_context_base.h"

class S3ClovisKVSWriterContext : public S3AsyncOpContextBase {
  // Basic Operation context.
  struct s3_clovis_idx_op_context *clovis_idx_op_context;
  bool has_clovis_idx_op_context;

  // Read/Write Operation context.
  struct s3_clovis_kvs_op_context* clovis_kvs_op_context;
  bool has_clovis_kvs_op_context;

public:
  S3ClovisKVSWriterContext(std::shared_ptr<S3RequestObject> req,std::function<void()> success_callback, std::function<void()> failed_callback) : S3AsyncOpContextBase(req, success_callback, failed_callback) {
    printf("S3ClovisKVSWriterContext created\n");
    // Create or write, we need op context
    clovis_idx_op_context = create_basic_idx_op_ctx(1);
    has_clovis_idx_op_context = true;

    clovis_kvs_op_context = NULL;
    has_clovis_kvs_op_context = false;
  }

  ~S3ClovisKVSWriterContext() {
    printf("S3ClovisKVSWriterContext deleted.\n");
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
  notexists,  // Object does not exists
};

class S3ClovisKVSWriter {
private:
  struct m0_uint128 id;

  std::shared_ptr<S3RequestObject> request;
  std::unique_ptr<S3ClovisKVSWriterContext> writer_context;

  // Used to report to caller
  std::function<void()> handler_on_success;
  std::function<void()> handler_on_failed;

  S3ClovisKVSWriterOpState state;

public:
  S3ClovisKVSWriter(std::shared_ptr<S3RequestObject> req);
  ~S3ClovisKVSWriter();

  S3ClovisKVSWriterOpState get_state() {
    return state;
  }

  // async create
  void create_index(std::string index_name,std::function<void(void)> on_success, std::function<void(void)> on_failed);
  void create_index_successful();
  void create_index_failed();

  // Async save operation.
  void put_keyval(std::string index_name, std::string key, std::string val,std::function<void(void)> on_success, std::function<void(void)> on_failed);
  void put_keyval_successful();
  void put_keyval_failed();

  void set_up_key_value_store(struct s3_clovis_kvs_op_context* kvs_ctx, std::string key, std::string val);
};

#endif
