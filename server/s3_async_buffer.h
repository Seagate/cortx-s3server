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

#ifndef __S3_SERVER_S3_ASYNC_BUFFER_H__
#define __S3_SERVER_S3_ASYNC_BUFFER_H__

#include <deque>
#include <string>
#include <tuple>

#include <evhtp.h>
#include "s3_log.h"

#include <gtest/gtest_prod.h>

// Wrapper around evhtp evbuf_t buffers for incoming data.
class S3AsyncBufferContainer {
 private:
  // Remembers all the incoming data buffers
  std::deque<evbuf_t*> buffered_input;
  size_t buffered_input_length;

  bool is_expecting_more;

  // Manages read state. stores count of bufs shared outside for consumption.
  size_t count_bufs_shared_for_read;

 public:
  S3AsyncBufferContainer();
  virtual ~S3AsyncBufferContainer();

  // Call this to indicate that no more data will be added to buffer.
  void freeze();

  bool is_freezed();

  size_t length();

  void add_content(evbuf_t* buf);

  // Call this to get at least expected_content_size of data buffers.
  // Anything less is returned only if there is no more data to be filled in.
  // Call mark_size_of_data_consumed() to drain the data that was consumed
  // after get_buffers_ref call.
  std::deque<std::tuple<void*, size_t> > get_buffers_ref(
      size_t expected_content_size);

  // Pull up all the data in contiguous memory and releases internal buffers
  // only if all data is in and its freezed. Use check is_freezed()
  std::string get_content_as_string();

  void mark_size_of_data_consumed(size_t size_consumed);

  // Unit Testing only
  FRIEND_TEST(S3AsyncBufferContainerTest,
              DataBufferIsAddedToQueueAndCanBeRetrieved);
};

#endif
