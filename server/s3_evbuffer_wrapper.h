/*
 * Copyright (c) 2021 Seagate Technology LLC and/or its Affiliates
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

#ifndef __S3_SERVER_S3_EVBUFFER_WRAPPER_H__
#define __S3_SERVER_S3_EVBUFFER_WRAPPER_H__

#include <evhtp.h>
#include <string>

// Usage example:
//  Say for 32mb read operation with 1 mb unit size object.
//  S3Evbuffer *evbuf = new S3Evbuffer(32 * 1048576/* 32mb */, 1048576 /* 1mb */
// )
//  buf->to_motr_read_buffers(rw_ctx)
//  ... motr will fill data in rw_ctx with async read op ...
//  After read op success follow next steps.
//  evbuf->commit_buffers()
//  struct evbuffer *data = buf->release_ownership();
//  delete evbuf;
//  ...
//  evhtp_obj->http_send_reply_body(ev_req, evbuf);

class S3Evbuffer {
  struct evbuffer *p_evbuf;
  // Next are references to internals of p_evbuf
  // struct evbuffer_iovec *vec;
  struct evbuffer_iovec *vec;
  size_t nvecs;
  size_t total_size;
  size_t buffer_unit_sz;
  std::string request_id;

 public:
  // Create evbuffer of size buf_sz
  S3Evbuffer(std::string request_id, size_t buf_sz, int buf_unit_sz = 16384);

  int init();
  inline unsigned int const get_nvecs() { return nvecs; }
  int const get_no_of_extends();
  // Setup pointers in motr structs so that buffers can be passed to motr.
  // Caller will be responsible not to free any pointers returned as these are
  // owned by evbuffer
  void to_motr_read_buffers(struct s3_motr_rw_op_context *rw_ctx,
                            uint64_t *offset);

  size_t get_evbuff_length();
  void read_drain_data_from_buffer(size_t read_data_start_offset);
  int drain_data(size_t start_offset);
  // Releases ownership of evbuffer, caller needs to ensure evbuffer is freed
  // later point of time by owning a reference returned.
  struct evbuffer *release_ownership();

  ~S3Evbuffer() {
    if (p_evbuf != nullptr) {
      evbuffer_free(p_evbuf);
      p_evbuf = NULL;
    }
  }
};

#endif
