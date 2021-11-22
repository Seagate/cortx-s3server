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
#include <string>
#include <memory>

#include <gtest/gtest.h>

#include "mock_s3_factory.h"
#include "mock_s3_probable_delete_record.h"
#include "s3_object_data_copier.h"
#include "s3_m0_uint128_helper.h"
#include "s3_ut_common.h"
#include "s3_test_utils.h"

using ::testing::AtLeast;
using ::testing::Eq;
using ::testing::Invoke;
using ::testing::ReturnRef;

class S3ObjectDataCopierTest : public testing::Test {
 protected:
  S3ObjectDataCopierTest();
  void SetUp() override;

  std::shared_ptr<MockS3RequestObject> ptr_mock_request;
  std::shared_ptr<MockS3Motr> ptr_mock_s3_motr_api;
  std::shared_ptr<MockS3MotrReaderFactory> ptr_mock_motr_reader_factory;
  std::shared_ptr<MockS3MotrWriterFactory> ptr_mock_motr_writer_factory;

  std::unique_ptr<S3ObjectDataCopier> entity_under_test;

  struct m0_uint128 oid = {0x1ffff, 0x1ffff};
  struct m0_uint128 objects_version_list_idx_oid = {0x1ffff, 0x11fff};
  struct m0_uint128 object_list_indx_oid = {0x11ffff, 0x1ffff};
  struct m0_uint128 zero_oid_idx = {};

  bool f_success;
  bool f_failed;

 public:
  void on_success_cb();
  void on_failed_cb();
};

void S3ObjectDataCopierTest::on_success_cb() { f_success = true; }

void S3ObjectDataCopierTest::on_failed_cb() { f_failed = true; }

bool fn_false_cb() { return false; }

bool fn_true_cb() { return true; }

S3ObjectDataCopierTest::S3ObjectDataCopierTest()
    : ptr_mock_s3_motr_api(std::make_shared<MockS3Motr>()),
      f_success(false),
      f_failed(false) {

  // ptr_mock_s3_motr_api = std::make_shared<MockS3Motr>();

  EXPECT_CALL(*ptr_mock_s3_motr_api, m0_h_ufid_next(_))
      .WillRepeatedly(Invoke(dummy_helpers_ufid_next));

  ptr_mock_request =
      std::make_shared<MockS3RequestObject>(nullptr, new EvhtpWrapper());

  ptr_mock_motr_writer_factory = std::make_shared<MockS3MotrWriterFactory>(
      ptr_mock_request, oid, ptr_mock_s3_motr_api);

  ptr_mock_motr_reader_factory =
      std::make_shared<MockS3MotrReaderFactory>(ptr_mock_request, oid, 1);
}

void S3ObjectDataCopierTest::SetUp() {

  entity_under_test.reset(new S3ObjectDataCopier(
      ptr_mock_request, ptr_mock_motr_writer_factory->mock_motr_writer,
      ptr_mock_motr_reader_factory, ptr_mock_s3_motr_api));

  entity_under_test->motr_reader =
      ptr_mock_motr_reader_factory->mock_motr_reader;
  entity_under_test->on_success =
      std::bind(&S3ObjectDataCopierTest::on_success_cb, this);
  entity_under_test->on_failure =
      std::bind(&S3ObjectDataCopierTest::on_failed_cb, this);
  entity_under_test->motr_unit_size =
      S3MotrLayoutMap::get_instance()->get_unit_size_for_layout(1);
  entity_under_test->check_shutdown_and_rollback = &fn_false_cb;

  entity_under_test->bytes_left_to_read = 1024;
  entity_under_test->copy_failed = false;
  entity_under_test->read_in_progress = false;
  entity_under_test->write_in_progress = false;

  f_success = false;
  f_failed = false;
}

TEST_F(S3ObjectDataCopierTest, ReadDataBlockStarted) {

  EXPECT_CALL(*ptr_mock_motr_reader_factory->mock_motr_reader,
              read_object_data(_, _, _))
      .Times(1)
      .WillOnce(Return(true));

  entity_under_test->read_data_block();

  EXPECT_TRUE(entity_under_test->read_in_progress);
}

TEST_F(S3ObjectDataCopierTest, ReadDataBlockFailedToStart) {

  EXPECT_CALL(*ptr_mock_motr_reader_factory->mock_motr_reader,
              read_object_data(_, _, _))
      .Times(1)
      .WillOnce(Return(false));

  EXPECT_CALL(*ptr_mock_motr_reader_factory->mock_motr_reader, get_state())
      .Times(1)
      .WillOnce(Return(S3MotrReaderOpState::failed_to_launch));

  entity_under_test->read_data_block();

  EXPECT_FALSE(entity_under_test->read_in_progress);
  EXPECT_TRUE(entity_under_test->copy_failed);
  EXPECT_FALSE(entity_under_test->get_s3_error().empty());
}

TEST_F(S3ObjectDataCopierTest, ReadDataBlockSuccessWhileShuttingDown) {

  entity_under_test->read_in_progress = true;
  entity_under_test->check_shutdown_and_rollback = &fn_true_cb;

  entity_under_test->read_data_block_success();

  EXPECT_FALSE(entity_under_test->read_in_progress);
}

TEST_F(S3ObjectDataCopierTest, ReadDataBlockSuccessCopyFailed) {

  entity_under_test->read_in_progress = true;
  entity_under_test->copy_failed = true;
  entity_under_test->set_s3_error("InternalError");

  entity_under_test->read_data_block_success();

  EXPECT_FALSE(entity_under_test->read_in_progress);
  EXPECT_TRUE(f_failed);
}

TEST_F(S3ObjectDataCopierTest, ReadDataBlockSuccessShouldStartWrite) {
  entity_under_test->read_in_progress = true;

  S3BufferSequence data_blocks_read;
  data_blocks_read.emplace_back(nullptr, entity_under_test->size_of_ev_buffer);

  EXPECT_CALL(*ptr_mock_motr_reader_factory->mock_motr_reader,
              extract_blocks_read())
      .Times(1)
      .WillOnce(Return(data_blocks_read));

  EXPECT_CALL(*ptr_mock_motr_writer_factory->mock_motr_writer,
              write_content(_, _, _, _)).Times(1);
  EXPECT_CALL(*ptr_mock_motr_writer_factory->mock_motr_writer, get_state())
      .Times(1);
  auto *p_evbuffer = evbuffer_new();
  EXPECT_CALL(*ptr_mock_motr_reader_factory->mock_motr_reader,
              get_evbuffer_ownership())
      .Times(1)
      .WillOnce(Return(p_evbuffer));

  entity_under_test->read_data_block_success();

  EXPECT_TRUE(entity_under_test->write_in_progress);
}

TEST_F(S3ObjectDataCopierTest, ReadDataBlockFailed) {

  entity_under_test->read_in_progress = true;

  EXPECT_CALL(*ptr_mock_motr_reader_factory->mock_motr_reader, get_state())
      .Times(1);

  entity_under_test->read_data_block_failed();

  EXPECT_FALSE(entity_under_test->read_in_progress);
  EXPECT_TRUE(entity_under_test->copy_failed);
  EXPECT_FALSE(entity_under_test->get_s3_error().empty());
  EXPECT_TRUE(f_failed);
}

TEST_F(S3ObjectDataCopierTest, SetS3CopyFailed) {
  entity_under_test->copy_failed = true;
  entity_under_test->set_s3_copy_failed();

  EXPECT_TRUE(entity_under_test->copy_failed);
}

TEST_F(S3ObjectDataCopierTest, WriteObjectStarted) {

  entity_under_test->data_blocks_read.emplace_back(nullptr, 0);
  entity_under_test->p_evbuffer_read = evbuffer_new();

  EXPECT_CALL(*ptr_mock_motr_writer_factory->mock_motr_writer,
              write_content(_, _, _, _)).Times(1);
  EXPECT_CALL(*ptr_mock_motr_writer_factory->mock_motr_writer, get_state())
      .Times(1)
      .WillOnce(Return(S3MotrWiterOpState::start));

  entity_under_test->write_data_block();

  EXPECT_TRUE(entity_under_test->write_in_progress);
  EXPECT_TRUE(entity_under_test->data_blocks_read.empty());
}

TEST_F(S3ObjectDataCopierTest, WriteObjectFailedShouldUndoMarkProgress) {

  entity_under_test->write_in_progress = true;

  EXPECT_CALL(*ptr_mock_motr_writer_factory->mock_motr_writer, get_state())
      .Times(1)
      .WillOnce(Return(S3MotrWiterOpState::failed));

  entity_under_test->write_data_block_failed();

  EXPECT_STREQ("InternalError", entity_under_test->get_s3_error().c_str());
  EXPECT_FALSE(entity_under_test->write_in_progress);
  EXPECT_TRUE(f_failed);
}

TEST_F(S3ObjectDataCopierTest, WriteObjectFailedDuetoEntityOpenFailure) {

  entity_under_test->write_in_progress = true;

  EXPECT_CALL(*ptr_mock_motr_writer_factory->mock_motr_writer, get_state())
      .Times(1)
      .WillOnce(Return(S3MotrWiterOpState::failed_to_launch));

  entity_under_test->write_data_block_failed();

  EXPECT_FALSE(entity_under_test->write_in_progress);
  EXPECT_STREQ("ServiceUnavailable", entity_under_test->get_s3_error().c_str());
  EXPECT_TRUE(f_failed);
}

TEST_F(S3ObjectDataCopierTest, WriteObjectSuccessfulWhileShuttingDown) {

  entity_under_test->check_shutdown_and_rollback = &fn_true_cb;
  entity_under_test->write_in_progress = true;
  entity_under_test->p_evbuffer_write = evbuffer_new();

  entity_under_test->write_data_block_success();

  EXPECT_FALSE(entity_under_test->write_in_progress);
}

TEST_F(S3ObjectDataCopierTest, WriteObjectSuccessfulShouldRestartWritingData) {

  entity_under_test->write_in_progress = true;
  entity_under_test->data_blocks_read.emplace_back(nullptr, 0);
  entity_under_test->p_evbuffer_read = evbuffer_new();
  entity_under_test->p_evbuffer_write = evbuffer_new();

  EXPECT_CALL(*ptr_mock_motr_writer_factory->mock_motr_writer,
              write_content(_, _, _, _)).Times(1);
  EXPECT_CALL(*ptr_mock_motr_writer_factory->mock_motr_writer, get_state())
      .Times(1)
      .WillOnce(Return(S3MotrWiterOpState::start));

  entity_under_test->write_data_block_success();

  EXPECT_TRUE(entity_under_test->write_in_progress);
}

TEST_F(S3ObjectDataCopierTest,
       WriteObjectSuccessfulDoNextStepWhenAllIsWritten) {

  entity_under_test->write_in_progress = true;
  entity_under_test->bytes_left_to_read = 0;
  entity_under_test->p_evbuffer_write = evbuffer_new();

  entity_under_test->write_data_block_success();

  EXPECT_FALSE(entity_under_test->write_in_progress);
  EXPECT_TRUE(f_success);
}
