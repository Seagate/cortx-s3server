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

#ifndef __S3_SERVER_S3_CLOVIS_WRITER_H__
#define __S3_SERVER_S3_CLOVIS_WRITER_H__

#include <deque>
#include <functional>
#include <memory>

#include "s3_asyncop_context_base.h"
#include "s3_clovis_context.h"
#include "s3_clovis_wrapper.h"
#include "s3_log.h"
#include "s3_md5_hash.h"
#include "s3_request_object.h"

class S3ClovisWriterContext : public S3AsyncOpContextBase {
  // Basic Operation context.
  struct s3_clovis_op_context* clovis_op_context;
  bool has_clovis_op_context;

  // Read/Write Operation context.
  struct s3_clovis_rw_op_context* clovis_rw_op_context;
  bool has_clovis_rw_op_context;

  // buffer currently used to write, will be freed on completion
  std::shared_ptr<S3AsyncBufferOptContainer> write_async_buffer;

 public:
  S3ClovisWriterContext(std::shared_ptr<S3RequestObject> req,
                        std::function<void()> success_callback,
                        std::function<void()> failed_callback,
                        int ops_count = 1)
      : S3AsyncOpContextBase(req, success_callback, failed_callback,
                             ops_count) {
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
  void init_write_op_ctx(size_t clovis_buf_count) {
    clovis_rw_op_context = create_basic_rw_op_ctx(clovis_buf_count, false);
    has_clovis_rw_op_context = true;
  }

  struct s3_clovis_rw_op_context* get_clovis_rw_op_ctx() {
    return clovis_rw_op_context;
  }

  void remember_async_buf(std::shared_ptr<S3AsyncBufferOptContainer> buffer) {
    write_async_buffer = buffer;
  }

  std::shared_ptr<S3AsyncBufferOptContainer> get_write_async_buffer() {
    return write_async_buffer;
  }
};

enum class S3ClovisWriterOpState {
  failed,
  start,
  created,
  saved,
  deleted,
  exists,   // Object already exists
  missing,  // Object does not exists
};

class S3ClovisWriter {
 private:
  std::shared_ptr<S3RequestObject> request;
  std::unique_ptr<S3ClovisWriterContext> writer_context;
  std::shared_ptr<ClovisAPI> s3_clovis_api;

  // Used to report to caller
  std::function<void()> handler_on_success;
  std::function<void()> handler_on_failed;

  struct m0_uint128 oid;

  S3ClovisWriterOpState state;

  std::string content_md5;
  uint64_t last_index;

  // md5 for the content written to clovis.
  MD5hash md5crypt;

  // maintain state for debugging.
  size_t size_in_current_write;
  size_t total_written;

  int ops_count;

  void* place_holder_for_last_unit;

  // helper to set mock clovis apis, only used in tests.
  void reset_clovis_api(std::shared_ptr<ClovisAPI> api);

 public:
  // struct m0_uint128 id;
  S3ClovisWriter(std::shared_ptr<S3RequestObject> req,
                 struct m0_uint128 object_id, uint64_t offset = 0);
  S3ClovisWriter(std::shared_ptr<S3RequestObject> req, uint64_t offset = 0);
  ~S3ClovisWriter();

  virtual S3ClovisWriterOpState get_state() { return state; }

  virtual struct m0_uint128 get_oid() { return oid; }

  virtual void set_oid(struct m0_uint128 id) { oid = id; }

  // This concludes the md5 calculation
  virtual std::string get_content_md5() {
    // Complete MD5 computation and remember
    if (content_md5.empty()) {
      md5crypt.Finalize();
      content_md5 = md5crypt.get_md5_string();
    }
    s3_log(S3_LOG_DEBUG, "content_md5 of data written = %s\n",
           content_md5.c_str());
    return content_md5;
  }

  // async create
  virtual void create_object(std::function<void(void)> on_success,
                             std::function<void(void)> on_failed);
  void create_object_successful();
  void create_object_failed();

  // Async save operation.
  virtual void write_content(std::function<void(void)> on_success,
                             std::function<void(void)> on_failed,
                             std::shared_ptr<S3AsyncBufferOptContainer> buffer);
  void write_content_successful();
  void write_content_failed();

  // Async delete operation.
  virtual void delete_object(std::function<void(void)> on_success,
                             std::function<void(void)> on_failed);
  void delete_object_successful();
  void delete_object_failed();

  virtual void delete_objects(std::vector<struct m0_uint128> oids,
                              std::function<void(void)> on_success,
                              std::function<void(void)> on_failed);
  void delete_objects_successful();
  void delete_objects_failed();

  virtual int get_op_ret_code_for(int index) {
    return writer_context->get_errno_for(index);
  }

  void set_up_clovis_data_buffers(struct s3_clovis_rw_op_context* rw_ctx,
                                  std::deque<evbuffer*>& data_items,
                                  size_t clovis_buf_count);

  // For Testing purpose
  FRIEND_TEST(S3ClovisWriterTest, Constructor);
  FRIEND_TEST(S3ClovisWriterTest, Constructor2);
  FRIEND_TEST(S3ClovisWriterTest, CreateObjectTest);
  FRIEND_TEST(S3ClovisWriterTest, CreateObjectSuccessfulTest);
  FRIEND_TEST(S3ClovisWriterTest, CreateObjectFailedTest);
  FRIEND_TEST(S3ClovisWriterTest, DeleteObjectTest);
  FRIEND_TEST(S3ClovisWriterTest, DeleteObjectSuccessfulTest);
  FRIEND_TEST(S3ClovisWriterTest, DeleteObjectFailedTest);
  FRIEND_TEST(S3ClovisWriterTest, DeleteObjectsTest);
  FRIEND_TEST(S3ClovisWriterTest, DeleteObjectsSuccessfulTest);
  FRIEND_TEST(S3ClovisWriterTest, DeleteObjectsFailedTest);
  FRIEND_TEST(S3ClovisWriterTest, WriteContentTest);
  FRIEND_TEST(S3ClovisWriterTest, WriteContentSuccessfulTest);
  FRIEND_TEST(S3ClovisWriterTest, WriteContentFailedTest);
};

#endif
