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

#ifndef __S3_SERVER_S3_MOTR_READER_H__
#define __S3_SERVER_S3_MOTR_READER_H__

#include <functional>
#include <memory>

#include "s3_asyncop_context_base.h"
#include "s3_buffer_sequence.h"
#include "s3_motr_context.h"
#include "s3_motr_layout.h"
#include "s3_motr_wrapper.h"
#include "s3_log.h"
#include "s3_option.h"
#include "s3_request_object.h"
#include "s3_evbuffer_wrapper.h"

extern S3Option* g_option_instance;

class S3MotrReaderContext : public S3AsyncOpContextBase {
  // Basic Operation context.
  struct s3_motr_op_context* motr_op_context;
  bool has_motr_op_context;

  // Read/Write Operation context.
  struct s3_motr_rw_op_context* motr_rw_op_context;
  bool has_motr_rw_op_context;
  std::unique_ptr<S3Evbuffer> p_s3_evbuffer;

  int layout_id;
  std::string request_id;
  std::string stripped_request_id;

 public:
  S3MotrReaderContext(std::shared_ptr<RequestObject> req,
                      std::function<void()> success_callback,
                      std::function<void()> failed_callback, int layoutid,
                      std::shared_ptr<MotrAPI> motr_api = nullptr)
      // Passing default value of opcount explicitly.
      : S3AsyncOpContextBase(req, success_callback, failed_callback, 1,
                             motr_api) {
    request_id = request->get_request_id();
    stripped_request_id = request->get_stripped_request_id();
    s3_log(S3_LOG_DEBUG, request_id, "%s Ctor: layout_id = %d\n", __func__,
           layoutid);
    assert(layoutid > 0);

    layout_id = layoutid;

    // Create or write, we need op context
    motr_op_context = create_basic_op_ctx(1);
    has_motr_op_context = true;

    motr_rw_op_context = NULL;
    has_motr_rw_op_context = false;
  }

  ~S3MotrReaderContext() {
    s3_log(S3_LOG_DEBUG, request_id, "%s\n", __func__);

    if (has_motr_op_context) {
      free_basic_op_ctx(motr_op_context);
    }
    if (has_motr_rw_op_context) {
      free_basic_rw_op_ctx(motr_rw_op_context);
    }
  }

  // Call this when you want to do read op.
  // param(in): motr_block_count - motr blocks to read
  // param(in): sz_per_block - motr unit size of each block to read
  // param(in/out): last_index - where next read should start
  bool init_read_op_ctx(std::string request_id, size_t motr_block_count,
                        size_t sz_per_block, uint64_t* last_index) {
    // Since we use const size buffer pool in libevent, we use its size of buf
    size_t total_read_sz = motr_block_count * sz_per_block;
    size_t evbuf_unit_buf_sz =
        S3Option::get_instance()->get_libevent_pool_buffer_size();
    size_t buf_count_in_evbuf =
        (total_read_sz + (evbuf_unit_buf_sz - 1)) / evbuf_unit_buf_sz;
    motr_rw_op_context =
        create_basic_rw_op_ctx(buf_count_in_evbuf, 0, evbuf_unit_buf_sz);
    if (motr_rw_op_context == NULL) {
      // out of memory
      s3_log(S3_LOG_ERROR, request_id,
             "create_basic_rw_op_ctx failed, buf_count_in_evbuf = %zu "
             "evbuf_unit_buf_sz = %zu\n",
             buf_count_in_evbuf, evbuf_unit_buf_sz);
      return false;
    }
    // Create real buffer space using evbuffer
    p_s3_evbuffer = std::unique_ptr<S3Evbuffer>(
        new S3Evbuffer(request_id, total_read_sz, evbuf_unit_buf_sz));
    int rc = p_s3_evbuffer->init();
    if (rc != 0) {
      s3_log(S3_LOG_ERROR, request_id, "p_s3_evbuffer->init failed\n");
      return false;
    }
    // Setup the motr structs to point to evbuf buffers
    p_s3_evbuffer->to_motr_read_buffers(motr_rw_op_context, last_index);

    has_motr_rw_op_context = true;

    return true;
  }

  struct s3_motr_op_context* get_motr_op_ctx() { return motr_op_context; }

  struct s3_motr_rw_op_context* get_motr_rw_op_ctx() {
    return motr_rw_op_context;
  }

  struct s3_motr_rw_op_context* get_ownership_motr_rw_op_ctx() {
    has_motr_rw_op_context = false;  // release ownership, caller should free.
    return motr_rw_op_context;
  }

  virtual struct evbuffer* get_evbuffer_ownership() {
    if (p_s3_evbuffer) {
      return p_s3_evbuffer->release_ownership();
    }
    return NULL;
  }

  S3Evbuffer* get_evbuffer() { return p_s3_evbuffer.get(); }
};

enum class S3MotrReaderOpState {
  start,
  failed_to_launch,
  failed,
  reading,
  success,
  missing,  // Missing object
  ooo,      // out-of-memory
};

class S3MotrReader {
 private:
  std::shared_ptr<RequestObject> request;
  std::unique_ptr<S3MotrReaderContext> reader_context;
  std::unique_ptr<S3MotrReaderContext> open_context;
  std::shared_ptr<MotrAPI> s3_motr_api;

  std::string request_id;
  std::string stripped_request_id;

  // Used to report to caller
  std::function<void()> handler_on_success;
  std::function<void()> handler_on_failed;

  struct m0_uint128 oid;
  struct m0_fid pvid;
  int layout_id;
  size_t motr_unit_size;

  S3MotrReaderOpState state = S3MotrReaderOpState::start;

  // Holds references to buffers after the read so it can be consumed.
  struct s3_motr_rw_op_context* motr_rw_op_context = nullptr;
  size_t iteration_index = 0;
  // to Help iteration.
  size_t num_of_blocks_to_read = 0;

  uint64_t last_index = 0;

  bool is_object_opened = false;
  struct s3_motr_obj_context* obj_ctx = nullptr;

  // fill entire object with zeroes when reading it from the storage
  // See S3MotrWiter::corrupt_fill_zero.
  bool corrupt_fill_zero = false;

  // Internal open operation so motr can fetch required object metadata
  // for example object pool version
  int open_object(std::function<void(void)> on_success,
                  std::function<void(void)> on_failed);
  void open_object_successful();
  void open_object_failed();

  // This reads "num_of_blocks_to_read" blocks, and is called after object is
  // opened.
  virtual bool read_object();
  void read_object_successful();
  void read_object_failed();

  void clean_up_contexts();

 public:
  // object id is generated at upper level and passed to this constructor
  S3MotrReader(std::shared_ptr<RequestObject> req, struct m0_uint128 id,
               int layout_id, struct m0_fid pvid = {},
               std::shared_ptr<MotrAPI> motr_api = {});
  virtual ~S3MotrReader();

  virtual S3MotrReaderOpState get_state() { return state; }
  virtual struct m0_uint128 get_oid() { return oid; }

  virtual void set_oid(struct m0_uint128 id) { oid = id; }

  // async read
  // Returns: true = launched, false = failed to launch (out-of-memory)
  virtual bool read_object_data(size_t num_of_blocks,
                                std::function<void(void)> on_success,
                                std::function<void(void)> on_failed);

  virtual bool check_object_exist(std::function<void(void)> on_success,
                                  std::function<void(void)> on_failed);

  // Iterate over the content.
  // Returns size of data in first block and 0 if there is no content,
  // and content in data.
  virtual size_t get_first_block(char** data);
  virtual size_t get_next_block(char** data);
  virtual S3BufferSequence extract_blocks_read();
  virtual S3Evbuffer* get_evbuffer() { return reader_context->get_evbuffer(); }

  virtual size_t get_last_index() { return last_index; }
  virtual struct evbuffer* get_evbuffer_ownership() {
    if (reader_context) {
      return reader_context->get_evbuffer_ownership();
    }
    return NULL;
  }

  virtual void set_last_index(size_t index) { last_index = index; }

  // For Testing purpose
  FRIEND_TEST(S3MotrReaderTest, Constructor);
  FRIEND_TEST(S3MotrReaderTest, OpenObjectDataTest);
  FRIEND_TEST(S3MotrReaderTest, OpenObjectCheckNoHoleFlagTest);
  FRIEND_TEST(S3MotrReaderTest, ReadObjectDataTest);
  FRIEND_TEST(S3MotrReaderTest, ReadObjectDataCheckNoHoleFlagTest);
  FRIEND_TEST(S3MotrReaderTest, ReadObjectDataSuccessful);
  FRIEND_TEST(S3MotrReaderTest, ReadObjectDataFailed);
  FRIEND_TEST(S3MotrReaderTest, CleanupContexts);
  FRIEND_TEST(S3MotrReaderTest, OpenObjectTest);
  FRIEND_TEST(S3MotrReaderTest, OpenObjectFailedTest);
  FRIEND_TEST(S3MotrReaderTest, ReadObjectDataFailedMissing);
  FRIEND_TEST(S3MotrReaderTest, OpenObjectMissingTest);
  FRIEND_TEST(S3MotrReaderTest, OpenObjectErrFailedTest);
  FRIEND_TEST(S3MotrReaderTest, OpenObjectSuccessTest);
};

#endif
