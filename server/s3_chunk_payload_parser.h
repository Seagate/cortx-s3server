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

#ifndef __S3_SERVER_S3_CHUNK_PAYLOAD_PARSER_H__
#define __S3_SERVER_S3_CHUNK_PAYLOAD_PARSER_H__

#include <evhtp.h>

#include <memory>
#include <queue>
#include <string>
#include <vector>

#include <gtest/gtest_prod.h>

#include "s3_sha256.h"

#define LF (unsigned char)10
#define CR (unsigned char)13

#define S3_AWS_CHUNK_KEY \
  { 'c', 'h', 'u', 'n', 'k', '-', 's', 'i', 'g', 'n', 'a', 't', 'u', 'r', 'e' }

class S3ChunkDetail {
  size_t chunk_size;
  std::string signature;
  std::string payload_hash;
  S3sha256 hash_ctx;
  bool ready;  // this is true when all the data is updated in hash
  int chunk_number;

 public:
  S3ChunkDetail();
  void reset();
  void incr_chunk_number() { chunk_number++; }
  void debug_dump();

  // preferably Call in sequence to complete the details
  void add_size(size_t size);
  void add_signature(const std::string &sign);
  bool update_hash(const void *data_ptr, size_t data_len = 0);
  bool fini_hash();

  size_t get_size();
  const std::string &get_signature();
  std::string get_payload_hash();
  bool is_ready();
};

// http://docs.aws.amazon.com/AmazonS3/latest/API/sigv4-streaming.html
// Parsing Syntax:
// string(IntHexBase(chunk-size)) + ";chunk-signature=" + signature + \r\n +
// chunk-data + \r\n
// State within each chunk
enum class ChunkParserState {
  c_start = 0,
  c_chunk_size,
  c_chunk_signature_key,  // "chunk-signature"
  c_chunk_signature_value,
  c_cr,  // carriage return CR char
  c_chunk_data,
  c_chunk_data_end_cr,
  c_chunk_data_end_lf,
  c_error
};

class S3ChunkPayloadParser {
  std::queue<S3ChunkDetail> chunk_details;
  ChunkParserState parser_state;
  std::string current_chunk_size;
  size_t chunk_data_size_to_read;
  size_t content_length;  // actual data embedded in chunks
  std::string current_chunk_signature;
  S3ChunkDetail current_chunk_detail;

  const std::queue<char> chunk_sig_key_q_const;
  std::queue<char> chunk_sig_key_char_state;  // init has val = chunk-signature,
                                              // pops on each char encountered

  void reset_parser_state();

  // We hold a spare buffer block internally to copy chunked data in it
  // and keep rotating with incoming buffers in parser. Once data in incoming
  // buffer is emptied we retain it as spare and buffer that is filled
  // completely
  // is given out for consumption.
  std::deque<evbuf_t *> spare_buffers;
  std::deque<evbuf_t *> ready_buffers;

  // Adds to current spare buffer. but when spare is full:
  // moves filled spare -> ready
  void add_to_spare(const void *data, size_t len);

 public:
  S3ChunkPayloadParser();
  virtual ~S3ChunkPayloadParser();

  void setup_content_length(size_t len) { content_length = len; }

  // For each buf passed, it strips the chunk-size and signature
  // from payload and creates bufs with filled data.
  // It returns the bufs with data only when entire buf is filled,
  // else it remember the partial data internally and returns in next call
  // *Note: check for parser errors(get_state()) before using return value
  std::deque<evbuf_t *> run(evbuf_t *buf);

  ChunkParserState get_state() { return parser_state; }

  bool is_chunk_detail_ready();
  S3ChunkDetail pop_chunk_detail();

  FRIEND_TEST(S3ChunkPayloadParserTest, HasProperInitState);
  FRIEND_TEST(S3ChunkPayloadParserTest, AddToSpareBufferEqualToSpareSize);
  FRIEND_TEST(S3ChunkPayloadParserTest, AddToSpareBufferNotEqualToStdSpareSize);
  FRIEND_TEST(S3ChunkPayloadParserTest,
              AddToSpareBufferNotEqualToStdSpareSize1);
  FRIEND_TEST(S3ChunkPayloadParserTest,
              AddToSpareBufferNotEqualToStdSpareSize2);
};

#endif
