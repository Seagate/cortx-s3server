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

#include <cstddef>
#include <functional>
#include <memory>
#include <string>

#include <gtest/gtest_prod.h>

#include "lib/types.h"  // struct m0_uint128
#include "fid/fid.h"    // struct m0_fid
#include "s3_buffer_sequence.h"

class RequestObject;
class S3MotrReader;
class S3MotrReaderFactory;
class S3MotrWiter;
struct evbuffer;

class S3ObjectDataCopier {

  std::string request_id;
  std::string s3_error;

  std::function<bool(void)> check_shutdown_and_rollback;
  std::function<void(void)> on_success;
  std::function<void(void)> on_failure;
  std::function<void(int)> on_success_for_fragments;
  std::function<void(int)> on_failure_for_fragments;

  std::shared_ptr<RequestObject> request_object;
  std::shared_ptr<S3MotrWiter> motr_writer;
  std::shared_ptr<S3MotrReaderFactory> motr_reader_factory;
  std::shared_ptr<MotrAPI> motr_api;
  std::shared_ptr<S3MotrReader> motr_reader;
  std::vector<struct S3ExtendedObjectInfo> extended_objects_list;
  bool copy_parts_fragment = 0;
  int vector_index;
  unsigned int part_vector_index = 0;
  bool copy_parts_fragment_in_single_source = false;
  std::vector<struct s3_part_frag_context> part_fragment_context_list;
  S3BufferSequence data_blocks_read;  // Source's data has been read but not
                                      // taken for writing.

  // All POD variables should be (re)initialized in This::copy()
  size_t bytes_left_to_read;
  size_t motr_unit_size;
  size_t first_byte_offset;
  bool copy_failed;
  bool read_in_progress;
  bool write_in_progress;
  // Size of each ev buffer (e.g, 16384)
  size_t size_of_ev_buffer;

  struct evbuffer* p_evbuffer_read = nullptr;
  struct evbuffer* p_evbuffer_write = nullptr;

  void cleanup_blocks_written();
  void read_data_block();
  void read_data_block_success();
  void read_data_block_failed();
  void write_data_block();
  void write_data_block_success();
  void write_data_block_failed();
  void set_s3_error(std::string);

 public:
  S3ObjectDataCopier(std::shared_ptr<RequestObject> request_object,
                     std::shared_ptr<S3MotrWiter> motr_writer = nullptr,
                     std::shared_ptr<S3MotrReaderFactory> motr_reader_factory =
                         nullptr,
                     std::shared_ptr<MotrAPI> motr_api = nullptr);

  ~S3ObjectDataCopier();

  void copy(struct m0_uint128 src_obj_id, size_t object_size, int layout_id,
            struct m0_fid pvid,
            std::function<bool(void)> check_shutdown_and_rollback,
            std::function<void(void)> on_success,
            std::function<void(void)> on_failure, size_t first_byte_offset = 0);

  void copy_part_fragment(
      std::vector<struct s3_part_frag_context> fragment_context_list,
      struct m0_uint128 target_obj_id, int index_to_fragment_context,
      std::function<bool(void)> check_shutdown_and_rollback,
      std::function<void(int)> on_success_for_fragment,
      std::function<void(int)> on_failure_for_fragment);

  void copy_part_fragment_in_single_source(
      std::vector<struct s3_part_frag_context> fragment_context_list,
      std::vector<struct S3ExtendedObjectInfo> extended_objects,
      size_t first_byte_offset_to_copy,
      std::function<bool(void)> check_shutdown_and_rollback,
      std::function<void(void)> on_success,
      std::function<void(void)> on_failure, bool is_range_copy = false);

  void set_next_part_context(bool is_range_copy = false);

  const std::string& get_s3_error() { return s3_error; }
  // Trims input 'buffer' to the expected size
  // Used when we need specific portion of 'buffer', discarding the remianing
  // data.
  void get_buffers(S3BufferSequence& buffer, size_t total_size,
                   size_t expected_size);
  void set_s3_copy_failed();

  friend class S3ObjectDataCopierTest;

  FRIEND_TEST(S3ObjectDataCopierTest, ReadDataBlockStarted);
  FRIEND_TEST(S3ObjectDataCopierTest, ReadDataBlockFailedToStart);
  FRIEND_TEST(S3ObjectDataCopierTest, ReadDataBlockSuccessWhileShuttingDown);
  FRIEND_TEST(S3ObjectDataCopierTest, ReadDataBlockSuccessCopyFailed);
  FRIEND_TEST(S3ObjectDataCopierTest, ReadDataBlockSuccessShouldStartWrite);
  FRIEND_TEST(S3ObjectDataCopierTest, ReadDataBlockFailed);
  FRIEND_TEST(S3ObjectDataCopierTest, WriteObjectStarted);
  FRIEND_TEST(S3ObjectDataCopierTest, SetS3CopyFailed);
  FRIEND_TEST(S3ObjectDataCopierTest, WriteObjectFailedShouldUndoMarkProgress);
  FRIEND_TEST(S3ObjectDataCopierTest, WriteObjectFailedDuetoEntityOpenFailure);
  FRIEND_TEST(S3ObjectDataCopierTest, WriteObjectSuccessfulWhileShuttingDown);
  FRIEND_TEST(S3ObjectDataCopierTest,
              WriteObjectSuccessfulShouldRestartWritingData);
  FRIEND_TEST(S3ObjectDataCopierTest,
              WriteObjectSuccessfulDoNextStepWhenAllIsWritten);
};
