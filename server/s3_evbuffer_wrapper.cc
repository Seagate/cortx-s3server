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

#include "s3_evbuffer_wrapper.h"
#include "s3_motr_context.h"

// Create evbuffer of size buf_sz, with each basic buffer buf_unit_sz
S3Evbuffer::S3Evbuffer(const std::string req_id, size_t buf_sz,
                       int buf_unit_sz) {
  p_evbuf = evbuffer_new();
  assert(p_evbuf);
  assert(buf_unit_sz);
  // example, if buf_unit_sz = 16K, total buf size = 8mb, then nvecs = 8mb/16k
  nvecs = (buf_sz + (buf_unit_sz - 1)) / buf_unit_sz;
  total_size = buf_sz;
  buffer_unit_sz = buf_unit_sz;
  request_id = req_id;
  vec = nullptr;
}

int S3Evbuffer::init() {
  vec = (struct evbuffer_iovec*)calloc(nvecs, sizeof(struct evbuffer_iovec));
  if (vec == nullptr) {
    s3_log(S3_LOG_ERROR, request_id, "memory allocation failure\n");
    return -ENOMEM;
  }
  for (size_t i = 0; i < nvecs; i++) {
    // Reserve buf_sz bytes memory
    int no_of_extends =
        evbuffer_reserve_space(p_evbuf, buffer_unit_sz, &vec[i], 1);
    if (no_of_extends < 0) {
      s3_log(S3_LOG_ERROR, request_id,
             "evbuffer_reserve_space failed, no_of_extends = %d\n",
             no_of_extends);
      return no_of_extends;
    }
    if (evbuffer_commit_space(p_evbuf, &vec[i], 1) < 0) {
      s3_log(S3_LOG_ERROR, request_id,
             "evbuffer_commit_space failed i = %zu, nvecs = %zu\n", i, nvecs);
      return -1;
    }
  }
  return 0;
}

// Setup pointers in motr structs so that buffers can be passed to motr.
// No references will be help within object to any rw_ctx members.
// Caller will be responsible not to free any pointers returned as these are
// owned by evbuffer
void S3Evbuffer::to_motr_read_buffers(struct s3_motr_rw_op_context* rw_ctx,
                                      uint64_t* last_index) {
  // Code similar to S3ClovisWriter::set_up_motr_data_buffers but not same
  for (size_t i = 0; i < nvecs && total_size > 0; ++i) {
    rw_ctx->data->ov_buf[i] = vec[i].iov_base;
    size_t len = vec[i].iov_len;
    rw_ctx->data->ov_vec.v_count[i] = len;
    // Init motr buffer attrs.
    rw_ctx->ext->iv_index[i] = *last_index;
    rw_ctx->ext->iv_vec.v_count[i] = len;
    *last_index += len;

    /* we don't want any attributes */
    rw_ctx->attr->ov_vec.v_count[i] = 0;
  }
}

// Releases ownership of evbuffer, caller needs to ensure evbuffer is freed
// later point of time by owning a reference returned.
struct evbuffer* S3Evbuffer::release_ownership() {
  struct evbuffer* tmp = p_evbuf;
  p_evbuf = NULL;
  return tmp;
}

int S3Evbuffer::drain_data(size_t start_offset) {
  int rc;
  if (p_evbuf == NULL) {
    return -1;
  }
  rc = evbuffer_drain(p_evbuf, start_offset);
  if (rc != 0) {
    s3_log(S3_LOG_ERROR, request_id,
           "Error moving start offset to non-zero as per requested range");
  }
  return rc;
}

size_t S3Evbuffer::get_evbuff_length() {
  if (p_evbuf == nullptr) {
    return 0;
  }
  return evbuffer_get_length(p_evbuf);
}

void S3Evbuffer::read_drain_data_from_buffer(size_t length) {
  struct evbuffer* new_evbuf_body = evbuffer_new();
  int moved_bytes = evbuffer_remove_buffer(p_evbuf, new_evbuf_body, length);
  evbuffer_free(p_evbuf);
  if (moved_bytes != (int)length) {
    s3_log(S3_LOG_ERROR, request_id,
           "Error populating new_evbuf_body: len_to_send_to_client(%zd), "
           "len_in_reply_evbuffer(%d).\n",
           length, moved_bytes);
  }
  p_evbuf = new_evbuf_body;
}
