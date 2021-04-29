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

class S3ObjectDataCopier {

  std::string request_id;
  std::string s3_error;

  std::function<bool(void)> check_shutdown_and_rollback;
  std::function<void(void)> on_success;
  std::function<void(void)> on_failure;

  std::shared_ptr<RequestObject> request_object;
  std::shared_ptr<S3MotrWiter> motr_writer;
  std::shared_ptr<S3MotrReaderFactory> motr_reader_factory;
  std::shared_ptr<MotrAPI> motr_api;
  std::shared_ptr<S3MotrReader> motr_reader;

  S3BufferSequence data_blocks_read;     // Source's data has been read but not
                                         // taken for writing.
  S3BufferSequence data_blocks_writing;  // Source's data currently beeing
                                         // written

  // All POD variables should be (re)initialized in This::copy()
  size_t bytes_left_to_read;
  size_t motr_unit_size;
  bool copy_failed;
  bool read_in_progress;
  bool write_in_progress;

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
                     std::shared_ptr<S3MotrWiter> motr_writer,
                     std::shared_ptr<S3MotrReaderFactory> motr_reader_factory,
                     std::shared_ptr<MotrAPI> motr_api);

  ~S3ObjectDataCopier();

  void copy(struct m0_uint128 src_obj_id, size_t object_size, int layout_id,
            struct m0_fid pvid,
            std::function<bool(void)> check_shutdown_and_rollback,
            std::function<void(void)> on_success,
            std::function<void(void)> on_failure);

  const std::string& get_s3_error() { return s3_error; }

  friend class S3ObjectDataCopierTest;

  FRIEND_TEST(S3ObjectDataCopierTest, ReadDataBlockStarted);
  FRIEND_TEST(S3ObjectDataCopierTest, ReadDataBlockFailedToStart);
  FRIEND_TEST(S3ObjectDataCopierTest, ReadDataBlockSuccessWhileShuttingDown);
  FRIEND_TEST(S3ObjectDataCopierTest, ReadDataBlockSuccessCopyFailed);
  FRIEND_TEST(S3ObjectDataCopierTest, ReadDataBlockSuccessShouldStartWrite);
  FRIEND_TEST(S3ObjectDataCopierTest, ReadDataBlockFailed);
  FRIEND_TEST(S3ObjectDataCopierTest, WriteObjectStarted);
  FRIEND_TEST(S3ObjectDataCopierTest, WriteObjectFailedShouldUndoMarkProgress);
  FRIEND_TEST(S3ObjectDataCopierTest, WriteObjectFailedDuetoEntityOpenFailure);
  FRIEND_TEST(S3ObjectDataCopierTest, WriteObjectSuccessfulWhileShuttingDown);
  FRIEND_TEST(S3ObjectDataCopierTest,
              WriteObjectSuccessfulShouldRestartWritingData);
  FRIEND_TEST(S3ObjectDataCopierTest,
              WriteObjectSuccessfulDoNextStepWhenAllIsWritten);
};
