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

#include "s3_async_buffer.h"
#include "gtest/gtest.h"

// To use a test fixture, derive a class from testing::Test.
class S3AsyncBufferContainerTest : public testing::Test {
 protected:  // You should make the members protected s.t. they can be
             // accessed from sub-classes.

  S3AsyncBufferContainerTest() { buffer = new S3AsyncBufferContainer(); }

  ~S3AsyncBufferContainerTest() { delete buffer; }

  // A helper functions
  evbuf_t* get_evbuf_t_with_data(std::string content) {
    evbuf_t* buf = evbuffer_new();
    // buf lifetime is managed by S3AsyncBufferContainer
    evbuffer_add(buf, content.c_str(), content.length());
    return buf;
  }

  // Declares the variables your tests want to use.
  S3AsyncBufferContainer* buffer;
};

TEST_F(S3AsyncBufferContainerTest, HasProperInitState) {
  EXPECT_EQ(0, buffer->length());
  EXPECT_FALSE(buffer->is_freezed());
  EXPECT_STREQ("", buffer->get_content_as_string().c_str());
}

TEST_F(S3AsyncBufferContainerTest, IsProperlyLockedWithFreeze) {
  buffer->freeze();
  EXPECT_TRUE(buffer->is_freezed());
  EXPECT_STREQ("", buffer->get_content_as_string().c_str());
}

TEST_F(S3AsyncBufferContainerTest, DataBufferAddedToQueueAndCanBeRetrieved) {
  buffer->add_content(get_evbuf_t_with_data("Hello World"));
  buffer->freeze();
  EXPECT_TRUE(buffer->is_freezed());
  EXPECT_STREQ("Hello World", buffer->get_content_as_string().c_str());
}

TEST_F(S3AsyncBufferContainerTest,
       MultipleDataBufferAddedToQueueAndCanBeRetrieved) {
  buffer->add_content(get_evbuf_t_with_data("Hello World."));
  buffer->add_content(get_evbuf_t_with_data("Seagate Storage"));
  buffer->freeze();
  EXPECT_TRUE(buffer->is_freezed());
  EXPECT_STREQ("Hello World.Seagate Storage",
               buffer->get_content_as_string().c_str());
}

TEST_F(S3AsyncBufferContainerTest, DataInTransitAndAskingForMoreReturnsEmptyQ) {
  buffer->add_content(get_evbuf_t_with_data("Hello"));    // len = 5
  buffer->add_content(get_evbuf_t_with_data("Seagate"));  // len = 7
  EXPECT_EQ(5 + 7, buffer->length());
  EXPECT_FALSE(buffer->is_freezed());

  EXPECT_TRUE(buffer->get_buffers_ref(20).empty());

  // Flush all to free buffers
  buffer->freeze();
  buffer->get_content_as_string();  // Flush all contents
  EXPECT_EQ(0, buffer->length());
}

TEST_F(S3AsyncBufferContainerTest,
       DataInTransitAndAskingForLessReturnsValidData) {
  buffer->add_content(get_evbuf_t_with_data("Hello"));    // len = 5
  buffer->add_content(get_evbuf_t_with_data("Seagate"));  // len = 7
  EXPECT_EQ(5 + 7, buffer->length());
  EXPECT_FALSE(buffer->is_freezed());

  std::deque<std::tuple<void*, size_t> > bufs = buffer->get_buffers_ref(5);
  EXPECT_EQ(1, bufs.size());

  std::tuple<void*, size_t> item = bufs.front();
  EXPECT_EQ(0, strncmp("Hello", (char*)std::get<0>(item), 5));
  EXPECT_EQ(5, std::get<1>(item));
  bufs.pop_front();
  buffer->mark_size_of_data_consumed(5);  // Flush all contents

  bufs = buffer->get_buffers_ref(7);
  item = bufs.front();
  EXPECT_EQ(0, strncmp("Seagate", (char*)std::get<0>(item), 7));
  EXPECT_EQ(7, std::get<1>(item));
  bufs.pop_front();

  buffer->mark_size_of_data_consumed(7);  // Flush all contents
  EXPECT_EQ(0, buffer->length());
}

// Non Boundary check
TEST_F(S3AsyncBufferContainerTest,
       DataInTransitAndAskingForLessReturnsValidData1) {
  buffer->add_content(get_evbuf_t_with_data("Hello"));    // len = 5
  buffer->add_content(get_evbuf_t_with_data("Seagate"));  // len = 7
  EXPECT_EQ(5 + 7, buffer->length());
  EXPECT_FALSE(buffer->is_freezed());

  std::deque<std::tuple<void*, size_t> > bufs = buffer->get_buffers_ref(4);
  EXPECT_EQ(1, bufs.size());

  std::tuple<void*, size_t> item = bufs.front();
  EXPECT_EQ(0, strncmp("Hell", (char*)std::get<0>(item), 4));
  EXPECT_EQ(4, std::get<1>(item));
  bufs.pop_front();
  buffer->mark_size_of_data_consumed(4);  // Flush all contents

  bufs = buffer->get_buffers_ref(8);
  EXPECT_EQ(2, bufs.size());

  item = bufs.front();
  EXPECT_EQ(0, strncmp("o", (char*)std::get<0>(item), 1));
  EXPECT_EQ(1, std::get<1>(item));
  bufs.pop_front();

  item = bufs.front();
  EXPECT_EQ(0, strncmp("Seagate", (char*)std::get<0>(item), 7));
  EXPECT_EQ(7, std::get<1>(item));
  bufs.pop_front();

  buffer->mark_size_of_data_consumed(8);  // Flush all contents
  EXPECT_EQ(0, buffer->length());
}

// Another Non Boundary check
TEST_F(S3AsyncBufferContainerTest,
       DataInTransitAndAskingForLessReturnsValidData2) {
  buffer->add_content(get_evbuf_t_with_data("Hello"));    // len = 5
  buffer->add_content(get_evbuf_t_with_data("Seagate"));  // len = 7
  EXPECT_EQ(5 + 7, buffer->length());
  EXPECT_FALSE(buffer->is_freezed());

  std::deque<std::tuple<void*, size_t> > bufs = buffer->get_buffers_ref(8);
  EXPECT_EQ(2, bufs.size());

  std::tuple<void*, size_t> item = bufs.front();
  EXPECT_EQ(0, strncmp("Hello", (char*)std::get<0>(item), 5));
  EXPECT_EQ(5, std::get<1>(item));
  bufs.pop_front();

  item = bufs.front();
  EXPECT_EQ(0, strncmp("Sea", (char*)std::get<0>(item), 3));
  EXPECT_EQ(3, std::get<1>(item));
  bufs.pop_front();
  buffer->mark_size_of_data_consumed(8);  // Flush contents

  bufs = buffer->get_buffers_ref(4);
  EXPECT_EQ(1, bufs.size());

  item = bufs.front();
  EXPECT_EQ(0, strncmp("gate", (char*)std::get<0>(item), 4));
  EXPECT_EQ(4, std::get<1>(item));
  bufs.pop_front();

  buffer->mark_size_of_data_consumed(4);  // Flush all contents
  EXPECT_EQ(0, buffer->length());
}

TEST_F(S3AsyncBufferContainerTest,
       AllDataReceivedAndAskingForMoreReturnsAllAvailable) {
  buffer->add_content(get_evbuf_t_with_data("Hello"));    // len = 5
  buffer->add_content(get_evbuf_t_with_data("Seagate"));  // len = 7
  buffer->freeze();

  EXPECT_EQ(5 + 7, buffer->length());
  EXPECT_TRUE(buffer->is_freezed());

  std::deque<std::tuple<void*, size_t> > bufs = buffer->get_buffers_ref(20);
  EXPECT_EQ(2, bufs.size());

  std::tuple<void*, size_t> item = bufs.front();
  EXPECT_EQ(0, strncmp("Hello", (char*)std::get<0>(item), 5));
  EXPECT_EQ(5, std::get<1>(item));
  bufs.pop_front();

  item = bufs.front();
  EXPECT_EQ(0, strncmp("Seagate", (char*)std::get<0>(item), 7));
  EXPECT_EQ(7, std::get<1>(item));
  bufs.pop_front();

  buffer->mark_size_of_data_consumed(5 + 7);  // Flush all contents
  EXPECT_EQ(0, buffer->length());
}
