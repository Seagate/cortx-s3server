
#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_CLOVIS_KVS_READER_H__
#define __MERO_FE_S3_SERVER_S3_CLOVIS_KVS_READER_H__

#include <memory>
#include <functional>

#include "s3_request_object.h"
#include "s3_clovis_context.h"
#include "s3_asyncop_context_base.h"

class S3ClovisKVSReaderContext : public S3AsyncOpContextBase {
  // Basic Operation context.
  struct s3_clovis_op_context* clovis_op_context;
  bool has_clovis_op_context;

  struct s3_clovis_idx_op_context *clovis_idx_op_context;
  bool has_clovis_idx_op_context;

  // Read/Write Operation context.
  struct s3_clovis_kvs_op_context* clovis_kvs_op_context;
  bool has_clovis_kvs_op_context;

public:
  S3ClovisKVSReaderContext(std::shared_ptr<S3RequestObject> req,std::function<void()> success_callback, std::function<void()> failed_callback) : S3AsyncOpContextBase(req, success_callback, failed_callback) {
    // Create or write, we need op context
    clovis_op_context = create_basic_op_ctx(1);
    has_clovis_op_context = true;

    clovis_idx_op_context = NULL;
    has_clovis_idx_op_context = false;
    clovis_kvs_op_context = NULL;
    has_clovis_kvs_op_context = false;
  }

  ~S3ClovisKVSReaderContext() {
    if (has_clovis_op_context) {
      free_basic_op_ctx(clovis_op_context);
    }
    if(has_clovis_idx_op_context) {
      free_basic_idx_op_ctx(clovis_idx_op_context);
    }
    if (has_clovis_kvs_op_context) {
      free_basic_kvs_op_ctx(clovis_kvs_op_context);
    }
  }

  struct s3_clovis_op_context* get_clovis_op_ctx() {
    return clovis_op_context;
  }

  void init_idx_read_op_ctx(int idx_count) {
    clovis_idx_op_context = create_basic_idx_op_ctx(idx_count);
    has_clovis_idx_op_context = true;
  }

  struct s3_clovis_idx_op_context* get_clovis_idx_op_ctx() {
    return clovis_idx_op_context;
  }

  // Call this when you want to do write op.
  void init_kvs_read_op_ctx(int no_of_keys) {
    clovis_kvs_op_context = create_basic_kvs_op_ctx(no_of_keys);
    has_clovis_kvs_op_context = true;
  }

  struct s3_clovis_kvs_op_context* get_clovis_kvs_op_ctx() {
    return clovis_kvs_op_context;
  }

};

enum class S3ClovisKVSReaderOpState {
  failed,
  start,
  present,
};

class S3ClovisKVSReader {
private:
  struct m0_uint128 id;

  std::shared_ptr<S3RequestObject> request;

  // Used to report to caller
  std::function<void()> handler_on_success;
  std::function<void()> handler_on_failed;

  S3ClovisKVSReaderOpState state;

  // Holds references to keys and values after the read so it can be consumed.
  struct s3_clovis_kvs_op_context* clovis_kvs_op_context;
  std::string last_value;
  size_t iteration_index;

public:
  //struct m0_uint128 id;
  S3ClovisKVSReader(std::shared_ptr<S3RequestObject> req);

  S3ClovisKVSReaderOpState get_state() {
    return state;
  }

  // Async save operation.
  void get_keyval(std::string index_name, std::string key,std::function<void(void)> on_success, std::function<void(void)> on_failed);
  void get_keyval_successful();
  void get_keyval_failed();

  // TODO
  std::string get_value() {
    return last_value;
  }
};

#endif
