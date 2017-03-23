/*
 * COPYRIGHT 2017 SEAGATE LLC
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
 * Original creation date: 23-Mar-2017
 */

#include <memory>

#include "mock_s3_factory.h"
#include "s3_put_object_action.h"
#include "s3_test_utils.h"

using ::testing::Eq;
using ::testing::Return;
using ::testing::Invoke;
using ::testing::_;
using ::testing::ReturnRef;
using ::testing::AtLeast;
using ::testing::DefaultValue;

#define CREATE_BUCKET_METADATA                                            \
  do {                                                                    \
    EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), load(_, _)) \
        .Times(AtLeast(1));                                               \
    action_under_test->fetch_bucket_info();                               \
  } while (0)

#define CREATE_OBJECT_METADATA                                             \
  do {                                                                     \
    CREATE_BUCKET_METADATA;                                                \
    EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state()) \
        .WillOnce(Return(S3BucketMetadataState::present));                 \
    bucket_meta_factory->mock_bucket_metadata->set_object_list_index_oid(  \
        object_list_indx_oid);                                             \
    EXPECT_CALL(*(object_meta_factory->mock_object_metadata), load(_, _))  \
        .Times(AtLeast(1));                                                \
    action_under_test->fetch_object_info();                                \
  } while (0)

class S3PutObjectActionTest : public testing::Test {
 protected:
  S3PutObjectActionTest() {
    S3Option::get_instance()->disable_auth();

    evhtp_request_t *req = NULL;
    EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();

    oid = {0x1ffff, 0x1ffff};
    object_list_indx_oid = {0x11ffff, 0x1ffff};
    zero_oid_idx = {0ULL, 0ULL};

    call_count_one = 0;

    async_buffer_factory = new MockS3AsyncBufferOptContainerFactory(
        S3Option::get_instance()->get_libevent_pool_buffer_size());

    ptr_mock_request = std::make_shared<MockS3RequestObject>(
        req, evhtp_obj_ptr, async_buffer_factory);

    // Owned and deleted by shared_ptr in S3PutObjectAction
    bucket_meta_factory = new MockS3BucketMetadataFactory(ptr_mock_request);

    object_meta_factory =
        new MockS3ObjectMetadataFactory(ptr_mock_request, object_list_indx_oid);

    clovis_writer_factory =
        new MockS3ClovisWriterFactory(ptr_mock_request, oid);

    action_under_test.reset(
        new S3PutObjectAction(ptr_mock_request, bucket_meta_factory,
                              object_meta_factory, clovis_writer_factory));
  }

  std::shared_ptr<MockS3RequestObject> ptr_mock_request;
  MockS3BucketMetadataFactory *bucket_meta_factory;
  MockS3ObjectMetadataFactory *object_meta_factory;
  MockS3ClovisWriterFactory *clovis_writer_factory;
  MockS3AsyncBufferOptContainerFactory *async_buffer_factory;

  std::shared_ptr<S3PutObjectAction> action_under_test;

  struct m0_uint128 object_list_indx_oid;
  struct m0_uint128 oid;
  struct m0_uint128 zero_oid_idx;

  int call_count_one;

 public:
  void func_callback_one() { call_count_one += 1; }
};

TEST_F(S3PutObjectActionTest, ConstructorTest) {
  EXPECT_EQ(0, action_under_test->tried_count);
  EXPECT_EQ("uri_salt_", action_under_test->salt);
  EXPECT_NE(0, action_under_test->number_of_tasks());
}

TEST_F(S3PutObjectActionTest, FetchBucketInfo) {
  CREATE_BUCKET_METADATA;
  EXPECT_TRUE(action_under_test->bucket_metadata != NULL);
}

TEST_F(S3PutObjectActionTest, FetchObjectInfoWhenBucketNotPresent) {
  CREATE_BUCKET_METADATA;

  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillRepeatedly(Return(S3BucketMetadataState::missing));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(_, _)).Times(1);
  EXPECT_CALL(*ptr_mock_request, resume()).Times(1);

  action_under_test->fetch_object_info();

  EXPECT_STREQ("NoSuchBucket", action_under_test->get_s3_error_code().c_str());
  EXPECT_TRUE(action_under_test->bucket_metadata != NULL);
  EXPECT_TRUE(action_under_test->object_metadata == NULL);
}

TEST_F(S3PutObjectActionTest, FetchObjectInfoWhenBucketAndObjIndexPresent) {
  CREATE_BUCKET_METADATA;

  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillRepeatedly(Return(S3BucketMetadataState::present));
  bucket_meta_factory->mock_bucket_metadata->set_object_list_index_oid(
      object_list_indx_oid);

  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), load(_, _))
      .Times(AtLeast(1));

  action_under_test->fetch_object_info();

  EXPECT_TRUE(action_under_test->bucket_metadata != NULL);
  EXPECT_TRUE(action_under_test->object_metadata != NULL);
}

TEST_F(S3PutObjectActionTest,
       FetchObjectInfoWhenBucketPresentAndObjIndexAbsent) {
  CREATE_BUCKET_METADATA;

  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .Times(AtLeast(1))
      .WillOnce(Return(S3BucketMetadataState::present));
  bucket_meta_factory->mock_bucket_metadata->set_object_list_index_oid(
      zero_oid_idx);

  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), load(_, _))
      .Times(0);

  // Mock out the next calls on action.
  action_under_test->clear_tasks();
  action_under_test->add_task(
      std::bind(&S3PutObjectActionTest::func_callback_one, this));

  action_under_test->fetch_object_info();

  EXPECT_EQ(1, call_count_one);
  EXPECT_TRUE(action_under_test->bucket_metadata != nullptr);
  EXPECT_TRUE(action_under_test->object_metadata == nullptr);
}

TEST_F(S3PutObjectActionTest, FetchObjectInfoReturnedFoundShouldHaveNewOID) {
  CREATE_OBJECT_METADATA;

  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_state())
      .WillOnce(Return(S3ObjectMetadataState::present));

  struct m0_uint128 old_oid = {0x1ffff, 0x1ffff};
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_oid())
      .Times(1)
      .WillOnce(Return(old_oid));

  // Mock out the next calls on action.
  action_under_test->clear_tasks();
  action_under_test->add_task(
      std::bind(&S3PutObjectActionTest::func_callback_one, this));

  // Remember default generated OID
  struct m0_uint128 oid_before_regen = action_under_test->new_object_oid;
  action_under_test->fetch_object_info_status();

  EXPECT_EQ(1, call_count_one);
  EXPECT_OID_NE(zero_oid_idx, action_under_test->old_object_oid);
  EXPECT_OID_NE(oid_before_regen, action_under_test->new_object_oid);
}

TEST_F(S3PutObjectActionTest, FetchObjectInfoReturnedNotFoundShouldUseURL2OID) {
  CREATE_OBJECT_METADATA;

  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_state())
      .WillRepeatedly(Return(S3ObjectMetadataState::missing));

  // Mock out the next calls on action.
  action_under_test->clear_tasks();
  action_under_test->add_task(
      std::bind(&S3PutObjectActionTest::func_callback_one, this));

  // Remember default generated OID
  struct m0_uint128 oid_before_regen = action_under_test->new_object_oid;
  action_under_test->fetch_object_info_status();

  EXPECT_EQ(1, call_count_one);
  EXPECT_OID_EQ(zero_oid_idx, action_under_test->old_object_oid);
  EXPECT_OID_EQ(oid_before_regen, action_under_test->new_object_oid);
}

TEST_F(S3PutObjectActionTest, FetchObjectInfoReturnedInvalidStateReportsError) {
  CREATE_OBJECT_METADATA;

  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_state())
      .WillRepeatedly(Return(S3ObjectMetadataState::failed));

  // Mock out the next calls on action.
  action_under_test->clear_tasks();
  action_under_test->add_task(
      std::bind(&S3PutObjectActionTest::func_callback_one, this));

  // Remember default generated OID
  struct m0_uint128 oid_before_regen = action_under_test->new_object_oid;

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(_, _)).Times(1);
  EXPECT_CALL(*ptr_mock_request, resume()).Times(1);

  action_under_test->fetch_object_info_status();

  EXPECT_STREQ("InternalError", action_under_test->get_s3_error_code().c_str());
  EXPECT_EQ(0, call_count_one);
  EXPECT_OID_EQ(zero_oid_idx, action_under_test->old_object_oid);
  EXPECT_OID_EQ(oid_before_regen, action_under_test->new_object_oid);
}

TEST_F(S3PutObjectActionTest, CreateObjectFirstAttempt) {
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer), create_object(_, _))
      .Times(1);
  action_under_test->create_object();
  EXPECT_TRUE(action_under_test->clovis_writer != nullptr);
}

TEST_F(S3PutObjectActionTest, CreateObjectSecondAttempt) {
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer), create_object(_, _))
      .Times(2);
  action_under_test->create_object();
  action_under_test->tried_count = 1;
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer), set_oid(_))
      .Times(1);
  action_under_test->create_object();
  EXPECT_TRUE(action_under_test->clovis_writer != nullptr);
}

TEST_F(S3PutObjectActionTest, CreateObjectFailedTestWhileShutdown) {
  S3Option::get_instance()->set_is_s3_shutting_down(true);
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(503, _)).Times(1);
  EXPECT_CALL(*ptr_mock_request, resume()).Times(1);

  action_under_test->create_object_failed();
  EXPECT_TRUE(action_under_test->clovis_writer == NULL);
  S3Option::get_instance()->set_is_s3_shutting_down(false);
}

TEST_F(S3PutObjectActionTest, CreateObjectFailedWithCollisionExceededRetry) {
  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer), get_state())
      .Times(1)
      .WillOnce(Return(S3ClovisWriterOpState::exists));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(500, _)).Times(1);
  EXPECT_CALL(*ptr_mock_request, resume()).Times(1);

  action_under_test->tried_count = MAX_COLLISION_RETRY_COUNT + 1;
  action_under_test->create_object_failed();

  EXPECT_STREQ("InternalError", action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3PutObjectActionTest, CreateObjectFailedWithCollisionRetry) {
  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer), get_state())
      .Times(1)
      .WillOnce(Return(S3ClovisWriterOpState::exists));

  action_under_test->tried_count = MAX_COLLISION_RETRY_COUNT - 1;
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer), set_oid(_))
      .Times(1);
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer), create_object(_, _))
      .Times(1);

  action_under_test->create_object_failed();
}

TEST_F(S3PutObjectActionTest, CreateObjectFailedTest) {
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer), create_object(_, _))
      .Times(1);
  action_under_test->create_object();

  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer), get_state())
      .Times(1)
      .WillOnce(Return(S3ClovisWriterOpState::failed));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(_, _)).Times(1);
  EXPECT_CALL(*ptr_mock_request, resume()).Times(1);

  action_under_test->create_object_failed();
  EXPECT_STREQ("InternalError", action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3PutObjectActionTest, CreateNewOidTest) {
  struct m0_uint128 old_oid = action_under_test->new_object_oid;

  action_under_test->create_new_oid(old_oid);

  EXPECT_OID_NE(old_oid, action_under_test->new_object_oid);
}

TEST_F(S3PutObjectActionTest, RollbackTest) {
  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer), delete_object(_, _))
      .Times(1);
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer), set_oid(_))
      .Times(1);
  action_under_test->rollback_create();
}

TEST_F(S3PutObjectActionTest, RollbackFailedTest1) {
  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer), get_state())
      .Times(1)
      .WillOnce(Return(S3ClovisWriterOpState::missing));
  action_under_test->add_task_rollback(
      std::bind(&S3PutObjectActionTest::func_callback_one, this));
  action_under_test->rollback_create_failed();
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3PutObjectActionTest, RollbackFailedTest2) {
  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer), get_state())
      .WillOnce(Return(S3ClovisWriterOpState::failed));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(500, _)).Times(1);
  EXPECT_CALL(*ptr_mock_request, resume()).Times(1);

  action_under_test->rollback_create_failed();
  EXPECT_EQ(0, call_count_one);
}

TEST_F(S3PutObjectActionTest, InitiateDataStreamingForZeroSizeObject) {
  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;
  EXPECT_CALL(*ptr_mock_request, get_content_length())
      .Times(1)
      .WillOnce(Return(0));

  // Mock out the next calls on action.
  action_under_test->clear_tasks();
  action_under_test->add_task(
      std::bind(&S3PutObjectActionTest::func_callback_one, this));

  action_under_test->initiate_data_streaming();
  EXPECT_EQ(false, action_under_test->write_in_progress);
  EXPECT_EQ(1, call_count_one);
  EXPECT_EQ(1, action_under_test->number_of_rollback_tasks());
}

TEST_F(S3PutObjectActionTest, InitiateDataStreamingExpectingMoreData) {
  EXPECT_CALL(*ptr_mock_request, get_content_length())
      .Times(1)
      .WillOnce(Return(1024));
  EXPECT_CALL(*ptr_mock_request, has_all_body_content())
      .Times(1)
      .WillOnce(Return(false));
  EXPECT_CALL(*ptr_mock_request, listen_for_incoming_data(_, _)).Times(1);

  action_under_test->initiate_data_streaming();

  EXPECT_EQ(false, action_under_test->write_in_progress);
}

TEST_F(S3PutObjectActionTest, InitiateDataStreamingWeHaveAllData) {
  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;
  EXPECT_CALL(*ptr_mock_request, get_content_length())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(1024));
  EXPECT_CALL(*ptr_mock_request, has_all_body_content())
      .Times(1)
      .WillOnce(Return(true));

  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer),
              write_content(_, _, _))
      .Times(1);

  action_under_test->initiate_data_streaming();

  EXPECT_EQ(true, action_under_test->write_in_progress);
}

// Write not in progress and we have all the data
TEST_F(S3PutObjectActionTest, ConsumeIncomingShouldWriteIfWeAllData) {
  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;
  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), is_freezed())
      .WillRepeatedly(Return(true));
  // S3Option::get_instance()->get_clovis_write_payload_size() = 1048576 * 1
  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), get_content_length())
      .WillRepeatedly(Return(1024));

  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer),
              write_content(_, _, _))
      .Times(1);

  action_under_test->consume_incoming_content();

  EXPECT_EQ(true, action_under_test->write_in_progress);
}

// Write not in progress, expecting more, we have exact what we can write
TEST_F(S3PutObjectActionTest, ConsumeIncomingShouldWriteIfWeExactData) {
  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;
  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), is_freezed())
      .WillRepeatedly(Return(false));
  // S3Option::get_instance()->get_clovis_write_payload_size() = 1048576 * 1
  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), get_content_length())
      .Times(AtLeast(1))
      .WillRepeatedly(
          Return(S3Option::get_instance()->get_clovis_write_payload_size()));

  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer),
              write_content(_, _, _))
      .Times(1);

  EXPECT_CALL(*ptr_mock_request, pause()).Times(1);

  action_under_test->consume_incoming_content();

  EXPECT_EQ(true, action_under_test->write_in_progress);
}

// Write not in progress, expecting more, we have more than we can write
TEST_F(S3PutObjectActionTest, ConsumeIncomingShouldWriteIfWeHaveMoreData) {
  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;
  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), is_freezed())
      .WillRepeatedly(Return(false));
  // S3Option::get_instance()->get_clovis_write_payload_size() = 1048576 * 1
  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), get_content_length())
      .WillRepeatedly(Return(
          S3Option::get_instance()->get_clovis_write_payload_size() + 1024));

  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer),
              write_content(_, _, _))
      .Times(1);

  EXPECT_CALL(*ptr_mock_request, pause()).Times(1);

  action_under_test->consume_incoming_content();

  EXPECT_EQ(true, action_under_test->write_in_progress);
}

// we are expecting more data
TEST_F(S3PutObjectActionTest, ConsumeIncomingShouldPauseWhenWeHaveTooMuch) {
  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;
  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), is_freezed())
      .WillRepeatedly(Return(false));
  // S3Option::get_instance()->get_clovis_write_payload_size() = 1048576 * 1
  // S3_READ_AHEAD_MULTIPLE: 1
  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), get_content_length())
      .WillRepeatedly(
          Return(S3Option::get_instance()->get_clovis_write_payload_size() *
                 S3Option::get_instance()->get_read_ahead_multiple() * 2));

  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer),
              write_content(_, _, _))
      .Times(1);

  EXPECT_CALL(*ptr_mock_request, pause()).Times(1);
  action_under_test->consume_incoming_content();

  EXPECT_EQ(true, action_under_test->write_in_progress);
}

TEST_F(S3PutObjectActionTest,
       ConsumeIncomingShouldNotWriteWhenWriteInprogress) {
  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;
  action_under_test->write_in_progress = true;
  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), is_freezed())
      .WillRepeatedly(Return(true));

  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer),
              write_content(_, _, _))
      .Times(0);

  action_under_test->consume_incoming_content();
}

TEST_F(S3PutObjectActionTest, WriteObjectShouldWriteContentAndMarkProgress) {
  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;

  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer),
              write_content(_, _, _))
      .Times(1);
  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), get_content_length())
      .WillRepeatedly(Return(1024));

  action_under_test->write_object(async_buffer_factory->get_mock_buffer());

  EXPECT_EQ(action_under_test->write_in_progress, true);
}

TEST_F(S3PutObjectActionTest, WriteObjectFailedShouldUndoMarkProgress) {
  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;

  // mock mark progress
  action_under_test->write_in_progress = true;
  // Mock out the rollback calls on action.
  action_under_test->clear_tasks_rollback();
  action_under_test->add_task_rollback(
      std::bind(&S3PutObjectActionTest::func_callback_one, this));

  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), is_freezed())
      .WillRepeatedly(Return(true));

  action_under_test->write_object_failed();

  EXPECT_EQ(action_under_test->write_in_progress, false);
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3PutObjectActionTest, WriteObjectSuccessfulWhileShuttingDown) {
  S3Option::get_instance()->set_is_s3_shutting_down(true);
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(503, _)).Times(1);
  EXPECT_CALL(*ptr_mock_request, resume()).Times(1);

  // mock mark progress
  action_under_test->write_in_progress = true;

  action_under_test->write_object_successful();

  S3Option::get_instance()->set_is_s3_shutting_down(false);

  EXPECT_EQ(action_under_test->write_in_progress, false);
}

TEST_F(S3PutObjectActionTest,
       WriteObjectSuccessfulWhileShuttingDownAndRollback) {
  S3Option::get_instance()->set_is_s3_shutting_down(true);

  // mock mark progress
  action_under_test->write_in_progress = true;
  // Mock out the rollback calls on action.
  action_under_test->clear_tasks_rollback();
  action_under_test->add_task_rollback(
      std::bind(&S3PutObjectActionTest::func_callback_one, this));

  action_under_test->write_object_successful();

  S3Option::get_instance()->set_is_s3_shutting_down(false);

  EXPECT_EQ(action_under_test->write_in_progress, false);
  EXPECT_EQ(1, call_count_one);
}

// We have all the data: Freezed
TEST_F(S3PutObjectActionTest, WriteObjectSuccessfulShouldWriteStateAllData) {
  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;

  // mock mark progress
  action_under_test->write_in_progress = true;

  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), is_freezed())
      .WillRepeatedly(Return(true));
  // S3Option::get_instance()->get_clovis_write_payload_size() = 1048576 * 1
  // S3_READ_AHEAD_MULTIPLE: 1
  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), get_content_length())
      .WillRepeatedly(Return(1024));
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer),
              write_content(_, _, _))
      .Times(1);

  action_under_test->write_object_successful();

  EXPECT_EQ(action_under_test->write_in_progress, true);
}

// We have some data but not all and exact to write
TEST_F(S3PutObjectActionTest,
       WriteObjectSuccessfulShouldWriteWhenExactWritableSize) {
  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;

  // mock mark progress
  action_under_test->write_in_progress = true;

  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), is_freezed())
      .WillRepeatedly(Return(false));
  // S3Option::get_instance()->get_clovis_write_payload_size() = 1048576 * 1
  // S3_READ_AHEAD_MULTIPLE: 1
  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), get_content_length())
      .WillRepeatedly(
          Return(S3Option::get_instance()->get_clovis_write_payload_size()));

  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer),
              write_content(_, _, _))
      .Times(1);

  action_under_test->write_object_successful();

  EXPECT_EQ(action_under_test->write_in_progress, true);
}

// We have some data but not all and but have more to write
TEST_F(S3PutObjectActionTest,
       WriteObjectSuccessfulShouldWriteSomeDataWhenMoreData) {
  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;

  // mock mark progress
  action_under_test->write_in_progress = true;

  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), is_freezed())
      .WillRepeatedly(Return(false));
  // S3Option::get_instance()->get_clovis_write_payload_size() = 1048576 * 1
  // S3_READ_AHEAD_MULTIPLE: 1
  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), get_content_length())
      .WillRepeatedly(Return(
          S3Option::get_instance()->get_clovis_write_payload_size() + 1024));

  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer),
              write_content(_, _, _))
      .Times(1);

  action_under_test->write_object_successful();

  EXPECT_EQ(action_under_test->write_in_progress, true);
}

// We have some data but not all and but have more to write
TEST_F(S3PutObjectActionTest, WriteObjectSuccessfulDoNextStepWhenAllIsWritten) {
  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;

  // mock mark progress
  action_under_test->write_in_progress = true;

  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), is_freezed())
      .WillRepeatedly(Return(true));
  // S3Option::get_instance()->get_clovis_write_payload_size() = 1048576 * 1
  // S3_READ_AHEAD_MULTIPLE: 1
  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), get_content_length())
      .WillRepeatedly(Return(0));

  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer),
              write_content(_, _, _))
      .Times(0);

  // Mock out the next calls on action.
  action_under_test->clear_tasks();
  action_under_test->add_task(
      std::bind(&S3PutObjectActionTest::func_callback_one, this));

  action_under_test->write_object_successful();

  EXPECT_EQ(1, call_count_one);
  EXPECT_EQ(action_under_test->write_in_progress, false);
}

// We expecting more and not enough to write
TEST_F(S3PutObjectActionTest, WriteObjectSuccessfulShouldRestartReadingData) {
  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;

  // mock mark progress
  action_under_test->write_in_progress = true;

  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), is_freezed())
      .WillRepeatedly(Return(false));
  // S3Option::get_instance()->get_clovis_write_payload_size() = 1048576 * 1
  // S3_READ_AHEAD_MULTIPLE: 1
  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), get_content_length())
      .WillRepeatedly(Return(
          S3Option::get_instance()->get_clovis_write_payload_size() - 1024));

  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer),
              write_content(_, _, _))
      .Times(0);

  EXPECT_CALL(*ptr_mock_request, resume()).Times(1);

  action_under_test->write_object_successful();

  EXPECT_EQ(action_under_test->write_in_progress, false);
}

TEST_F(S3PutObjectActionTest, SaveMetadata) {
  CREATE_BUCKET_METADATA;
  bucket_meta_factory->mock_bucket_metadata->set_object_list_index_oid(
      object_list_indx_oid);

  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;

  EXPECT_CALL(*ptr_mock_request, get_content_length_str())
      .Times(1)
      .WillOnce(Return("1024"));
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata),
              set_content_length(Eq("1024")))
      .Times(AtLeast(1));
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer), get_content_md5())
      .Times(AtLeast(1))
      .WillOnce(Return("abcd1234abcd"));
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata),
              set_md5(Eq("abcd1234abcd")))
      .Times(AtLeast(1));
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer), get_oid())
      .Times(AtLeast(1))
      .WillOnce(Return(oid));
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), set_oid(_))
      .Times(AtLeast(1));

  std::map<std::string, std::string> input_headers;
  input_headers["x-amz-meta-item-1"] = "1024";
  input_headers["x-amz-meta-item-2"] = "s3.seagate.com";

  EXPECT_CALL(*ptr_mock_request, get_in_headers_copy())
      .Times(1)
      .WillOnce(ReturnRef(input_headers));

  EXPECT_CALL(*(object_meta_factory->mock_object_metadata),
              add_user_defined_attribute(Eq("x-amz-meta-item-1"), Eq("1024")))
      .Times(AtLeast(1));
  EXPECT_CALL(
      *(object_meta_factory->mock_object_metadata),
      add_user_defined_attribute(Eq("x-amz-meta-item-2"), Eq("s3.seagate.com")))
      .Times(AtLeast(1));

  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), save(_, _))
      .Times(AtLeast(1));

  action_under_test->save_metadata();
}

TEST_F(S3PutObjectActionTest, DeleteObjectNotRequired) {
  // Mock out the next calls on action.
  action_under_test->clear_tasks();
  action_under_test->add_task(
      std::bind(&S3PutObjectActionTest::func_callback_one, this));

  action_under_test->delete_old_object_if_present();

  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3PutObjectActionTest, DeleteObjectSinceItsPresent) {
  CREATE_OBJECT_METADATA;
  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;

  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_state())
      .WillOnce(Return(S3ObjectMetadataState::present));

  struct m0_uint128 old_oid = {0x1ffff, 0x1ffff};
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_oid())
      .Times(1)
      .WillOnce(Return(old_oid));

  // Mock out the next calls on action.
  action_under_test->clear_tasks();
  action_under_test->add_task(
      std::bind(&S3PutObjectActionTest::func_callback_one, this));

  action_under_test->fetch_object_info_status();
  EXPECT_EQ(1, call_count_one);

  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer), set_oid(_))
      .Times(AtLeast(1));
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer), delete_object(_, _))
      .Times(AtLeast(1));

  action_under_test->delete_old_object_if_present();
}

TEST_F(S3PutObjectActionTest, DeleteObjectFailed) {
  // Mock out the next calls on action.
  action_under_test->clear_tasks();
  action_under_test->add_task(
      std::bind(&S3PutObjectActionTest::func_callback_one, this));

  action_under_test->delete_old_object_failed();

  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3PutObjectActionTest, SendResponseWhenShuttingDown) {
  S3Option::get_instance()->set_is_s3_shutting_down(true);

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request,
              set_out_header_value(Eq("Retry-After"), Eq("1")))
      .Times(1);
  EXPECT_CALL(*ptr_mock_request, send_response(503, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, resume()).Times(1);

  // send_response_to_s3_client is called in check_shutdown_and_rollback
  action_under_test->check_shutdown_and_rollback();

  S3Option::get_instance()->set_is_s3_shutting_down(false);
}

TEST_F(S3PutObjectActionTest, SendErrorResponse) {
  action_under_test->set_s3_error("InternalError");

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(500, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, resume()).Times(1);

  action_under_test->send_response_to_s3_client();
}

TEST_F(S3PutObjectActionTest, SendSuccessResponse) {
  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;

  action_under_test->object_metadata =
      object_meta_factory->create_object_metadata_obj(ptr_mock_request,
                                                      object_list_indx_oid);

  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_state())
      .WillOnce(Return(S3ObjectMetadataState::saved));
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer), get_content_md5())
      .Times(AtLeast(1))
      .WillOnce(Return("abcd1234abcd"));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(200, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, resume()).Times(1);

  action_under_test->send_response_to_s3_client();
}

TEST_F(S3PutObjectActionTest, SendFailedResponse) {
  action_under_test->object_metadata =
      object_meta_factory->create_object_metadata_obj(ptr_mock_request,
                                                      object_list_indx_oid);

  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_state())
      .WillOnce(Return(S3ObjectMetadataState::failed));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(500, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, resume()).Times(1);

  action_under_test->send_response_to_s3_client();
}
