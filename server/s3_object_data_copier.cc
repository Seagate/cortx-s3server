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

#include "s3_factory.h"
#include "s3_log.h"
#include "s3_m0_uint128_helper.h"
#include "s3_mem_pool_manager.h"
#include "s3_motr_layout.h"
#include "s3_motr_reader.h"
#include "s3_motr_writer.h"
#include "s3_object_data_copier.h"

S3ObjectDataCopier::S3ObjectDataCopier(
    std::shared_ptr<RequestObject> request_object,
    std::shared_ptr<S3MotrWiter> motr_writer,
    std::shared_ptr<S3MotrReaderFactory> motr_reader_factory,
    std::shared_ptr<MotrAPI> motr_api)
    : request_object(std::move(request_object)),
      motr_writer(std::move(motr_writer)),
      motr_reader_factory(std::move(motr_reader_factory)),
      motr_api(std::move(motr_api)) {

  request_id = this->request_object->get_request_id();
  s3_log(S3_LOG_DEBUG, request_id, "%s Ctor\n", __func__);
}

S3ObjectDataCopier::~S3ObjectDataCopier() {
  if (!data_blocks_writing.empty()) {
    cleanup_blocks_written();
  }
  if (!data_blocks_read.empty()) {
    data_blocks_writing = std::move(data_blocks_read);
    cleanup_blocks_written();
  }
}

void S3ObjectDataCopier::read_data_block() {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);

  assert(!read_in_progress);
  assert(data_blocks_read.empty());
  assert(bytes_left_to_read > 0);

  const auto n_blocks = std::min<size_t>(
      S3Option::get_instance()->get_motr_units_per_request(),
      (bytes_left_to_read + motr_unit_size - 1) / motr_unit_size);

  if (motr_reader->read_object_data(
          n_blocks,
          std::bind(&S3ObjectDataCopier::read_data_block_success, this),
          std::bind(&S3ObjectDataCopier::read_data_block_failed, this))) {

    s3_log(S3_LOG_DEBUG, request_id, "Read of %zu data block is started",
           n_blocks);
    read_in_progress = true;
  } else {
    copy_failed = true;
    s3_log(S3_LOG_ERROR, request_id, "Read of %zu data block failed to start",
           n_blocks);
    set_s3_error(motr_reader->get_state() ==
                         S3MotrReaderOpState::failed_to_launch
                     ? "ServiceUnavailable"
                     : "InternalError");
    if (!write_in_progress) {
      this->on_failure();
    }
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3ObjectDataCopier::read_data_block_success() {

  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);
  s3_log(S3_LOG_INFO, request_id, "Reading a part of data succeeded");

  assert(read_in_progress);
  read_in_progress = false;

  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, nullptr, "Shutdown or rollback");
    return;
  }
  if (copy_failed) {
    assert(!write_in_progress);

    this->on_failure();
    return;
  }
  assert(data_blocks_read.empty());
  data_blocks_read = motr_reader->extract_blocks_read();

  size_t bytes_in_chunk_count = 0;

  if (data_blocks_read.empty()) {
    s3_log(S3_LOG_ERROR, request_id, "Motr reader returned no data");

    copy_failed = true;
    set_s3_error("InternalError");

    if (!write_in_progress) {
      this->on_failure();
    }
    return;
  }
  // Calculating actial size of data that has just been read

  for (size_t i = 0, n = data_blocks_read.size() - 1; i < n; ++i) {
    const auto motr_block_size = data_blocks_read[i].second;
    assert(motr_block_size == motr_unit_size);

    // We can use multiplication, but something may change in the future
    bytes_in_chunk_count += motr_block_size;
  }
  if (bytes_in_chunk_count >= bytes_left_to_read) {
    s3_log(S3_LOG_ERROR, request_id,
           "Too many data has been read. Data left - %zu, got - %zu",
           bytes_left_to_read, bytes_in_chunk_count);

    copy_failed = true;
    set_s3_error("InternalError");

    if (!write_in_progress) {
      this->on_failure();
    }
    return;
  }
  bytes_left_to_read -= bytes_in_chunk_count;

  auto& r_last_block_size =
      data_blocks_read[data_blocks_read.size() - 1].second;

  if (r_last_block_size >= bytes_left_to_read) {
    r_last_block_size = bytes_left_to_read;
    bytes_left_to_read = 0;
  } else {
    bytes_left_to_read -= r_last_block_size;
  }
  bytes_in_chunk_count += r_last_block_size;

  s3_log(S3_LOG_DEBUG, request_id, "Got %zu bytes in %zu blocks",
         bytes_in_chunk_count, data_blocks_read.size());

  if (!write_in_progress) {
    // Anyway we have data for writing here and can kick data writing
    write_data_block();

    if (write_in_progress && bytes_left_to_read) {
      // Writing has started successfully.
      // So we have 'space' for reading.
      assert(data_blocks_read.empty());

      read_data_block();
    }
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3ObjectDataCopier::read_data_block_failed() {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);
  s3_log(S3_LOG_ERROR, request_id, "Failed to read object data from motr");

  assert(read_in_progress);
  read_in_progress = false;
  copy_failed = true;

  set_s3_error(motr_reader->get_state() == S3MotrReaderOpState::failed_to_launch
                   ? "ServiceUnavailable"
                   : "InternalError");
  if (!write_in_progress) {
    this->on_failure();
  }

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3ObjectDataCopier::write_data_block() {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);

  assert(!write_in_progress);
  assert(data_blocks_writing.empty());
  assert(!data_blocks_read.empty());

  data_blocks_writing = std::move(data_blocks_read);
  // We have just separated part of data for writing and
  // also freed a 'slot' for futher reading

  motr_writer->write_content(
      std::bind(&S3ObjectDataCopier::write_data_block_success, this),
      std::bind(&S3ObjectDataCopier::write_data_block_failed, this),
      data_blocks_writing,  // NO std::move()!
                            // We must pass a copy of the stucture.
                            // Data buffers will be freed just after writing.
      motr_unit_size);

  if (motr_writer->get_state() == S3MotrWiterOpState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id, "Write of data block failed to start");

    copy_failed = true;
    set_s3_error("ServiceUnavailable");

    if (!read_in_progress) {
      this->on_failure();
    }
  } else {
    write_in_progress = true;
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3ObjectDataCopier::cleanup_blocks_written() {
  auto* p_mem_pool_man = S3MempoolManager::get_instance();
  assert(p_mem_pool_man != nullptr);

  while (!data_blocks_writing.empty()) {
    auto* p_data_block = data_blocks_writing.back().first;

    if (p_data_block) {
      p_mem_pool_man->release_buffer_for_unit_size(p_data_block,
                                                   motr_unit_size);
    }
    data_blocks_writing.pop_back();
  }
}

void S3ObjectDataCopier::write_data_block_success() {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);

  assert(write_in_progress);
  write_in_progress = false;

  cleanup_blocks_written();

  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, nullptr, "Shutdown or rollback");
    return;
  }
  if (copy_failed) {
    assert(!read_in_progress);

    this->on_failure();
    return;
  }
  if (!data_blocks_read.empty()) {
    // We have new chunk of data for writing
    write_data_block();

    if (copy_failed) {
      // New writing failed to start
      return;
    }
  }
  assert(!copy_failed);
  assert(data_blocks_read.empty());

  if (bytes_left_to_read) {

    if (!read_in_progress) {
      read_data_block();
    }
  } else if (!write_in_progress) {
    // We have read all source's data but the call of write_data_block()
    // did not start writing.
    // It means that the function did not find data ready for writing.
    // So all data has been written.
    this->on_success();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3ObjectDataCopier::write_data_block_failed() {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);
  s3_log(S3_LOG_ERROR, request_id, "Failed to write object data to motr");

  assert(write_in_progress);
  write_in_progress = false;

  copy_failed = true;

  cleanup_blocks_written();

  set_s3_error(motr_writer->get_state() == S3MotrWiterOpState::failed_to_launch
                   ? "ServiceUnavailable"
                   : "InternalError");
  if (!read_in_progress) {
    this->on_failure();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3ObjectDataCopier::copy(
    struct m0_uint128 src_obj_id, size_t object_size, int layout_id,
    std::function<bool(void)> check_shutdown_and_rollback,
    std::function<void(void)> on_success,
    std::function<void(void)> on_failure) {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);

  assert(non_zero(src_obj_id));
  assert(object_size > 0);
  assert(layout_id > 0);
  assert(check_shutdown_and_rollback);
  assert(on_success);
  assert(on_failure);

  this->check_shutdown_and_rollback = std::move(check_shutdown_and_rollback);
  this->on_success = std::move(on_success);
  this->on_failure = std::move(on_failure);

  motr_unit_size =
      S3MotrLayoutMap::get_instance()->get_unit_size_for_layout(layout_id);

  motr_reader = motr_reader_factory->create_motr_reader(
      request_object, src_obj_id, layout_id, motr_api);
  motr_reader->set_last_index(0);

  bytes_left_to_read = object_size;

  copy_failed = false;
  read_in_progress = false;
  write_in_progress = false;

  read_data_block();

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3ObjectDataCopier::set_s3_error(std::string s3_error) {
  this->s3_error = std::move(s3_error);
}
