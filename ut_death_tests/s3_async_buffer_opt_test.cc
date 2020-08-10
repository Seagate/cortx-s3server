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

#include "s3_async_buffer_opt.h"
#include "gtest/gtest.h"
#include "s3_option.h"

extern S3Option* g_option_instance;

// To use a test fixture, derive a class from testing::Test.
class S3AsyncBufferOptContainerTest : public testing::Test {
 protected:  // You should make the members protected s.t. they can be
             // accessed from sub-classes.

  S3AsyncBufferOptContainerTest()
      : nfourk_buffer(g_option_instance->get_libevent_pool_buffer_size(), 'A') {
    buffer = new S3AsyncBufferOptContainer(
        g_option_instance->get_libevent_pool_buffer_size());
  }

  ~S3AsyncBufferOptContainerTest() { delete buffer; }

  // A helper functions
  evbuf_t* get_evbuf_t_with_data(std::string content) {
    evbuf_t* buf = evbuffer_new();
    // buf lifetime is managed by S3AsyncBufferOptContainer
    evbuffer_add(buf, content.c_str(), content.length());
    return buf;
  }

  // gets the internal data pointer with length from evbuf_t
  const char* get_datap_4_evbuf_t(evbuf_t* buf) {
    return (const char*)evbuffer_pullup(buf, evbuffer_get_length(buf));
  }

  // Declares the variables your tests want to use.
  S3AsyncBufferOptContainer* buffer;
  std::string nfourk_buffer;
};

// Non std buf size = non multiple of size_of_each_buf
// (libevent_pool_buffer_size)
TEST_F(S3AsyncBufferOptContainerTest, GetBufferWithoutAddDeathTest) {
  buffer->add_content(get_evbuf_t_with_data(nfourk_buffer), false, false, true);
  buffer->add_content(get_evbuf_t_with_data(nfourk_buffer), false, false, true);

  // Ask for first part
  auto ret = buffer->get_buffers(nfourk_buffer.length());

  EXPECT_DEATH(buffer->get_buffers(nfourk_buffer.length()),
               "Assertion `processing_q.empty\\(\\)' failed");
}
