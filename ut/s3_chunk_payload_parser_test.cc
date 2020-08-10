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

#include "s3_chunk_payload_parser.h"
#include "gtest/gtest.h"
#include "s3_option.h"

extern S3Option* g_option_instance;

// To use a test fixture, derive a class from testing::Test.
class S3ChunkPayloadParserTest : public testing::Test {
 protected:  // You should make the members protected s.t. they can be
             // accessed from sub-classes.

  S3ChunkPayloadParserTest()
      : nfourk_buffer(g_option_instance->get_libevent_pool_buffer_size(), 'A') {
    parser = new S3ChunkPayloadParser();
  }

  ~S3ChunkPayloadParserTest() { delete parser; }

  evbuf_t* allocate_evbuf() {
    evbuf_t* buffer = evbuffer_new();
    // Lets just preallocate space in evbuf to max we intend
    evbuffer_expand(buffer, g_option_instance->get_libevent_pool_buffer_size());
    return buffer;
  }

  // A helper functions
  // Returns an evbuf_t with given data. Caller to manage memory
  evbuf_t* get_evbuf_t_with_data(std::string content) {
    evbuf_t* buf = allocate_evbuf();
    evbuffer_add(buf, content.c_str(), content.length());
    return buf;
  }

  // gets the internal data pointer with length from evbuf_t
  const char* get_datap_4_evbuf_t(evbuf_t* buf) {
    return (const char*)evbuffer_pullup(buf, evbuffer_get_length(buf));
  }

  // Declares the variables your tests want to use.
  S3ChunkPayloadParser* parser;
  std::string nfourk_buffer;
};

TEST_F(S3ChunkPayloadParserTest, HasProperInitState) {
  EXPECT_EQ(ChunkParserState::c_start, parser->get_state());
  EXPECT_FALSE(parser->is_chunk_detail_ready());
  EXPECT_TRUE(parser->current_chunk_size.empty());
  EXPECT_EQ(0, parser->chunk_data_size_to_read);
  EXPECT_EQ(0, parser->content_length);
  EXPECT_TRUE(parser->current_chunk_signature.empty());
  EXPECT_TRUE(parser->chunk_details.empty());
  EXPECT_TRUE(parser->ready_buffers.empty());
  EXPECT_EQ(1, parser->spare_buffers.size());
}

TEST_F(S3ChunkPayloadParserTest, AddToSpareBufferEqualToSpareSize) {
  parser->add_to_spare(nfourk_buffer.c_str(), nfourk_buffer.length());
  // Allocate spare like parser::run()
  parser->spare_buffers.push_back(allocate_evbuf());

  parser->add_to_spare(nfourk_buffer.c_str(), nfourk_buffer.length());

  EXPECT_EQ(1, parser->spare_buffers.size());
  EXPECT_EQ(1, parser->ready_buffers.size());

  EXPECT_EQ(nfourk_buffer.length(),
            evbuffer_get_length(parser->ready_buffers.front()));
  EXPECT_EQ(nfourk_buffer.length(),
            evbuffer_get_length(parser->spare_buffers.front()));
}

TEST_F(S3ChunkPayloadParserTest, AddToSpareBufferNotEqualToStdSpareSize) {
  parser->add_to_spare(nfourk_buffer.c_str(), nfourk_buffer.length());
  // Allocate spare like parser::run()
  parser->spare_buffers.push_back(allocate_evbuf());

  parser->add_to_spare("ABCD", 4);

  EXPECT_EQ(1, parser->spare_buffers.size());
  EXPECT_EQ(1, parser->ready_buffers.size());

  EXPECT_EQ(nfourk_buffer.length(),
            evbuffer_get_length(parser->ready_buffers.front()));
  EXPECT_EQ(4, evbuffer_get_length(parser->spare_buffers.front()));
}

TEST_F(S3ChunkPayloadParserTest, AddToSpareBufferNotEqualToStdSpareSize1) {
  parser->add_to_spare(nfourk_buffer.c_str(), nfourk_buffer.length() - 10);
  // Allocate spare like parser::run()
  parser->spare_buffers.push_back(allocate_evbuf());

  parser->add_to_spare(nfourk_buffer.c_str(), nfourk_buffer.length());

  EXPECT_EQ(1, parser->spare_buffers.size());
  EXPECT_EQ(1, parser->ready_buffers.size());

  EXPECT_EQ(nfourk_buffer.length(),
            evbuffer_get_length(parser->ready_buffers.front()));
  EXPECT_EQ(nfourk_buffer.length() - 10,
            evbuffer_get_length(parser->spare_buffers.front()));
}

TEST_F(S3ChunkPayloadParserTest, AddToSpareBufferNotEqualToStdSpareSize2) {
  parser->add_to_spare(nfourk_buffer.c_str(), nfourk_buffer.length() - 10);

  // Allocate spare like parser::run()
  parser->spare_buffers.push_back(allocate_evbuf());
  parser->add_to_spare(nfourk_buffer.c_str(), nfourk_buffer.length());

  // Allocate spare like parser::run()
  parser->spare_buffers.push_back(allocate_evbuf());
  parser->add_to_spare(nfourk_buffer.c_str(), nfourk_buffer.length());

  EXPECT_EQ(1, parser->spare_buffers.size());
  EXPECT_EQ(2, parser->ready_buffers.size());

  EXPECT_EQ(nfourk_buffer.length(),
            evbuffer_get_length(parser->ready_buffers.front()));
  parser->ready_buffers.pop_front();
  EXPECT_EQ(nfourk_buffer.length(),
            evbuffer_get_length(parser->ready_buffers.front()));

  EXPECT_EQ(nfourk_buffer.length() - 10,
            evbuffer_get_length(parser->spare_buffers.front()));
}
