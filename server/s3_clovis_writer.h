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
 * Original creation date: 1-Oct-2015
 */

#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_CLOVIS_WRITER_H__
#define __MERO_FE_S3_SERVER_S3_CLOVIS_WRITER_H__

#include <memory>
#include <deque>
#include <functional>

#include "s3_request_object.h"
#include "s3_clovis_context.h"
#include "s3_asyncop_context_base.h"
#include "s3_md5_hash.h"
#include "s3_log.h"

class S3ClovisWriterContext : public S3AsyncOpContextBase {
  // Basic Operation context.
  struct s3_clovis_op_context* clovis_op_context;
  bool has_clovis_op_context;

  // Read/Write Operation context.
  struct s3_clovis_rw_op_context* clovis_rw_op_context;
  bool has_clovis_rw_op_context;

public:
  S3ClovisWriterContext(std::shared_ptr<S3RequestObject> req, std::function<void()> success_callback, std::function<void()> failed_callback, int ops_count = 1) : S3AsyncOpContextBase(req, success_callback, failed_callback, ops_count) {
    s3_log(S3_LOG_DEBUG, "Constructor\n");
    // Create or write, we need op context
    clovis_op_context = create_basic_op_ctx(ops_count);
    has_clovis_op_context = true;

    clovis_rw_op_context = NULL;
    has_clovis_rw_op_context = false;
  }

  ~S3ClovisWriterContext() {
    s3_log(S3_LOG_DEBUG, "Destructor\n");
    if (has_clovis_op_context) {
      free_basic_op_ctx(clovis_op_context);
    }
    if (has_clovis_rw_op_context) {
      free_basic_rw_op_ctx(clovis_rw_op_context);
    }
  }

  struct s3_clovis_op_context* get_clovis_op_ctx() {
    return clovis_op_context;
  }

  // Call this when you want to do write op.
  void init_write_op_ctx(size_t clovis_block_count, size_t clovis_block_size) {
    clovis_rw_op_context = create_basic_rw_op_ctx(clovis_block_count, clovis_block_size);
    has_clovis_rw_op_context = true;
  }

  struct s3_clovis_rw_op_context* get_clovis_rw_op_ctx() {
    return clovis_rw_op_context;
  }

};

enum class S3ClovisWriterOpState {
  failed,
  start,
  created,
  saved,
  deleted,
  exists,  // Object already exists
  missing,  // Object does not exists
};

class S3ClovisWriter {
private:
  struct m0_uint128 oid;

  std::shared_ptr<S3RequestObject> request;
  std::unique_ptr<S3ClovisWriterContext> writer_context;

  // Used to report to caller
  std::function<void()> handler_on_success;
  std::function<void()> handler_on_failed;

  S3ClovisWriterOpState state;

  std::string content_md5;
  uint64_t last_index;

  // md5 for the content written to clovis.
  MD5hash  md5crypt;

  // TODO remove, just for debugging.
  size_t total_written;

  int ops_count;

public:
  //struct m0_uint128 id;
  S3ClovisWriter(std::shared_ptr<S3RequestObject> req, struct m0_uint128 object_id, uint64_t offset = 0);
  S3ClovisWriter(std::shared_ptr<S3RequestObject> req, uint64_t offset = 0);

  S3ClovisWriterOpState get_state() {
    return state;
  }

  struct m0_uint128 get_oid() {
    return oid;
  }

  void set_oid(struct m0_uint128 id) {
    //TODO
    // oid = id;
  }

  // This concludes the md5 calculation
  std::string get_content_md5() {
    // Complete MD5 computation and remember
    if (content_md5.empty()) {
      md5crypt.Finalize();
      content_md5 = md5crypt.get_md5_string();
    }
    s3_log(S3_LOG_DEBUG, "content_md5 of data written = %s\n", content_md5.c_str());
    return content_md5;
  }

  // async create
  void create_object(std::function<void(void)> on_success, std::function<void(void)> on_failed);
  void create_object_successful();
  void create_object_failed();

  // Async save operation.
  void write_content(std::function<void(void)> on_success, std::function<void(void)> on_failed,
      S3AsyncBufferContainer& buffer);
  void write_content_successful();
  void write_content_failed();

  // Async delete operation.
  void delete_object(std::function<void(void)> on_success, std::function<void(void)> on_failed);
  void delete_object_successful();
  void delete_object_failed();

  void delete_objects(std::vector<struct m0_uint128> oids, std::function<void(void)> on_success, std::function<void(void)> on_failed);
  void delete_objects_successful();
  void delete_objects_failed();

  int get_op_ret_code_for(int index) {
    return writer_context->get_errno_for(index);
  }

  // xxx remove this
  void set_up_clovis_data_buffers(struct s3_clovis_rw_op_context* rw_ctx,
      std::deque< std::tuple<void*, size_t> >& data_items, size_t clovis_block_count);
};

#endif
