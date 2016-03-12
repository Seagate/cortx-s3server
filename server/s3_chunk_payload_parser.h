/*
 * COPYRIGHT 2016 SEAGATE LLC
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
 * Original creation date: 15-Mar-2016
 */

#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_CHUNK_PAYLOAD_PARSER_H__
#define __MERO_FE_S3_SERVER_S3_CHUNK_PAYLOAD_PARSER_H__

#include <evhtp.h>

#include <string>
#include <memory>
#include <vector>
#include <queue>

#include "s3_sha256.h"

#define LF               (unsigned char)10
#define CR               (unsigned char)13

#define S3_AWS_CHUNK_KEY {'c', 'h', 'u', 'n', 'k', '-', \
                       's', 'i', 'g', 'n', 'a', 't', 'u', 'r', 'e'}


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
  void incr_chunk_number() {
    chunk_number++;
  }
  void debug_dump();

  // preferably Call in sequence to complete the details
  void add_size(size_t size);
  void add_signature(const std::string& sign);
  bool update_hash(evbuf_t *buf);
  bool fini_hash();

  size_t get_size();
  const std::string& get_signature();
  std::string get_payload_hash();
  bool is_ready();
};

// http://docs.aws.amazon.com/AmazonS3/latest/API/sigv4-streaming.html
// Parsing Syntax:
// string(IntHexBase(chunk-size)) + ";chunk-signature=" + signature + \r\n + chunk-data + \r\n
// State within each chunk
enum class ChunkParserState {
    c_start = 0,
    c_chunk_size,
    c_chunk_signature_key,  // "chunk-signature"
    c_chunk_signature_value,
    c_cr,  // carriage return CR char
    c_chunk_data,
    c_chunk_data_end_cr,
    c_error
};

class S3ChunkPayloadParser {
  std::queue<S3ChunkDetail> chunk_details;
  ChunkParserState parser_state;
  std::string current_chunk_size;
  size_t chunk_data_size_to_read;
  std::string current_chunk_signature;
  S3ChunkDetail current_chunk_detail;

  const std::queue<char> chunk_sig_key_q_const;
  std::queue<char> chunk_sig_key_char_state;  // init has val = chunk-signature, pops on each char encountered

  void reset_parser_state();

public:
  S3ChunkPayloadParser();

  // For each buf passed, it strips the chunk-size and signature
  // from payload and returns the actual payload buffers.
  // It also splits the input buffer into 2 if the buf contains
  // parts of 2 incoming chunks.
  std::vector<evbuf_t *> run(evbuf_t *buf);

  ChunkParserState get_state() {
    return parser_state;
  }

  bool is_chunk_detail_ready();
  S3ChunkDetail pop_chunk_detail();
};

#endif
