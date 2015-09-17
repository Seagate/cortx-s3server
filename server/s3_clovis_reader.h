
#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_CLOVIS_READER_H__
#define __MERO_FE_S3_SERVER_S3_CLOVIS_READER_H__

#include <memory>
#include <functional>

#include "s3_request_object.h"
#include "s3_clovis_context.h"
#include "s3_asyncop_context_base.h"

class S3ClovisReaderContext : public S3AsyncOpContextBase {
  // Basic Operation context.
  struct s3_clovis_op_context* clovis_op_context;
  bool has_clovis_op_context;

  // Read/Write Operation context.
  struct s3_clovis_rw_op_context* clovis_rw_op_context;
  bool has_clovis_rw_op_context;

public:
  S3ClovisReaderContext(std::shared_ptr<S3RequestObject> req, std::function<void()> success_callback, std::function<void()> failed_callback) : S3AsyncOpContextBase(req, success_callback, failed_callback) {
    // Create or write, we need op context
    clovis_op_context = create_basic_op_ctx(1);
    has_clovis_op_context = true;

    clovis_rw_op_context = NULL;
    has_clovis_rw_op_context = false;
  }

  ~S3ClovisReaderContext() {
    if (has_clovis_op_context) {
      free_basic_op_ctx(clovis_op_context);
    }
    if (has_clovis_rw_op_context) {
      free_basic_rw_op_ctx(clovis_rw_op_context);
    }
  }

  // Call this when you want to do write op.
  void init_read_op_ctx(size_t clovis_block_count, size_t clovis_block_size) {
    clovis_rw_op_context = create_basic_rw_op_ctx(clovis_block_count, clovis_block_size);
    has_clovis_rw_op_context = true;

    uint64_t last_index = 0;
    for (size_t i = 0; i < clovis_block_count; i++) {
      clovis_rw_op_context->ext->iv_index[i] = last_index ;
      clovis_rw_op_context->ext->iv_vec.v_count[i] = clovis_block_size;
      last_index += clovis_block_size;

      /* we don't want any attributes */
      clovis_rw_op_context->attr->ov_vec.v_count[i] = 0;
    }
  }

  struct s3_clovis_op_context* get_clovis_op_ctx() {
    return clovis_op_context;
  }

  struct s3_clovis_rw_op_context* get_clovis_rw_op_ctx() {
    return clovis_rw_op_context;
  }

  struct s3_clovis_rw_op_context* get_ownership_clovis_rw_op_ctx() {
    has_clovis_rw_op_context = false;  // release ownership, caller should free.
    return clovis_rw_op_context;
  }

};

enum class S3ClovisReaderOpState {
  failed,
  start,
  complete,
};

class S3ClovisReader {
private:
  struct m0_uint128 id;

  std::shared_ptr<S3RequestObject> request;

  // Used to report to caller
  std::function<void()> handler_on_success;
  std::function<void()> handler_on_failed;

  S3ClovisReaderOpState state;

  // Holds references to buffers after the read so it can be consumed.
  struct s3_clovis_rw_op_context* clovis_rw_op_context;
  size_t iteration_index;

  std::string content_md5;
  size_t content_length;

public:
  //struct m0_uint128 id;
  S3ClovisReader(std::shared_ptr<S3RequestObject> req);

  S3ClovisReaderOpState get_state() {
    return state;
  }

  std::string get_content_md5() {
    return content_md5;
  }

  int get_content_length() {
    return content_length;
  }

  std::string get_content_length_str() {
    char c_content_size[1024];
    sprintf(c_content_size, "%ld", content_length);
    return std::string(c_content_size);
  }

  // async read
  void read_object(std::function<void(void)> on_success, std::function<void(void)> on_failed);
  void read_object_successful();
  void read_object_failed();

  // Iterate over the content.
  // Returns size of data in first block and 0 if there is no content,
  // and content in data.
  size_t get_first_block(char** data);
  size_t get_next_block(char** data);
};

#endif
