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

#ifndef __MOTR_FE_S3_SERVER_S3_ASYNC_BUFFER_OPT_H__
#define __MOTR_FE_S3_SERVER_S3_ASYNC_BUFFER_OPT_H__

#include <deque>
#include <string>
#include <utility>

#include <evhtp.h>
#include "s3_log.h"

#include <gtest/gtest_prod.h>

// Zero-copy Optimized version of S3AsyncBufferContainer
// Constraints: Each block of data within async buffer
// is contiguous bytes of block configured to be 4k/8k/16.
// Wrapper around evhtp evbuf_t buffers for incoming data.
class S3AsyncBufferOptContainer {
 private:
  // Remembers all the incoming data buffers
  // buf which is completely filled with data and ready for use
  std::deque<evbuf_t *> ready_q;
  // buf given out for consumption
  std::deque<evbuf_t *> processing_q;

  size_t size_of_each_evbuf;  // ideally 4k/8k/16k

  // Actual content within buffer
  size_t content_length;

  bool is_expecting_more;

  // Manages read state. stores count of bufs shared outside for consumption.
  size_t count_bufs_shared_for_read;

 public:
  S3AsyncBufferOptContainer(size_t size_of_each_buf);
  virtual ~S3AsyncBufferOptContainer();

  // Call this to indicate that no more data will be added to buffer.
  void freeze();

  virtual bool is_freezed();

  virtual size_t get_content_length();

  bool add_content(evbuf_t *buf, bool is_first_buf, bool is_last_buf = false,
                   bool is_put_request = false);

  // Call this to get at least expected_content_size of data buffers.
  // Anything less is returned only if there is no more data to be filled in.
  // Call flush_used_buffers() to drain the data that was consumed
  // after get_buffers call.
  // expected_content_size should be multiple of libevent read mempool item size
  std::pair<std::deque<evbuf_t *>, size_t> get_buffers(
      size_t expected_content_size);

  // Pull up all the data in contiguous memory and releases internal buffers
  // only if all data is in and its freezed. Use check is_freezed()
  std::string get_content_as_string();

  // flush buffers received using get_buffers
  void flush_used_buffers();
};

#endif
