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
    std::shared_ptr<S3MotrWiter> s3_motr_writer,
    std::shared_ptr<S3MotrReaderFactory> s3_motr_reader_factory,
    std::shared_ptr<MotrAPI> s3_motr_api)
    : request_object(std::move(request_object)) {
  if (s3_motr_api) {
    motr_api = std::move(s3_motr_api);
  } else {
    motr_api = std::make_shared<ConcreteMotrAPI>();
  }

  if (s3_motr_writer) {
    motr_writer = std::move(s3_motr_writer);
  } else {
    motr_writer = std::make_shared<S3MotrWiter>(request_object, motr_api);
  }

  if (s3_motr_reader_factory) {
    motr_reader_factory = std::move(s3_motr_reader_factory);
  } else {
    motr_reader_factory = std::make_shared<S3MotrReaderFactory>();
  }

  request_id = this->request_object->get_request_id();
  size_of_ev_buffer = g_option_instance->get_libevent_pool_buffer_size();
  s3_log(S3_LOG_DEBUG, request_id, "%s Ctor\n", __func__);
}

S3ObjectDataCopier::~S3ObjectDataCopier() {
  if (p_evbuffer_write) {
    ::evbuffer_free(p_evbuffer_write);
  }
  if (p_evbuffer_read) {
    ::evbuffer_free(p_evbuffer_read);
  }
}

void S3ObjectDataCopier::read_data_block() {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);

  assert(!read_in_progress);
  assert(data_blocks_read.empty());
  assert(!p_evbuffer_read);
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
      if (copy_parts_fragment) {
        this->on_failure_for_fragments(vector_index);
      } else {
        this->on_failure();
      }
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

    if (copy_parts_fragment) {
      this->on_failure_for_fragments(vector_index);
    } else {
      this->on_failure();
    }
    return;
  }
  assert(data_blocks_read.empty());
  data_blocks_read = motr_reader->extract_blocks_read();

  // Save ev buffer containing the read data, which will be freed after
  // data write completion.
  assert(!p_evbuffer_read);
  p_evbuffer_read = motr_reader->get_evbuffer_ownership();
  assert(p_evbuffer_read != nullptr);

  size_t bytes_in_chunk_count = 0;

  if (data_blocks_read.empty()) {
    s3_log(S3_LOG_ERROR, request_id, "Motr reader returned no data");

    copy_failed = true;
    set_s3_error("InternalError");

    if (!write_in_progress) {
      if (copy_parts_fragment) {
        this->on_failure_for_fragments(vector_index);
      } else {
        this->on_failure();
      }
    }
    return;
  }
  // Calculating actual size of data that has just been read

  for (size_t i = 0, n = data_blocks_read.size(); i < n; ++i) {
    const auto motr_block_size = data_blocks_read[i].second;
    assert(motr_block_size == size_of_ev_buffer);

    // We can use multiplication, but something may change in the future
    bytes_in_chunk_count += motr_block_size;
  }
  if (bytes_in_chunk_count >= bytes_left_to_read) {
    s3_log(S3_LOG_DEBUG, request_id,
           "Too many data has been read. Data left - %zu, got - %zu",
           bytes_left_to_read, bytes_in_chunk_count);
    // get_buffers(data_blocks_read, bytes_in_chunk_count, bytes_left_to_read);
    bytes_in_chunk_count = bytes_left_to_read;
    bytes_left_to_read = 0;
  } else {
    bytes_left_to_read -= bytes_in_chunk_count;
  }

  s3_log(S3_LOG_DEBUG, request_id, "Got %zu bytes in %zu blocks",
         bytes_in_chunk_count, data_blocks_read.size());

  if (!write_in_progress) {
    // Anyway we have data for writing here and can kick data writing
    write_data_block();

    if (write_in_progress && bytes_left_to_read) {
      // Writing has started successfully.
      // So we have 'space' for reading.
      assert(data_blocks_read.empty());
      assert(!p_evbuffer_read);

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
    if (copy_parts_fragment) {
      this->on_failure_for_fragments(vector_index);
    } else {
      this->on_failure();
    }
  }

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3ObjectDataCopier::write_data_block() {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);

  assert(!write_in_progress);

  assert(!data_blocks_read.empty());

  assert(p_evbuffer_read != nullptr);
  assert(!p_evbuffer_write);
  p_evbuffer_write = p_evbuffer_read;
  p_evbuffer_read = nullptr;

  // We have just separated part of data for writing and
  // also freed a 'slot' for futher reading
  motr_writer->write_content(
      std::bind(&S3ObjectDataCopier::write_data_block_success, this),
      std::bind(&S3ObjectDataCopier::write_data_block_failed, this),
      std::move(data_blocks_read), size_of_ev_buffer);

  if (motr_writer->get_state() == S3MotrWiterOpState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id, "Write of data block failed to start");

    copy_failed = true;
    set_s3_error("ServiceUnavailable");

    if (!read_in_progress) {
      if (copy_parts_fragment) {
        this->on_failure_for_fragments(vector_index);
      } else {
        this->on_failure();
      }
    }
  } else {
    write_in_progress = true;
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3ObjectDataCopier::write_data_block_success() {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);

  assert(p_evbuffer_write != nullptr);
  ::evbuffer_free(p_evbuffer_write);
  p_evbuffer_write = nullptr;

  assert(write_in_progress);
  write_in_progress = false;

  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, nullptr, "Shutdown or rollback");
    return;
  }
  if (copy_failed) {
    assert(!read_in_progress);
    if (copy_parts_fragment) {
      this->on_failure_for_fragments(vector_index);
    } else {
      this->on_failure();
    }
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

    // read data from next parts
    if (copy_parts_fragment_in_single_source &&
        ((part_vector_index + 1) < part_fragment_context_list.size())) {
      part_vector_index++;
      set_next_part_context();
      read_data_block();
    } else {
      if (copy_parts_fragment) {
        this->on_success_for_fragments(vector_index);
      } else {
        this->on_success();
      }
    }
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3ObjectDataCopier::write_data_block_failed() {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);
  s3_log(S3_LOG_ERROR, request_id, "Failed to write object data to motr");

  assert(write_in_progress);
  write_in_progress = false;

  copy_failed = true;

  set_s3_error(motr_writer->get_state() == S3MotrWiterOpState::failed_to_launch
                   ? "ServiceUnavailable"
                   : "InternalError");
  if (!read_in_progress) {
    if (copy_parts_fragment) {
      this->on_failure_for_fragments(vector_index);
    } else {
      this->on_failure();
    }
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3ObjectDataCopier::copy(
    struct m0_uint128 src_obj_id, size_t object_size, int layout_id,
    struct m0_fid pvid, std::function<bool(void)> check_shutdown_and_rollback,
    std::function<void(void)> on_success, std::function<void(void)> on_failure,
    size_t first_byte_offset) {
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
      request_object, src_obj_id, layout_id, pvid, motr_api);
  // motr_reader->set_last_index(0);

  size_t block_start_offset =
      first_byte_offset - (first_byte_offset % motr_unit_size);
  motr_reader->set_last_index(block_start_offset);

  bytes_left_to_read = object_size;

  copy_failed = false;
  read_in_progress = false;
  write_in_progress = false;

  read_data_block();

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3ObjectDataCopier::copy_part_fragment_in_single_source(
    std::vector<struct s3_part_frag_context> fragment_context_list,
    std::vector<struct S3ExtendedObjectInfo> extended_objects,
    size_t first_byte_offset_to_copy,
    std::function<bool(void)> check_shutdown_and_rollback,
    std::function<void(void)> on_success, std::function<void(void)> on_failure,
    bool is_range_copy) {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);
  assert(check_shutdown_and_rollback);
  assert(on_success);
  assert(on_failure);
  this->check_shutdown_and_rollback = std::move(check_shutdown_and_rollback);
  this->on_success = std::move(on_success);
  this->on_failure = std::move(on_failure);
  extended_objects_list = extended_objects;
  part_fragment_context_list = fragment_context_list;
  copy_parts_fragment_in_single_source = true;
  first_byte_offset = first_byte_offset_to_copy;
  set_next_part_context();
  read_data_block();
  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
}

void S3ObjectDataCopier::set_next_part_context(bool is_range_copy) {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);
  assert(non_zero(part_fragment_context_list[part_vector_index].motr_OID));
  assert(part_fragment_context_list[part_vector_index].item_size > 0);
  assert(part_fragment_context_list[part_vector_index].layout_id > 0);
  motr_unit_size = S3MotrLayoutMap::get_instance()->get_unit_size_for_layout(
      part_fragment_context_list[part_vector_index].layout_id);

  motr_reader = motr_reader_factory->create_motr_reader(
      request_object, part_fragment_context_list[part_vector_index].motr_OID,
      part_fragment_context_list[part_vector_index].layout_id,
      part_fragment_context_list[part_vector_index].PVID, motr_api);
  size_t block_start_offset = 0;
  if (is_range_copy) {
    if (part_vector_index == 0) {
      size_t unit_size_of_object_with_first_byte =
          S3MotrLayoutMap::get_instance()->get_unit_size_for_layout(
              extended_objects_list[part_vector_index].object_layout);

      size_t first_byte_offset_block =
          (first_byte_offset -
           extended_objects_list[part_vector_index].start_offset_in_object +
           unit_size_of_object_with_first_byte) /
          unit_size_of_object_with_first_byte;

      block_start_offset =
          (first_byte_offset_block - 1) * unit_size_of_object_with_first_byte;
    }
    bytes_left_to_read =
        extended_objects_list[part_vector_index].total_blocks_in_object;
  } else {
    bytes_left_to_read =
        part_fragment_context_list[part_vector_index].item_size;
  }
  motr_reader->set_last_index(block_start_offset);

  copy_failed = false;
  read_in_progress = false;
  write_in_progress = false;
  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
}

void S3ObjectDataCopier::copy_part_fragment(
    std::vector<struct s3_part_frag_context> part_fragment_context_list,
    struct m0_uint128 target_obj_id, int index,
    std::function<bool(void)> check_shutdown_and_rollback,
    std::function<void(int)> on_success_for_fragments,
    std::function<void(int)> on_failure_for_fragments) {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);

  assert(non_zero(part_fragment_context_list[index].motr_OID));
  assert(non_zero(target_obj_id));
  assert(part_fragment_context_list[index].item_size > 0);
  assert(part_fragment_context_list[index].layout_id > 0);
  assert(check_shutdown_and_rollback);
  assert(on_success_for_fragments);
  assert(on_failure_for_fragments);
  copy_parts_fragment = true;
  this->check_shutdown_and_rollback = std::move(check_shutdown_and_rollback);
  this->on_success_for_fragments = std::move(on_success_for_fragments);
  this->on_failure_for_fragments = std::move(on_failure_for_fragments);
  // Index of the part/fragment info vector
  this->vector_index = index;
  copy_parts_fragment = true;

  motr_unit_size = S3MotrLayoutMap::get_instance()->get_unit_size_for_layout(
      part_fragment_context_list[index].layout_id);

  motr_reader = motr_reader_factory->create_motr_reader(
      request_object, part_fragment_context_list[index].motr_OID,
      part_fragment_context_list[index].layout_id,
      part_fragment_context_list[index].PVID, motr_api);
  motr_reader->set_last_index(0);

  bytes_left_to_read = part_fragment_context_list[index].item_size;

  copy_failed = false;
  read_in_progress = false;
  write_in_progress = false;

  read_data_block();

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3ObjectDataCopier::set_s3_error(std::string s3_error) {
  this->s3_error = std::move(s3_error);
}

void S3ObjectDataCopier::set_s3_copy_failed() { copy_failed = true; }

// Trims the input buffer (in-place) to the specified expected size
void S3ObjectDataCopier::get_buffers(S3BufferSequence& buffer,
                                     size_t total_size, size_t expected_size) {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);
  assert(buffer.size() > 0);
  assert(expected_size < total_size);
  if (buffer.size() > 0 && (expected_size > 0 && expected_size < total_size)) {
    // Process to shrink buffer
    // Identify block in 'buffer' after which we need to discard all blocks
    // For this, identify the last block (0 index based) in 'buffer' sequence
    // that occupies the 'expected_size' of data
    size_t last_block_occupied_by_expected_size =
        ((expected_size + size_of_ev_buffer - 1) / size_of_ev_buffer) - 1;
    assert(last_block_occupied_by_expected_size < buffer.size());
    // Re-set the size of last block with data
    // Also, discard memory of all subsequent blocks
    size_t size_of_last_block = expected_size % size_of_ev_buffer;
    if (size_of_last_block > 0) {
      // The last block is not filled with all data bytes. Re-set its length
      buffer[last_block_occupied_by_expected_size].second = size_of_last_block;
    }
    // Discard subsequent blocks from 'buffer'
    size_t next_block = last_block_occupied_by_expected_size + 1;
    if (next_block < buffer.size()) {
      s3_log(S3_LOG_DEBUG, request_id,
             "Discarding [%zu] blocks, starting from block index [%zu]",
             (buffer.size() - last_block_occupied_by_expected_size - 1),
             next_block);
      auto* p_mem_pool_man = S3MempoolManager::get_instance();
      assert(p_mem_pool_man != nullptr);
      for (size_t i = buffer.size(); i > next_block; --i) {
        auto* p_data_block = buffer.back().first;

        if (p_data_block) {
          p_mem_pool_man->release_buffer_for_unit_size(p_data_block,
                                                       size_of_ev_buffer);
        }
        buffer.pop_back();
      }
    }
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}
