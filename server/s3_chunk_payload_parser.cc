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

#include <stdlib.h>

#include "s3_chunk_payload_parser.h"
#include "s3_iem.h"
#include "s3_log.h"

S3ChunkDetail::S3ChunkDetail() : chunk_size(0), ready(false), chunk_number(0) {}

void S3ChunkDetail::reset() {
  ready = false;
  chunk_size = 0;
  signature = "";
  payload_hash = "";
  hash_ctx.reset();
}

void S3ChunkDetail::debug_dump() {
  s3_log(S3_LOG_DEBUG,
         "Chunk Details start: chunk_number = [%d], size = [%zu],\n\
         signature = [%s]\nHash = [%s]\nChunk Details end.\n",
         chunk_number, chunk_size, signature.c_str(), payload_hash.c_str());
}

bool S3ChunkDetail::is_ready() { return ready; }

void S3ChunkDetail::add_size(size_t size) { chunk_size = size; }

void S3ChunkDetail::add_signature(const std::string &sign) { signature = sign; }

bool S3ChunkDetail::update_hash(evbuf_t *buf) {
  if (buf == NULL) {
    std::string emptee_string = "";
    bool status =
        hash_ctx.Update(emptee_string.c_str(), emptee_string.length());
    return status;
  }

  size_t len_in_buf = evbuffer_get_length(buf);
  size_t num_of_extents = evbuffer_peek(buf, len_in_buf, NULL, NULL, 0);
  bool status = false;

  /* do the actual peek */
  struct evbuffer_iovec *vec_in = (struct evbuffer_iovec *)malloc(
      num_of_extents * sizeof(struct evbuffer_iovec));

  /* do the actual peek at data */
  evbuffer_peek(buf, len_in_buf, NULL /*start of buffer*/, vec_in,
                num_of_extents);

  for (size_t i = 0; i < num_of_extents; i++) {
    status =
        hash_ctx.Update((const char *)vec_in[i].iov_base, vec_in[i].iov_len);
    if (!status) {
      break;
    }
  }
  free(vec_in);

  return status;
}

bool S3ChunkDetail::fini_hash() {
  bool status = hash_ctx.Finalize();
  if (status) {
    payload_hash = hash_ctx.get_hex_hash();
    ready = true;
  }
  return status;
}

size_t S3ChunkDetail::get_size() { return chunk_size; }

const std::string &S3ChunkDetail::get_signature() { return signature; }

std::string S3ChunkDetail::get_payload_hash() { return payload_hash; }

S3ChunkPayloadParser::S3ChunkPayloadParser()
    : parser_state(ChunkParserState::c_start),
      chunk_data_size_to_read(0),
      chunk_sig_key_q_const(S3_AWS_CHUNK_KEY),
      chunk_sig_key_char_state(S3_AWS_CHUNK_KEY) {
  s3_log(S3_LOG_DEBUG, "Constructor\n");
}

void S3ChunkPayloadParser::reset_parser_state() {
  chunk_data_size_to_read = 0;
  current_chunk_size = "";
  current_chunk_signature = "";
  chunk_sig_key_char_state = chunk_sig_key_q_const;
  current_chunk_detail.reset();
  current_chunk_detail.incr_chunk_number();
}

std::vector<evbuf_t *> S3ChunkPayloadParser::run(evbuf_t *buf) {
  std::vector<evbuf_t *> return_val;
  bool remember_to_drain_cr = false;
  bool reset_parsing = false;

  while (true) {
    size_t len_in_buf = evbuffer_get_length(buf);
    s3_log(S3_LOG_DEBUG, "Parsing evbuffer_get_length(buf) = %zu\n",
           evbuffer_get_length(buf));

    size_t remaining_to_parse = len_in_buf;
    size_t num_of_extents = evbuffer_peek(buf, len_in_buf, NULL, NULL, 0);

    /* do the actual peek */
    struct evbuffer_iovec *vec_in = (struct evbuffer_iovec *)malloc(
        num_of_extents * sizeof(struct evbuffer_iovec));

    /* do the actual peek at data */
    evbuffer_peek(buf, len_in_buf, NULL /*start of buffer*/, vec_in,
                  num_of_extents);
    size_t meta_read_in_current_parse = 0;
    s3_log(S3_LOG_DEBUG, "num_of_extents = %zu\n", num_of_extents);
    for (size_t i = 0; i < num_of_extents; i++) {
      // std::string chunk_debug((const char*)vec_in[i].iov_base,
      // vec_in[i].iov_len);
      // s3_log(S3_LOG_DEBUG, "vec_in[i].iov_base = %s\n", chunk_debug.c_str());
      // s3_log(S3_LOG_DEBUG, "vec_in[i].iov_len = %zu\n", vec_in[i].iov_len);

      for (size_t j = 0; j < vec_in[i].iov_len; j++) {
        // Parsing Syntax:
        // string(IntHexBase(chunk-size)) + ";chunk-signature=" + signature +
        // \r\n + chunk-data + \r\n
        if (parser_state == ChunkParserState::c_start) {
          reset_parser_state();
          reset_parsing = false;
          parser_state = ChunkParserState::c_chunk_size;
        }
        switch (parser_state) {
          case ChunkParserState::c_error: {
            s3_log(S3_LOG_ERROR, "ChunkParserState::c_error.\n");
            evbuffer_drain(buf, -1);
            evbuffer_free(buf);
            free(vec_in);
            return return_val;
          }
          case ChunkParserState::c_start: {
            // s3_log(S3_LOG_DEBUG, "ChunkParserState::c_start.")
          }  // dummycase will never happen, see 1 line above
          case ChunkParserState::c_chunk_size: {
            // s3_log(S3_LOG_DEBUG, "ChunkParserState::c_chunk_size.")
            const char *chptr = (const char *)vec_in[i].iov_base;

            if (chptr[j] == ';') {
              parser_state = ChunkParserState::c_chunk_signature_key;
              chunk_sig_key_char_state = chunk_sig_key_q_const;
              chunk_data_size_to_read =
                  strtol(current_chunk_size.c_str(), NULL, 16);
              current_chunk_detail.add_size(chunk_data_size_to_read);
              s3_log(S3_LOG_DEBUG, "current_chunk_size = [%s]\n",
                     current_chunk_size.c_str());
              s3_log(S3_LOG_DEBUG, "chunk_data_size_to_read (int) = [%zu]\n",
                     chunk_data_size_to_read);

              // ignore the semicolon
            } else {
              current_chunk_size.push_back(chptr[j]);
            }
            meta_read_in_current_parse++;
            break;
          }
          case ChunkParserState::c_chunk_signature_key: {
            // s3_log(S3_LOG_DEBUG, "ChunkParserState::c_chunk_signature_key.")
            const char *chptr = (const char *)vec_in[i].iov_base;

            if (chunk_sig_key_char_state.empty()) {
              // ignore '='
              if (chptr[j] == '=') {
                parser_state = ChunkParserState::c_chunk_signature_value;
              } else {
                parser_state = ChunkParserState::c_error;
                evbuffer_drain(buf, -1);
                evbuffer_free(buf);
                free(vec_in);
                return return_val;
              }
            } else {
              if (chptr[j] == chunk_sig_key_char_state.front()) {
                chunk_sig_key_char_state.pop();
              } else {
                parser_state = ChunkParserState::c_error;
                evbuffer_drain(buf, -1);
                evbuffer_free(buf);
                free(vec_in);
                return return_val;
              }
            }
            meta_read_in_current_parse++;
            break;
          }
          case ChunkParserState::c_chunk_signature_value: {
            // s3_log(S3_LOG_DEBUG,
            // "ChunkParserState::c_chunk_signature_value.")
            const char *chptr = (const char *)vec_in[i].iov_base;
            if ((unsigned char)chptr[j] == CR) {
              parser_state = ChunkParserState::c_cr;
            } else {
              current_chunk_signature.push_back(chptr[j]);
            }
            meta_read_in_current_parse++;
            break;
          }
          case ChunkParserState::c_cr: {
            // s3_log(S3_LOG_DEBUG, "ChunkParserState::c_cr.")
            const char *chptr = (const char *)vec_in[i].iov_base;
            if ((unsigned char)chptr[j] == LF) {
              // CRLF means we are done with signature
              parser_state = ChunkParserState::c_chunk_data;
              current_chunk_detail.add_signature(current_chunk_signature);
              s3_log(S3_LOG_DEBUG, "current_chunk_signature = [%s]\n",
                     current_chunk_signature.c_str());
            } else {
              // what we detected as CR was part of signature, move back
              current_chunk_signature.push_back(CR);
              if ((unsigned char)chptr[j] == CR) {
                parser_state = ChunkParserState::c_cr;
              } else {
                current_chunk_signature.push_back(chptr[j]);
              }
            }
            meta_read_in_current_parse++;
            break;
          }
          case ChunkParserState::c_chunk_data: {
            // s3_log(S3_LOG_DEBUG, "ChunkParserState::c_chunk_data.")
            // we have to read 'chunk_data_size_to_read' data, so we have 3
            // cases.
            // 1. current payload has "all" and "only" data related to current
            // chunk, optionally crlf
            // 2. current payload has "partial" and "only" data related to
            // current chunk.
            // 3. current payload had "all" data of current chunk and some part
            // of next chunk
            if (chunk_data_size_to_read > 0) {
              s3_log(S3_LOG_DEBUG, "len_in_buf = [%zu]\n", len_in_buf);
              s3_log(S3_LOG_DEBUG, "meta_read_in_current_parse = [%zu]\n",
                     meta_read_in_current_parse);

              s3_log(S3_LOG_DEBUG, "Before draining metadata = %zu\n",
                     evbuffer_get_length(buf));
              evbuffer_drain(buf, meta_read_in_current_parse);
              s3_log(S3_LOG_DEBUG, "After draining metadata = %zu\n",
                     evbuffer_get_length(buf));

              remaining_to_parse = len_in_buf - meta_read_in_current_parse;
              s3_log(S3_LOG_DEBUG, "remaining_to_parse = [%zu]\n",
                     remaining_to_parse);
              if (chunk_data_size_to_read == remaining_to_parse) {
                // (==) we have all data, missing crlf
                parser_state = ChunkParserState::c_chunk_data_end_cr;
                return_val.push_back(buf);
                current_chunk_detail.update_hash(buf);
                chunk_data_size_to_read -= evbuffer_get_length(buf);
                free(vec_in);
                return return_val;
              } else if (chunk_data_size_to_read > remaining_to_parse) {
                // (>)  we have partial data, but its all part of data OR
                return_val.push_back(buf);
                current_chunk_detail.update_hash(buf);
                chunk_data_size_to_read -= evbuffer_get_length(buf);
                free(vec_in);
                return return_val;
              } else if (chunk_data_size_to_read + 2 /*CRLF*/ ==
                         remaining_to_parse) {
                // we have all data + crlf
                evbuf_t *current_buf = evbuffer_new();
                evbuffer_remove_buffer(buf, current_buf,
                                       chunk_data_size_to_read);  // copy data
                return_val.push_back(current_buf);
                current_chunk_detail.update_hash(current_buf);
                chunk_data_size_to_read -= evbuffer_get_length(current_buf);
                evbuffer_drain(buf, -1);  // remove CRLF
                evbuffer_free(buf);
                parser_state = ChunkParserState::c_start;
                current_chunk_detail.fini_hash();
                current_chunk_detail.debug_dump();
                chunk_details.push(current_chunk_detail);
                free(vec_in);
                return return_val;
              } else if (chunk_data_size_to_read + 1 /*CR*/ ==
                         remaining_to_parse) {
                // we have all data + cr
                evbuf_t *current_buf = evbuffer_new();
                evbuffer_remove_buffer(buf, current_buf,
                                       chunk_data_size_to_read);
                return_val.push_back(current_buf);
                current_chunk_detail.update_hash(current_buf);
                chunk_data_size_to_read -= evbuffer_get_length(current_buf);
                evbuffer_drain(buf, -1);  // remove CR
                evbuffer_free(buf);

                parser_state = ChunkParserState::c_chunk_data_end_cr;
                free(vec_in);
                return return_val;
              } else if (chunk_data_size_to_read <= remaining_to_parse) {
                // we have all data + CRLF + some part of next chunk
                evbuf_t *current_buf = evbuffer_new();

                // copy/move data from buf to current_buf
                s3_log(S3_LOG_DEBUG,
                       "Before move evbuffer_get_length(buf) = %zu\n",
                       evbuffer_get_length(buf));
                evbuffer_remove_buffer(buf, current_buf,
                                       chunk_data_size_to_read);
                s3_log(S3_LOG_DEBUG,
                       "After move evbuffer_get_length(buf) = %zu\n",
                       evbuffer_get_length(buf));
                s3_log(S3_LOG_DEBUG,
                       "After move evbuffer_get_length(current_buf) = %zu\n",
                       evbuffer_get_length(current_buf));

                evbuffer_drain(buf, 2);  // remove CRLF
                return_val.push_back(current_buf);
                current_chunk_detail.update_hash(current_buf);
                chunk_data_size_to_read -= evbuffer_get_length(current_buf);

                // Now we need to reinit parsing with whatever is in buf
                reset_parsing = true;
                parser_state = ChunkParserState::c_start;
                current_chunk_detail.fini_hash();
                current_chunk_detail.debug_dump();
                chunk_details.push(current_chunk_detail);
              }
            } else {
              // This can be last chunk with size 0
              s3_log(S3_LOG_DEBUG, "Last chunk of size 0\n");
              reset_parsing = true;
              parser_state = ChunkParserState::c_start;
              current_chunk_detail.update_hash(NULL);
              current_chunk_detail.fini_hash();
              current_chunk_detail.debug_dump();
              chunk_details.push(current_chunk_detail);
              evbuffer_drain(buf, -1);
              evbuffer_free(buf);

              free(vec_in);
              return return_val;
            }
            break;
          }
          case ChunkParserState::c_chunk_data_end_cr: {
            // s3_log(S3_LOG_DEBUG, "ChunkParserState::c_chunk_data_end_cr.")
            const char *chptr = (const char *)vec_in[i].iov_base;

            if ((unsigned char)chptr[j] == LF) {
              // CRLF means we are done with data
              parser_state = ChunkParserState::c_start;
              reset_parsing = true;
              if (remember_to_drain_cr) {
                evbuffer_drain(buf, 2);  // remove CRLF
              } else {
                evbuffer_drain(buf, 1);  // remove LF
              }
              remember_to_drain_cr = false;
              current_chunk_detail.fini_hash();
              current_chunk_detail.debug_dump();
              chunk_details.push(current_chunk_detail);
            } else {
              parser_state = ChunkParserState::c_cr;
              remember_to_drain_cr = true;
            }
            break;
          }
          default: { s3_log(S3_LOG_ERROR, "Invalid ChunkParserState.\n"); }
        };  // switch
        if (reset_parsing) {
          break;
        }
      }  // Inner for
      if (reset_parsing) {
        break;
      }
    }  // for num_of_extents
    free(vec_in);
    if (!reset_parsing) {
      break;
    }  // else buf was rearranged, we need to continue parsing pending buf data.
  }    // while true

  if (parser_state < ChunkParserState::c_chunk_data) {
    // Here we just have metadata in buf which is already parsed, so free it.
    evbuffer_drain(buf, -1);
    evbuffer_free(buf);
  } else {
    // We missed adding data to return_val
    // logical error: should never be here.
    s3_log(S3_LOG_ERROR, "Fatal Error: Invalid ChunkParserState.\n");
    s3_iem(LOG_ERR, S3_IEM_CHUNK_PARSING_FAIL, S3_IEM_CHUNK_PARSING_FAIL_STR,
           S3_IEM_CHUNK_PARSING_FAIL_JSON);
  }
  return return_val;
}

/*
 *  <IEM_INLINE_DOCUMENTATION>
 *    <event_code>047002001</event_code>
 *    <application>S3 Server</application>
 *    <submodule>Chunk upload</submodule>
 *    <description>Chunk parsing failed</description>
 *    <audience>Development</audience>
 *    <details>
 *      Logical error occurred during chunk parsing.
 *      The data section of the event has following keys:
 *        time - timestamp.
 *        node - node name.
 *        pid  - process-id of s3server instance, useful to identify logfile.
 *        file - source code filename.
 *        line - line number within file where error occurred.
 *    </details>
 *    <service_actions>
 *      Save the S3 server log files. Contact development team for further
 *      investigation.
 *    </service_actions>
 *  </IEM_INLINE_DOCUMENTATION>
 */

bool S3ChunkPayloadParser::is_chunk_detail_ready() {
  if (!chunk_details.empty() && chunk_details.front().is_ready()) {
    return true;
  }
  return false;
}

// Check if its ready before pop
S3ChunkDetail S3ChunkPayloadParser::pop_chunk_detail() {
  if (chunk_details.front().is_ready()) {
    S3ChunkDetail detail = chunk_details.front();
    chunk_details.pop();
    return detail;
  }
  S3ChunkDetail detail;
  return detail;
}
