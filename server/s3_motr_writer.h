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

#ifndef __S3_SERVER_S3_MOTR_WRITER_H__
#define __S3_SERVER_S3_MOTR_WRITER_H__

#include <deque>
#include <functional>
#include <memory>

#include "s3_asyncop_context_base.h"
#include "s3_motr_context.h"
#include "s3_motr_wrapper.h"
#include "s3_log.h"
#include "s3_md5_hash.h"
#include "s3_request_object.h"

class S3MotrWiterContext : public S3AsyncOpContextBase {
  // Basic Operation context.
  struct s3_motr_op_context* motr_op_context = NULL;

  // Read/Write Operation context.
  struct s3_motr_rw_op_context* motr_rw_op_context = NULL;

 public:
  S3MotrWiterContext(std::shared_ptr<RequestObject> req,
                     std::function<void()> success_callback,
                     std::function<void()> failed_callback, int ops_count = 1,
                     std::shared_ptr<MotrAPI> motr_api = nullptr);

  ~S3MotrWiterContext();

  struct s3_motr_op_context* get_motr_op_ctx();

  // Call this when you want to do write op.
  void init_write_op_ctx(size_t motr_buf_count) {
    motr_rw_op_context = create_basic_rw_op_ctx(motr_buf_count, 0, false);
  }

  struct s3_motr_rw_op_context* get_motr_rw_op_ctx() {
    return motr_rw_op_context;
  }
};

enum class S3MotrWiterOpState {
  start,
  failed_to_launch,
  failed,
  creating,
  created,
  saved,
  writing,
  deleting,
  deleted,
  success,
  exists,   // Object already exists
  missing,  // Object does not exists
};

class S3MotrWiter {

  std::shared_ptr<RequestObject> request;
  std::unique_ptr<S3MotrWiterContext> open_context;
  std::unique_ptr<S3MotrWiterContext> create_context;
  std::unique_ptr<S3MotrWiterContext> writer_context;
  std::unique_ptr<S3MotrWiterContext> delete_context;
  std::shared_ptr<MotrAPI> s3_motr_api;

  // Used to report to caller
  std::function<void()> handler_on_success;
  std::function<void()> handler_on_failed;

  std::vector<struct m0_uint128> oid_list;
  std::vector<int> layout_ids;

  S3MotrWiterOpState state;

  std::string content_md5;
  uint64_t last_index;
  std::string request_id;
  // md5 for the content written to motr.
  MD5hash md5crypt;

  // maintain state for debugging.
  size_t size_in_current_write;
  size_t total_written;

  bool is_object_opened;
  struct s3_motr_obj_context* obj_ctx;

  void* place_holder_for_last_unit;
  // layout_id for place_holder_for_last_unit can be changed if
  // the motr_writer object is reused after use for writing data.
  // create motr_writer and use for write data with layout id = 9
  // followed by reusing same object for deleting obj layout id =1
  // This causes buffer to be returned to pool with wrong id.
  bool last_op_was_write;
  int unit_size_for_place_holder;

  // buffer currently used to write, will be freed on completion
  std::shared_ptr<S3AsyncBufferOptContainer> write_async_buffer;

  // Write - single object, delete - multiple objects supported
  int open_objects();
  void open_objects_successful();
  void open_objects_failed();

  void write_content();
  void write_content_successful();
  void write_content_failed();

  void delete_objects();
  void delete_objects_successful();
  void delete_objects_failed();

  void clean_up_contexts();

 public:
  // struct m0_uint128 id;
  S3MotrWiter(std::shared_ptr<RequestObject> req, struct m0_uint128 object_id,
              uint64_t offset = 0, std::shared_ptr<MotrAPI> motr_api = nullptr);
  S3MotrWiter(std::shared_ptr<RequestObject> req, uint64_t offset = 0,
              std::shared_ptr<MotrAPI> motr_api = nullptr);
  virtual ~S3MotrWiter();
  void reset_buffers_if_any(int buf_unit_sz);

  virtual S3MotrWiterOpState get_state() { return state; }

  virtual struct m0_uint128 get_oid() {
    assert(oid_list.size() == 1);
    return oid_list[0];
  }

  virtual int get_layout_id() {
    assert(layout_ids.size() == 1);
    return layout_ids[0];
  }

  virtual void set_oid(struct m0_uint128 id) {
    is_object_opened = false;
    oid_list.clear();
    oid_list.push_back(id);
  }

  virtual void set_layout_id(int id) {
    layout_ids.clear();
    layout_ids.push_back(id);
  }

  // This concludes the md5 calculation
  virtual std::string get_content_md5() {
    // Complete MD5 computation and remember
    if (content_md5.empty()) {
      md5crypt.Finalize();
      content_md5 = md5crypt.get_md5_string();
    }
    s3_log(S3_LOG_DEBUG, request_id, "content_md5 of data written = %s\n",
           content_md5.c_str());
    return content_md5;
  }

  virtual std::string get_content_md5_base64() {
    return md5crypt.get_md5_base64enc_string();
  }

  // async create
  virtual void create_object(std::function<void(void)> on_success,
                             std::function<void(void)> on_failed, int layoutid);
  void create_object_successful();
  void create_object_failed();

  // Async save operation.
  virtual void write_content(std::function<void(void)> on_success,
                             std::function<void(void)> on_failed,
                             std::shared_ptr<S3AsyncBufferOptContainer> buffer);

  // Async delete operation.
  virtual void delete_object(std::function<void(void)> on_success,
                             std::function<void(void)> on_failed, int layoutid);

  virtual void delete_objects(std::vector<struct m0_uint128> oids,
                              std::vector<int> layoutids,
                              std::function<void(void)> on_success,
                              std::function<void(void)> on_failed);

  virtual int get_op_ret_code_for(int index);
  virtual int get_op_ret_code_for_delete_op(int index);

  void set_up_motr_data_buffers(struct s3_motr_rw_op_context* rw_ctx,
                                std::deque<evbuffer*>& data_items,
                                size_t motr_buf_count);

  // For Testing purpose
  FRIEND_TEST(S3MotrWiterTest, Constructor);
  FRIEND_TEST(S3MotrWiterTest, Constructor2);
  FRIEND_TEST(S3MotrWiterTest, CreateObjectTest);
  FRIEND_TEST(S3MotrWiterTest, CreateObjectSuccessfulTest);
  FRIEND_TEST(S3MotrWiterTest, CreateObjectFailedTest);
  FRIEND_TEST(S3MotrWiterTest, CreateObjectEntityCreateFailTest);
  FRIEND_TEST(S3MotrWiterTest, DeleteObjectTest);
  FRIEND_TEST(S3MotrWiterTest, DeleteObjectSuccessfulTest);
  FRIEND_TEST(S3MotrWiterTest, DeleteObjectFailedTest);
  FRIEND_TEST(S3MotrWiterTest, DeleteObjectsTest);
  FRIEND_TEST(S3MotrWiterTest, DeleteObjectsSuccessfulTest);
  FRIEND_TEST(S3MotrWiterTest, DeleteObjectsFailedTest);
  FRIEND_TEST(S3MotrWiterTest, DeleteObjectmotrEntityDeleteFailedTest);
  FRIEND_TEST(S3MotrWiterTest, OpenObjectsTest);
  FRIEND_TEST(S3MotrWiterTest, OpenObjectsEntityOpenFailedTest);
  FRIEND_TEST(S3MotrWiterTest, OpenObjectsFailedTest);
  FRIEND_TEST(S3MotrWiterTest, OpenObjectsFailedMissingTest);
  FRIEND_TEST(S3MotrWiterTest, WriteContentSuccessfulTest);
  FRIEND_TEST(S3MotrWiterTest, WriteContentFailedTest);
};

#endif
