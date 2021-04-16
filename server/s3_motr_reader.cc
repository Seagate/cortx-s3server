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

#include <unistd.h>
#include "s3_common.h"

#include "s3_motr_reader.h"
#include "s3_motr_rw_common.h"
#include "s3_option.h"
#include "s3_uri_to_motr_oid.h"
#include "s3_addb.h"

extern struct m0_realm motr_uber_realm;
extern std::set<struct s3_motr_op_context *> global_motr_object_ops_list;
extern std::set<struct s3_motr_obj_context *> global_motr_obj;
extern int shutdown_motr_teardown_called;

S3MotrReader::S3MotrReader(std::shared_ptr<RequestObject> req,
                           struct m0_uint128 id, int layoutid,
                           struct m0_fid pvid,
                           std::shared_ptr<MotrAPI> motr_api)
    : request(std::move(req)), oid(id), pvid(pvid), layout_id(layoutid) {

  request_id = request->get_request_id();
  stripped_request_id = request->get_stripped_request_id();

  s3_log(S3_LOG_DEBUG, request_id, "%s Ctor\n", __func__);

  s3_motr_api =
      motr_api ? std::move(motr_api) : std::make_shared<ConcreteMotrAPI>();
  motr_unit_size =
      S3MotrLayoutMap::get_instance()->get_unit_size_for_layout(layout_id);
}

S3MotrReader::~S3MotrReader() { clean_up_contexts(); }

void S3MotrReader::clean_up_contexts() {
  // op contexts need to be free'ed before object
  open_context = nullptr;
  reader_context = nullptr;
  if (!shutdown_motr_teardown_called) {
    global_motr_obj.erase(obj_ctx);
    if (obj_ctx) {
      for (size_t i = 0; i < obj_ctx->n_initialized_contexts; i++) {
        s3_motr_api->motr_obj_fini(&obj_ctx->objs[i]);
      }
      free_obj_context(obj_ctx);
      obj_ctx = nullptr;
    }
  }
}

bool S3MotrReader::read_object_data(size_t num_of_blocks,
                                    std::function<void(void)> on_success,
                                    std::function<void(void)> on_failed) {
  s3_log(S3_LOG_INFO, stripped_request_id,
         "%s Entry with num_of_blocks = %zu from last_index = %zu\n", __func__,
         num_of_blocks, last_index);

  bool rc = true;
  state = S3MotrReaderOpState::reading;
  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;

  assert(this->handler_on_success != NULL);
  assert(this->handler_on_failed != NULL);

  num_of_blocks_to_read = num_of_blocks;

  if (is_object_opened) {
    rc = read_object();
  } else {
    int retcode =
        open_object(std::bind(&S3MotrReader::open_object_successful, this),
                    std::bind(&S3MotrReader::open_object_failed, this));
    if (retcode != 0) {
      this->handler_on_failed();
      rc = false;
    }
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
  return rc;
}

bool S3MotrReader::check_object_exist(std::function<void(void)> on_success,
                                      std::function<void(void)> on_failed) {
  bool rc = true;
  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;

  assert(this->handler_on_success != NULL);
  assert(this->handler_on_failed != NULL);

  if (is_object_opened) {
    this->handler_on_success();
  } else {
    int retcode =
        open_object(std::bind(&S3MotrReader::open_object_successful, this),
                    std::bind(&S3MotrReader::open_object_failed, this));
    if (retcode != 0) {
      rc = false;
    }
  }

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
  return rc;
}

int S3MotrReader::open_object(std::function<void(void)> on_success,
                              std::function<void(void)> on_failed) {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  int rc = 0;

  is_object_opened = false;

  // Reader always deals with one object
  if (obj_ctx) {
    // clean up any old allocations
    clean_up_contexts();
  }
  obj_ctx = create_obj_context(1);

  open_context.reset(
      new S3MotrReaderContext(request, on_success, on_failed, layout_id));

  struct s3_motr_context_obj *op_ctx = (struct s3_motr_context_obj *)calloc(
      1, sizeof(struct s3_motr_context_obj));

  op_ctx->op_index_in_launch = 0;
  op_ctx->application_context = (void *)open_context.get();
  struct s3_motr_op_context *ctx = open_context->get_motr_op_ctx();

  ctx->cbs[0].oop_executed = NULL;
  ctx->cbs[0].oop_stable = s3_motr_op_stable;
  ctx->cbs[0].oop_failed = s3_motr_op_failed;

  s3_motr_api->motr_obj_init(&obj_ctx->objs[0], &motr_uber_realm, &oid,
                             layout_id);
  obj_ctx->n_initialized_contexts = 1;
  memcpy(&obj_ctx->objs->ob_attr.oa_pver, &pvid, sizeof(struct m0_fid));

  rc = s3_motr_api->motr_entity_open(&(obj_ctx->objs[0].ob_entity),
                                     &(ctx->ops[0]));
  if (rc != 0) {
    s3_log(S3_LOG_WARN, request_id,
           "Motr API: motr_entity_open failed with error code %d\n", rc);
    state = S3MotrReaderOpState::failed_to_launch;
    s3_motr_op_pre_launch_failure(op_ctx->application_context, rc);
    return rc;
  }

  ctx->ops[0]->op_datum = (void *)op_ctx;
  s3_motr_api->motr_op_setup(ctx->ops[0], &ctx->cbs[0], 0);

  s3_log(S3_LOG_INFO, stripped_request_id,
         "Motr API: openobj(oid: ("
         "%" SCNx64 " : %" SCNx64 "))\n",
         oid.u_hi, oid.u_lo);

  s3_motr_api->motr_op_launch(request->addb_request_id, ctx->ops, 1,
                              MotrOpType::openobj);
  global_motr_object_ops_list.insert(ctx);
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
  return rc;
}

void S3MotrReader::open_object_successful() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_log(S3_LOG_INFO, stripped_request_id,
         "Motr API Successful: openobj(oid: ("
         "%" SCNx64 " : %" SCNx64 "))\n",
         oid.u_hi, oid.u_lo);
  is_object_opened = true;

  if (state == S3MotrReaderOpState::reading) {
    if (!read_object()) {
      // read cannot be launched, out-of-memory
      if (state != S3MotrReaderOpState::failed_to_launch) {
        this->handler_on_failed();
      }
    }
    s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
  } else {
    this->handler_on_success();
  }
}

void S3MotrReader::open_object_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (state != S3MotrReaderOpState::failed_to_launch) {
    s3_log(S3_LOG_DEBUG, request_id, "errno = %d\n",
           open_context->get_errno_for(0));
    is_object_opened = false;
    if (open_context->get_errno_for(0) == -ENOENT) {
      state = S3MotrReaderOpState::missing;
      s3_log(S3_LOG_DEBUG, request_id, "Object doesn't exists\n");
    } else {
      state = S3MotrReaderOpState::failed;
      s3_log(S3_LOG_ERROR, request_id, "Object initialization failed\n");
    }
  }
  this->handler_on_failed();

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

bool S3MotrReader::read_object() {
  int rc;
  s3_log(S3_LOG_INFO, stripped_request_id,
         "%s Entry with num_of_blocks_to_read = %zu from last_index = %zu\n",
         __func__, num_of_blocks_to_read, last_index);

  assert(is_object_opened);

  reader_context.reset(new S3MotrReaderContext(
      request, std::bind(&S3MotrReader::read_object_successful, this),
      std::bind(&S3MotrReader::read_object_failed, this), layout_id));

  /* Read the requisite number of blocks from the entity */
  if (!reader_context->init_read_op_ctx(request_id, num_of_blocks_to_read,
                                        motr_unit_size, &last_index)) {
    // out-of-memory
    state = S3MotrReaderOpState::ooo;
    s3_log(S3_LOG_ERROR, request_id,
           "Motr API failed: openobj(oid: ("
           "%" SCNx64 " : %" SCNx64 "), out-of-memory)\n",
           oid.u_hi, oid.u_lo);
    return false;
  }

  struct s3_motr_op_context *ctx = reader_context->get_motr_op_ctx();
  struct s3_motr_rw_op_context *rw_ctx = reader_context->get_motr_rw_op_ctx();
  // Remember, so buffers can be iterated.
  motr_rw_op_context = rw_ctx;
  iteration_index = 0;

  struct s3_motr_context_obj *op_ctx = (struct s3_motr_context_obj *)calloc(
      1, sizeof(struct s3_motr_context_obj));

  op_ctx->op_index_in_launch = 0;
  op_ctx->application_context = (void *)reader_context.get();

  ctx->cbs[0].oop_executed = NULL;
  ctx->cbs[0].oop_stable = s3_motr_op_stable;
  ctx->cbs[0].oop_failed = s3_motr_op_failed;

  /* Create the read request */
  rc = s3_motr_api->motr_obj_op(&obj_ctx->objs[0], M0_OC_READ, rw_ctx->ext,
                                rw_ctx->data, rw_ctx->attr, 0, M0_OOF_NOHOLE,
                                &ctx->ops[0]);
  if (rc != 0) {
    s3_log(S3_LOG_WARN, request_id,
           "Motr API: motr_obj_op failed with error code %d\n", rc);
    state = S3MotrReaderOpState::failed_to_launch;
    s3_motr_op_pre_launch_failure(op_ctx->application_context, rc);
    return false;
  }

  ctx->ops[0]->op_datum = (void *)op_ctx;
  s3_motr_api->motr_op_setup(ctx->ops[0], &ctx->cbs[0], 0);

  reader_context->start_timer_for("read_object_data");

  s3_log(S3_LOG_INFO, stripped_request_id,
         "Motr API: readobj(operation: M0_OC_READ, oid: ("
         "%" SCNx64 " : %" SCNx64 "))\n",
         oid.u_hi, oid.u_lo);

  s3_motr_api->motr_op_launch(request->addb_request_id, ctx->ops, 1,
                              MotrOpType::readobj);
  global_motr_object_ops_list.insert(ctx);
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
  return true;
}

void S3MotrReader::read_object_successful() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_log(S3_LOG_INFO, stripped_request_id,
         "Motr API Successful: readobj(oid: ("
         "%" SCNx64 " : %" SCNx64 "))\n",
         oid.u_hi, oid.u_lo);
  // see also similar code in S3MotrWiter::write_content()
  if (s3_di_fi_is_enabled("di_data_corrupted_on_read")) {
    struct s3_motr_rw_op_context *rw_ctx = reader_context->get_motr_rw_op_ctx();
    struct m0_bufvec *bv = rw_ctx->data;
    if (rw_ctx->ext->iv_index[0] == 0) {
      char first_byte = *(char *)bv->ov_buf[0];
      s3_log(S3_LOG_DEBUG, "", "%s first_byte=%d", __func__, first_byte);
      switch (first_byte) {
        case 'Z':  // zero
          corrupt_fill_zero = true;
          break;
        case 'F':  // first
          // corrupt the first byte
          *(char *)bv->ov_buf[0] = 0;
          break;
        case 'K':  // OK
          break;
      }
    }
    if (corrupt_fill_zero) {
      for (uint32_t i = 0; i < bv->ov_vec.v_nr; ++i)
        memset(bv->ov_buf[i], 0, bv->ov_vec.v_count[i]);
    }
  }
  state = S3MotrReaderOpState::success;
  this->handler_on_success();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3MotrReader::read_object_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_log(S3_LOG_DEBUG, request_id, "errno = %d\n",
         reader_context->get_errno_for(0));
  if (reader_context->get_errno_for(0) == -ENOENT) {
    s3_log(S3_LOG_DEBUG, request_id, "Object doesn't exist\n");
    state = S3MotrReaderOpState::missing;
  } else {
    s3_log(S3_LOG_ERROR, request_id, "Reading of object failed\n");
    state = S3MotrReaderOpState::failed;
  }
  this->handler_on_failed();
}

// Returns size of data in first block and 0 if there is no content,
// and content in data.
size_t S3MotrReader::get_first_block(char **data) {
  iteration_index = 0;
  return get_next_block(data);
}

// Returns size of data in next block and -1 if there is no content or done
size_t S3MotrReader::get_next_block(char **data) {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_log(S3_LOG_DEBUG, request_id,
         "num_of_blocks_to_read = %zu from iteration_index = %zu\n",
         num_of_blocks_to_read, iteration_index);
  size_t data_read = 0;
  if (iteration_index == num_of_blocks_to_read) {
    s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
    return 0;
  }

  *data = (char *)motr_rw_op_context->data->ov_buf[iteration_index];
  data_read = motr_rw_op_context->data->ov_vec.v_count[iteration_index];
  iteration_index++;

  size_t length = data_read;
  // Data is being read from Motr with at least 4KiB granularity.
  // Actual data checksum has to be calculated for might be smaller.
  // Example: object size = 1 byte, total_size_read would be 4096.
  if (total_size_to_read < total_size_read + length) {
    length = total_size_to_read - total_size_read;
  }
  if (multipart_part_size == 0) {
    // non-multipart-upload case. Just calculate md5 of the entire object.
    md5crypt.Update(*data, length);
  } else {
    // the object was created with multipart upload.
    // Calculate md5 checksums for each part independently.
    //
    // Multipart upload is done in the following way: parts are uploaded one by
    // one, then there is CompleteMultipartUpload call that is only about
    // changing the metadata. To calculate the entire object md5 checksum we
    // would have to read the entire object during CompleteMultipartUpload.
    // There may be a hash that allows to calculate full hash using partial
    // hashes, but md5 checksums were already implemented in the code for PUT
    // and UploadPart, so sticking with md5 checksums was the easiest way to
    // go.
    size_t remaining = length;
    // Each iteration jumps to the next part or the end of the buffer,
    // whichever comes first.
    while (remaining > 0) {
      // current absolute position in object for the purpose of checksumming
      size_t pos = total_size_read + length - remaining;
      size_t next_part_pos;
      // Current implementation supports only equal sized parts, except the
      // last part. This allows to store only a part size (for all parts except
      // the last) and total number of parts. Part boundaries could be derived
      // from these 2 values.
      // The following 'if' gets absolute position in object of the next part
      // boundary.
      if (pos >= multipart_part_size * (multipart_num_of_parts - 1)) {
        // pos is over the boundary between last and last but one part.
        // The part boundary is the end of object in this case.
        next_part_pos = total_size_to_read;
      } else {
        // get the next part boundary by getting the next part index and
        // multiplying it on the part size
        next_part_pos = (pos / multipart_part_size + 1) * multipart_part_size;
      }
      // let's update hash as much data as we can, but don't cross part
      // boundary.
      size_t to_update = std::min(next_part_pos - pos, remaining);
      md5crypt.Update(&(*data)[length - remaining], to_update);
      // if the part boundary is reached add hash to awsetag. It would
      // calculate the hash in exactly the same way as it's done in
      // S3PostCompleteAction::validate_parts() during CompleteMultipartUpload.
      if (pos + to_update == next_part_pos) {
        std::string s = get_content_md5();
        awsetag.add_part_etag(s);
        md5crypt.Reset();
      }
      remaining -= to_update;
    }
  }
  total_size_read += length;

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
  return data_read;
}

S3BufferSequence S3MotrReader::extract_blocks_read() {

  S3BufferSequence buffer_sequence;
  auto *const bufvec = motr_rw_op_context->data;

  assert(bufvec != nullptr);
  assert(!iteration_index);
  assert(bufvec->ov_vec.v_nr);
  s3_log(S3_LOG_DEBUG, request_id,
         "Number of ev buffers read is %u, each of size [%zu]\n",
         bufvec->ov_vec.v_nr, (size_t)bufvec->ov_vec.v_count[0]);

  for (; iteration_index < bufvec->ov_vec.v_nr; ++iteration_index) {
    buffer_sequence.emplace_back(bufvec->ov_buf[iteration_index],
                                 bufvec->ov_vec.v_count[iteration_index]);
    s3_log(S3_LOG_DEBUG, request_id,
           "From Motr buffer: Address = %p, len of buffer = %zu\n",
           bufvec->ov_buf[iteration_index],
           (size_t)bufvec->ov_vec.v_count[iteration_index]);
  }
  motr_rw_op_context->allocated_bufs = false;

  return buffer_sequence;
}
