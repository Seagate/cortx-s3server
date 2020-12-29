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

#include "atexit.h"
#include "mock_s3_factory.h"
#include "mock_s3_probable_delete_record.h"
#include "s3_copy_object_action.h"
#include "s3_m0_uint128_helper.h"
#include "s3_probable_delete_record.h"
#include "s3_ut_common.h"
#include "s3_test_utils.h"

using ::testing::AtLeast;
using ::testing::Eq;
using ::testing::Invoke;
using ::testing::ReturnRef;

class S3CopyObjectActionTest : public testing::Test {
 protected:
  S3CopyObjectActionTest();
  void SetUp() override;

  std::shared_ptr<MockS3RequestObject> ptr_mock_request;
  std::shared_ptr<MockS3Motr> ptr_mock_s3_motr_api;
  std::shared_ptr<MockS3BucketMetadataFactory> ptr_mock_bucket_meta_factory;
  std::shared_ptr<MockS3ObjectMetadataFactory> ptr_mock_object_meta_factory;
  std::shared_ptr<MockS3MotrReaderFactory> ptr_mock_motr_reader_factory;
  std::shared_ptr<MockS3MotrWriterFactory> ptr_mock_motr_writer_factory;
  std::shared_ptr<MockS3MotrKVSWriterFactory> ptr_mock_motr_kvs_writer_factory;

  std::unique_ptr<S3CopyObjectAction> action_under_test;

  int call_count_one;
  struct m0_uint128 oid = {0x1ffff, 0x1ffff};
  struct m0_uint128 objects_version_list_idx_oid = {0x1ffff, 0x11fff};
  struct m0_uint128 object_list_indx_oid = {0x11ffff, 0x1ffff};
  struct m0_uint128 zero_oid_idx = {};
  int layout_id;
  std::string destination_bucket_name = "detination-bucket";
  std::string destination_object_name = "destination-object";
  std::map<std::string, std::string> input_headers;

  void create_src_bucket_metadata();
  void create_src_object_metadata();

  void create_dst_bucket_metadata();
  void create_dst_object_metadata();

 public:
  void func_callback_one() { ++call_count_one; }
};

S3CopyObjectActionTest::S3CopyObjectActionTest() {
  S3Option::get_instance()->disable_auth();

  layout_id =
      S3MotrLayoutMap::get_instance()->get_best_layout_for_object_size();

  ptr_mock_s3_motr_api = std::make_shared<MockS3Motr>();
  EXPECT_CALL(*ptr_mock_s3_motr_api, m0_h_ufid_next(_))
      .WillRepeatedly(Invoke(dummy_helpers_ufid_next));

  ptr_mock_request =
      std::make_shared<MockS3RequestObject>(nullptr, new EvhtpWrapper());

  input_headers["Authorization"] = "1";
  EXPECT_CALL(*ptr_mock_request, get_in_headers_copy()).Times(1).WillOnce(
      ReturnRef(input_headers));

  EXPECT_CALL(*ptr_mock_request, get_bucket_name())
      .WillRepeatedly(ReturnRef(destination_bucket_name));

  EXPECT_CALL(*ptr_mock_request, get_object_name()).Times(AtLeast(1)).WillOnce(
      ReturnRef(destination_object_name));

  ptr_mock_bucket_meta_factory =
      std::make_shared<MockS3BucketMetadataFactory>(ptr_mock_request);

  ptr_mock_object_meta_factory = std::make_shared<MockS3ObjectMetadataFactory>(
      ptr_mock_request, ptr_mock_s3_motr_api);

  ptr_mock_motr_writer_factory = std::make_shared<MockS3MotrWriterFactory>(
      ptr_mock_request, oid, ptr_mock_s3_motr_api);

  ptr_mock_motr_reader_factory = std::make_shared<MockS3MotrReaderFactory>(
      ptr_mock_request, oid, layout_id);

  ptr_mock_motr_kvs_writer_factory =
      std::make_shared<MockS3MotrKVSWriterFactory>(ptr_mock_request,
                                                   ptr_mock_s3_motr_api);
}

void S3CopyObjectActionTest::SetUp() {
  action_under_test.reset(new S3CopyObjectAction(
      ptr_mock_request, ptr_mock_s3_motr_api, ptr_mock_bucket_meta_factory,
      ptr_mock_object_meta_factory, ptr_mock_motr_writer_factory,
      ptr_mock_motr_reader_factory, ptr_mock_motr_kvs_writer_factory));

  call_count_one = 0;
}

void S3CopyObjectActionTest::create_src_bucket_metadata() {

  EXPECT_CALL(*ptr_mock_bucket_meta_factory->mock_bucket_metadata, load(_, _))
      .Times(AtLeast(1));
  action_under_test->fetch_source_bucket_info();
}

void S3CopyObjectActionTest::create_src_object_metadata() {

  create_src_bucket_metadata();

  EXPECT_CALL(*(ptr_mock_bucket_meta_factory->mock_bucket_metadata),
              get_object_list_index_oid())
      .Times(1)
      .WillOnce(Return(object_list_indx_oid));
  EXPECT_CALL(*(ptr_mock_bucket_meta_factory->mock_bucket_metadata),
              get_objects_version_list_index_oid())
      .Times(1)
      .WillOnce(Return(objects_version_list_idx_oid));
  EXPECT_CALL(*ptr_mock_object_meta_factory->mock_object_metadata, load(_, _))
      .Times(AtLeast(1));

  action_under_test->fetch_source_object_info();
}

void S3CopyObjectActionTest::create_dst_bucket_metadata() {

  EXPECT_CALL(*ptr_mock_bucket_meta_factory->mock_bucket_metadata, load(_, _))
      .Times(AtLeast(1));
  action_under_test->fetch_bucket_info();
}

void S3CopyObjectActionTest::create_dst_object_metadata() {
  create_dst_bucket_metadata();

  EXPECT_CALL(*(ptr_mock_bucket_meta_factory->mock_bucket_metadata),
              get_object_list_index_oid())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(object_list_indx_oid));
  EXPECT_CALL(*(ptr_mock_bucket_meta_factory->mock_bucket_metadata),
              get_objects_version_list_index_oid())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(objects_version_list_idx_oid));
  EXPECT_CALL(*(ptr_mock_object_meta_factory->mock_object_metadata), load(_, _))
      .Times(AtLeast(1));
  EXPECT_CALL(*(ptr_mock_request), http_verb())
      .WillOnce(Return(S3HttpVerb::PUT));

  action_under_test->fetch_object_info();
}

TEST_F(S3CopyObjectActionTest, GetSourceBucketAndObjectSuccess) {
  EXPECT_CALL(*ptr_mock_request, get_headers_copysource()).Times(1).WillOnce(
      Return("my-source-bucket/my-source-object"));

  action_under_test->get_source_bucket_and_object();

  ASSERT_STREQ("my-source-bucket",
               action_under_test->source_bucket_name.c_str());
  ASSERT_STREQ("my-source-object",
               action_under_test->source_object_name.c_str());
}

TEST_F(S3CopyObjectActionTest, GetSourceBucketAndObjectFailure) {
  EXPECT_CALL(*ptr_mock_request, get_headers_copysource()).Times(1).WillOnce(
      Return("my-source-bucket-my-source-object"));

  action_under_test->get_source_bucket_and_object();

  ASSERT_STREQ("", action_under_test->source_bucket_name.c_str());
  ASSERT_STREQ("", action_under_test->source_object_name.c_str());
}

TEST_F(S3CopyObjectActionTest, FetchSourceBucketInfo) {
  create_src_bucket_metadata();
  EXPECT_TRUE(action_under_test->source_bucket_metadata != NULL);
}

TEST_F(S3CopyObjectActionTest, FetchSourceBucketInfoFailedMissing) {
  create_src_bucket_metadata();

  EXPECT_CALL(*(ptr_mock_bucket_meta_factory->mock_bucket_metadata),
              get_state())
      .Times(1)
      .WillOnce(Return(S3BucketMetadataState::missing));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(404, _)).Times(1);

  action_under_test->fetch_source_bucket_info_failed();

  EXPECT_STREQ("NoSuchBucket", action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3CopyObjectActionTest, FetchSourceBucketInfoFailedFailedToLaunch) {
  create_src_bucket_metadata();

  EXPECT_CALL(*(ptr_mock_bucket_meta_factory->mock_bucket_metadata),
              get_state())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(S3BucketMetadataState::failed_to_launch));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(503, _)).Times(1);

  action_under_test->fetch_source_bucket_info_failed();

  EXPECT_STREQ("ServiceUnavailable",
               action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3CopyObjectActionTest, FetchSourceBucketInfoFailedInternalError) {
  create_src_bucket_metadata();

  EXPECT_CALL(*(ptr_mock_bucket_meta_factory->mock_bucket_metadata),
              get_state())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(S3BucketMetadataState::failed));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(500, _)).Times(1);

  action_under_test->fetch_source_bucket_info_failed();

  EXPECT_STREQ("InternalError", action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3CopyObjectActionTest, FetchSourceObjectInfoListIndexGood) {
  create_src_object_metadata();

  EXPECT_TRUE(action_under_test->source_object_metadata != NULL);
}

TEST_F(S3CopyObjectActionTest, FetchSourceObjectInfoListIndexNull) {
  create_src_bucket_metadata();

  EXPECT_CALL(*(ptr_mock_bucket_meta_factory->mock_bucket_metadata),
              get_object_list_index_oid())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(zero_oid_idx));

  EXPECT_CALL(*(ptr_mock_bucket_meta_factory->mock_bucket_metadata),
              get_objects_version_list_index_oid())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(zero_oid_idx));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(404, _)).Times(1);

  action_under_test->fetch_source_object_info();

  EXPECT_STREQ("NoSuchKey", action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3CopyObjectActionTest, FetchSourceObjectInfoSuccessGreaterThan5GB) {
  create_src_object_metadata();
  EXPECT_CALL(*(ptr_mock_object_meta_factory->mock_object_metadata),
              get_content_length())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(MaxCopyObjectSourceSize + 1));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(400, _)).Times(1);

  action_under_test->fetch_source_object_info_success();

  EXPECT_STREQ("InvalidRequest",
               action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3CopyObjectActionTest, FetchSourceObjectInfoSuccessLessThan5GB) {
  create_src_object_metadata();

  EXPECT_CALL(*(ptr_mock_object_meta_factory->mock_object_metadata),
              get_content_length())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(MaxCopyObjectSourceSize - 1));

  action_under_test->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(action_under_test,
                         S3CopyObjectActionTest::func_callback_one, this);

  action_under_test->fetch_source_object_info_success();

  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3CopyObjectActionTest, ValidateCopyObjectRequestSourceBucketEmpty) {
  EXPECT_CALL(*ptr_mock_request, get_headers_copysource()).Times(1).WillOnce(
      Return("sourcebucketsourceobject"));
  EXPECT_CALL(*ptr_mock_request, get_header_value("x-amz-metadata-directive"))
      .Times(1)
      .WillOnce(Return(""));
  EXPECT_CALL(*ptr_mock_request, get_header_value("x-amz-tagging-directive"))
      .Times(1)
      .WillOnce(Return(""));
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(400, _)).Times(1);

  action_under_test->validate_copyobject_request();

  EXPECT_STREQ("InvalidArgument",
               action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3CopyObjectActionTest,
       ValidateCopyObjectRequestMetadataDirectiveReplace) {
  EXPECT_CALL(*ptr_mock_request, get_header_value("x-amz-metadata-directive"))
      .Times(1)
      .WillOnce(Return("REPLACE"));
  EXPECT_CALL(*ptr_mock_request, get_header_value("x-amz-tagging-directive"))
      .Times(1)
      .WillOnce(Return("COPY"));
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(501, _)).Times(1);

  action_under_test->validate_copyobject_request();

  EXPECT_STREQ("NotImplemented",
               action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3CopyObjectActionTest,
       ValidateCopyObjectRequestMetadataDirectiveInvalid) {
  EXPECT_CALL(*ptr_mock_request, get_header_value("x-amz-metadata-directive"))
      .Times(1)
      .WillOnce(Return("INVALID"));
  EXPECT_CALL(*ptr_mock_request, get_header_value("x-amz-tagging-directive"))
      .Times(1)
      .WillOnce(Return("COPY"));
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(400, _)).Times(1);

  action_under_test->validate_copyobject_request();

  EXPECT_STREQ("InvalidArgument",
               action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3CopyObjectActionTest,
       ValidateCopyObjectRequestTaggingDirectiveReplace) {
  EXPECT_CALL(*ptr_mock_request, get_header_value("x-amz-metadata-directive"))
      .Times(1)
      .WillOnce(Return(""));
  EXPECT_CALL(*ptr_mock_request, get_header_value("x-amz-tagging-directive"))
      .Times(1)
      .WillOnce(Return("REPLACE"));
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(501, _)).Times(1);

  action_under_test->validate_copyobject_request();

  EXPECT_STREQ("NotImplemented",
               action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3CopyObjectActionTest,
       ValidateCopyObjectRequestTaggingDirectiveInvalid) {
  EXPECT_CALL(*ptr_mock_request, get_header_value("x-amz-metadata-directive"))
      .Times(1)
      .WillOnce(Return("COPY"));
  EXPECT_CALL(*ptr_mock_request, get_header_value("x-amz-tagging-directive"))
      .Times(1)
      .WillOnce(Return("INVALID"));
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(400, _)).Times(1);

  action_under_test->validate_copyobject_request();

  EXPECT_STREQ("InvalidArgument",
               action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3CopyObjectActionTest,
       ValidateCopyObjectRequestSourceAndDestinationSame) {
  std::string destination_bucket = "my-bucket";
  std::string destination_object = "my-object";

  EXPECT_CALL(*ptr_mock_request, get_header_value("x-amz-metadata-directive"))
      .Times(1)
      .WillOnce(Return(""));
  EXPECT_CALL(*ptr_mock_request, get_header_value("x-amz-tagging-directive"))
      .Times(1)
      .WillOnce(Return(""));
  EXPECT_CALL(*ptr_mock_request, get_headers_copysource()).Times(1).WillOnce(
      Return("my-bucket/my-object"));

  EXPECT_CALL(*ptr_mock_request, get_bucket_name())
      .Times(AtLeast(1))
      .WillRepeatedly(ReturnRef(destination_bucket));

  EXPECT_CALL(*ptr_mock_request, get_object_name())
      .Times(AtLeast(1))
      .WillRepeatedly(ReturnRef(destination_object));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(400, _)).Times(1);

  action_under_test->validate_copyobject_request();

  ASSERT_STREQ("my-bucket", action_under_test->source_bucket_name.c_str());
  ASSERT_STREQ("my-object", action_under_test->source_object_name.c_str());
  EXPECT_EQ(action_under_test->s3_put_action_state,
            S3PutObjectActionState::validationFailed);
  EXPECT_STREQ("InvalidRequest",
               action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3CopyObjectActionTest, ValidateCopyObjectRequestSuccess) {
  std::string destination_bucket = "my-destination-bucket";
  EXPECT_CALL(*ptr_mock_request, get_header_value("x-amz-metadata-directive"))
      .Times(1)
      .WillOnce(Return(""));
  EXPECT_CALL(*ptr_mock_request, get_header_value("x-amz-tagging-directive"))
      .Times(1)
      .WillOnce(Return(""));
  EXPECT_CALL(*ptr_mock_request, get_headers_copysource()).Times(1).WillOnce(
      Return("my-source-bucket/my-source-object"));

  EXPECT_CALL(*ptr_mock_request, get_bucket_name())
      .Times(AtLeast(1))
      .WillRepeatedly(ReturnRef(destination_bucket));

  EXPECT_CALL(*ptr_mock_request, get_object_name()).Times(0);

  EXPECT_CALL(*(ptr_mock_bucket_meta_factory->mock_bucket_metadata), load(_, _))
      .Times(AtLeast(1));

  action_under_test->validate_copyobject_request();

  EXPECT_TRUE(action_under_test->source_bucket_metadata != NULL);
}

TEST_F(S3CopyObjectActionTest,
       FetchDestinationBucketInfoFailedMetadataMissing) {
  create_dst_bucket_metadata();
  EXPECT_CALL(*(ptr_mock_bucket_meta_factory->mock_bucket_metadata),
              get_state())
      .WillRepeatedly(Return(S3BucketMetadataState::missing));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(404, _)).Times(1);

  action_under_test->fetch_bucket_info_failed();

  EXPECT_STREQ("NoSuchBucket", action_under_test->get_s3_error_code().c_str());
  EXPECT_TRUE(action_under_test->bucket_metadata != NULL);
  EXPECT_TRUE(action_under_test->object_metadata == NULL);
}

TEST_F(S3CopyObjectActionTest, FetchDestinationBucketInfoFailedFailedToLaunch) {
  create_dst_bucket_metadata();
  EXPECT_CALL(*(ptr_mock_bucket_meta_factory->mock_bucket_metadata),
              get_state())
      .WillRepeatedly(Return(S3BucketMetadataState::failed_to_launch));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(503, _)).Times(1);

  action_under_test->fetch_bucket_info_failed();

  EXPECT_STREQ("ServiceUnavailable",
               action_under_test->get_s3_error_code().c_str());
  EXPECT_TRUE(action_under_test->bucket_metadata != NULL);
  EXPECT_TRUE(action_under_test->object_metadata == NULL);
}

TEST_F(S3CopyObjectActionTest, FetchDestinationBucketInfoFailedInternalError) {
  create_dst_bucket_metadata();
  EXPECT_CALL(*(ptr_mock_bucket_meta_factory->mock_bucket_metadata),
              get_state())
      .WillRepeatedly(Return(S3BucketMetadataState::failed));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(500, _)).Times(1);

  action_under_test->fetch_bucket_info_failed();

  EXPECT_STREQ("InternalError", action_under_test->get_s3_error_code().c_str());
  EXPECT_TRUE(action_under_test->bucket_metadata != NULL);
  EXPECT_TRUE(action_under_test->object_metadata == NULL);
}

TEST_F(S3CopyObjectActionTest, FetchDestinationObjectInfoFailed) {
  create_dst_object_metadata();

  action_under_test->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(action_under_test,
                         S3CopyObjectActionTest::func_callback_one, this);

  action_under_test->fetch_object_info_failed();

  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3CopyObjectActionTest, FetchDestinationObjectInfoSuccess) {
  create_dst_object_metadata();

  EXPECT_CALL(*ptr_mock_object_meta_factory->mock_object_metadata, get_state())
      .Times(1)
      .WillOnce(Return(S3ObjectMetadataState::missing));

  action_under_test->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(action_under_test,
                         S3CopyObjectActionTest::func_callback_one, this);

  action_under_test->fetch_object_info_success();

  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3CopyObjectActionTest, CreateObjectFirstAttempt) {
  action_under_test->total_data_to_stream = 1024;

  EXPECT_CALL(*ptr_mock_motr_writer_factory->mock_motr_writer,
              create_object(_, _, _)).Times(1);

  action_under_test->create_object();

  EXPECT_TRUE(action_under_test->motr_writer);
  EXPECT_TRUE(action_under_test->layout_id > 0);
  EXPECT_TRUE(action_under_test->motr_unit_size > 0);
}

TEST_F(S3CopyObjectActionTest, CreateObjectSecondAttempt) {
  action_under_test->total_data_to_stream = 1024;
  action_under_test->create_object();

  action_under_test->tried_count = 1;

  EXPECT_CALL(*ptr_mock_motr_writer_factory->mock_motr_writer, set_oid(_))
      .Times(1);
  EXPECT_CALL(*ptr_mock_motr_writer_factory->mock_motr_writer,
              create_object(_, _, _)).Times(1);

  action_under_test->create_object();
}

TEST_F(S3CopyObjectActionTest, CreateObjectFailedTestWhileShutdown) {
  S3Option::get_instance()->set_is_s3_shutting_down(true);

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(503, _)).Times(1);

  action_under_test->create_object_failed();

  S3Option::get_instance()->set_is_s3_shutting_down(false);
}

TEST_F(S3CopyObjectActionTest, CreateObjectFailedWithCollisionExceededRetry) {
  action_under_test->motr_writer =
      ptr_mock_motr_writer_factory->mock_motr_writer;
  EXPECT_CALL(*ptr_mock_motr_writer_factory->mock_motr_writer, get_state())
      .Times(1)
      .WillOnce(Return(S3MotrWiterOpState::exists));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(500, _)).Times(1);

  action_under_test->tried_count = MAX_COLLISION_RETRY_COUNT;
  action_under_test->create_object_failed();

  EXPECT_STREQ("InternalError", action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3CopyObjectActionTest, CreateObjectFailedWithCollisionRetry) {
  action_under_test->motr_writer =
      ptr_mock_motr_writer_factory->mock_motr_writer;
  action_under_test->total_data_to_stream = 1024;
  action_under_test->tried_count = MAX_COLLISION_RETRY_COUNT - 1;

  EXPECT_CALL(*ptr_mock_motr_writer_factory->mock_motr_writer, get_state())
      .Times(1)
      .WillOnce(Return(S3MotrWiterOpState::exists));
  EXPECT_CALL(*ptr_mock_motr_writer_factory->mock_motr_writer, set_oid(_))
      .Times(1);
  EXPECT_CALL(*ptr_mock_motr_writer_factory->mock_motr_writer,
              create_object(_, _, _)).Times(1);

  action_under_test->create_object_failed();
}

TEST_F(S3CopyObjectActionTest, CreateObjectFailedTest) {
  action_under_test->total_data_to_stream = 1024;
  EXPECT_CALL(*ptr_mock_motr_writer_factory->mock_motr_writer,
              create_object(_, _, _)).Times(1);
  action_under_test->create_object();

  EXPECT_CALL(*ptr_mock_motr_writer_factory->mock_motr_writer, get_state())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(S3MotrWiterOpState::failed));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(_, _)).Times(1);

  action_under_test->create_object_failed();
  EXPECT_STREQ("InternalError", action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3CopyObjectActionTest, CreateObjectFailedToLaunchTest) {
  action_under_test->total_data_to_stream = 1024;
  EXPECT_CALL(*ptr_mock_motr_writer_factory->mock_motr_writer,
              create_object(_, _, _)).Times(1);
  action_under_test->create_object();

  EXPECT_CALL(*ptr_mock_motr_writer_factory->mock_motr_writer, get_state())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(S3MotrWiterOpState::failed_to_launch));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(503, _)).Times(1);

  action_under_test->create_object_failed();
  EXPECT_STREQ("ServiceUnavailable",
               action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3CopyObjectActionTest, CreateNewOidTest) {
  struct m0_uint128 old_oid = action_under_test->new_object_oid;

  action_under_test->create_new_oid(old_oid);

  EXPECT_OID_NE(old_oid, action_under_test->new_object_oid);
}

TEST_F(S3CopyObjectActionTest, ZeroSizeObject) {
  action_under_test->total_data_to_stream = 0;
  action_under_test->clear_tasks();

  ACTION_TASK_ADD_OBJPTR(action_under_test,
                         S3CopyObjectActionTest::func_callback_one, this);
  action_under_test->copy_object();

  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3CopyObjectActionTest, NonZeroSizeObject) {
  action_under_test->total_data_to_stream = 1024;
  action_under_test->_set_layout_id(1);

  create_src_object_metadata();

  EXPECT_CALL(*ptr_mock_motr_reader_factory->mock_motr_reader,
              set_last_index(0)).Times(1);
  action_under_test->copy_object();

  EXPECT_TRUE(action_under_test->motr_reader);
  EXPECT_EQ(action_under_test->bytes_left_to_read,
            action_under_test->total_data_to_stream);
}

TEST_F(S3CopyObjectActionTest, ReadDataBlockStarted) {
  action_under_test->total_data_to_stream = 1024;
  action_under_test->bytes_left_to_read = 1024;
  action_under_test->_set_layout_id(1);

  action_under_test->motr_reader =
      ptr_mock_motr_reader_factory->mock_motr_reader;

  EXPECT_CALL(*ptr_mock_motr_reader_factory->mock_motr_reader,
              read_object_data(_, _, _))
      .Times(1)
      .WillOnce(Return(true));

  action_under_test->read_data_block();

  EXPECT_TRUE(action_under_test->read_in_progress);
}

TEST_F(S3CopyObjectActionTest, ReadDataBlockFailedToStart) {
  action_under_test->total_data_to_stream = 1024;
  action_under_test->bytes_left_to_read = 1024;
  action_under_test->_set_layout_id(1);

  action_under_test->motr_reader =
      ptr_mock_motr_reader_factory->mock_motr_reader;
  action_under_test->motr_writer =
      ptr_mock_motr_writer_factory->mock_motr_writer;

  EXPECT_CALL(*ptr_mock_motr_reader_factory->mock_motr_reader,
              read_object_data(_, _, _))
      .Times(1)
      .WillOnce(Return(false));
  EXPECT_CALL(*ptr_mock_motr_reader_factory->mock_motr_reader, get_state())
      .Times(1)
      .WillOnce(Return(S3MotrReaderOpState::failed_to_launch));

  action_under_test->read_data_block();

  EXPECT_FALSE(action_under_test->read_in_progress);
  EXPECT_TRUE(action_under_test->copy_failed);
  EXPECT_FALSE(action_under_test->get_s3_error_code().empty());
}

TEST_F(S3CopyObjectActionTest, ReadDataBlockSuccessWhileShuttingDown) {
  action_under_test->_set_layout_id(layout_id);
  action_under_test->read_in_progress = true;
  S3Option::get_instance()->set_is_s3_shutting_down(true);

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(503, _)).Times(1);

  action_under_test->read_data_block_success();

  EXPECT_FALSE(action_under_test->read_in_progress);
  S3Option::get_instance()->set_is_s3_shutting_down(false);
}

TEST_F(S3CopyObjectActionTest, ReadDataBlockSuccessCopyFailed) {
  action_under_test->total_data_to_stream = 1024;
  action_under_test->bytes_left_to_read = 1024;
  action_under_test->_set_layout_id(1);
  action_under_test->read_in_progress = true;

  action_under_test->copy_failed = true;
  action_under_test->set_s3_error("InternalError");

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(_, _)).Times(1);

  action_under_test->read_data_block_success();

  EXPECT_FALSE(action_under_test->read_in_progress);
}

TEST_F(S3CopyObjectActionTest, ReadDataBlockSuccessShouldStartWrite) {
  action_under_test->total_data_to_stream = 5120;
  action_under_test->bytes_left_to_read = 1024;
  action_under_test->read_in_progress = true;
  action_under_test->_set_layout_id(1);

  action_under_test->motr_reader =
      ptr_mock_motr_reader_factory->mock_motr_reader;
  action_under_test->motr_writer =
      ptr_mock_motr_writer_factory->mock_motr_writer;

  S3BufferSequence data_blocks_read;
  data_blocks_read.emplace_back(nullptr, 0);

  EXPECT_CALL(*ptr_mock_motr_reader_factory->mock_motr_reader,
              extract_blocks_read())
      .Times(1)
      .WillOnce(Return(data_blocks_read));

  EXPECT_CALL(*ptr_mock_motr_writer_factory->mock_motr_writer,
              write_content(_, _, _, _)).Times(1);
  EXPECT_CALL(*ptr_mock_motr_writer_factory->mock_motr_writer, get_state())
      .Times(1);

  action_under_test->read_data_block_success();

  EXPECT_TRUE(action_under_test->write_in_progress);
}

TEST_F(S3CopyObjectActionTest, ReadDataBlockFailed) {
  action_under_test->total_data_to_stream = 5120;
  action_under_test->bytes_left_to_read = 1024;
  action_under_test->read_in_progress = true;
  action_under_test->_set_layout_id(1);

  action_under_test->motr_reader =
      ptr_mock_motr_reader_factory->mock_motr_reader;
  action_under_test->motr_writer =
      ptr_mock_motr_writer_factory->mock_motr_writer;

  EXPECT_CALL(*ptr_mock_motr_reader_factory->mock_motr_reader, get_state())
      .Times(1);
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(_, _)).Times(1);

  action_under_test->read_data_block_failed();

  EXPECT_FALSE(action_under_test->read_in_progress);
  EXPECT_TRUE(action_under_test->copy_failed);
  EXPECT_EQ(action_under_test->s3_put_action_state,
            S3PutObjectActionState::writeFailed);
  EXPECT_FALSE(action_under_test->get_s3_error_code().empty());
}

TEST_F(S3CopyObjectActionTest, WriteObjectStarted) {
  action_under_test->motr_writer =
      ptr_mock_motr_writer_factory->mock_motr_writer;
  action_under_test->motr_reader =
      ptr_mock_motr_reader_factory->mock_motr_reader;

  action_under_test->_set_layout_id(1);
  action_under_test->total_data_to_stream = 5120;
  action_under_test->bytes_left_to_read = 1024;
  action_under_test->data_blocks_read.emplace_back(nullptr, 0);

  EXPECT_CALL(*ptr_mock_motr_writer_factory->mock_motr_writer,
              write_content(_, _, _, _)).Times(1);
  EXPECT_CALL(*ptr_mock_motr_writer_factory->mock_motr_writer, get_state())
      .Times(1)
      .WillOnce(Return(S3MotrWiterOpState::start));

  action_under_test->write_data_block();

  EXPECT_TRUE(action_under_test->write_in_progress);
  EXPECT_TRUE(action_under_test->data_blocks_read.empty());
  EXPECT_FALSE(action_under_test->data_blocks_writing.empty());
}

TEST_F(S3CopyObjectActionTest, WriteObjectFailedShouldUndoMarkProgress) {
  action_under_test->motr_writer =
      ptr_mock_motr_writer_factory->mock_motr_writer;
  action_under_test->_set_layout_id(layout_id);

  // mock mark progress
  action_under_test->write_in_progress = true;
  action_under_test->new_oid_str = S3M0Uint128Helper::to_string(oid);
  MockS3ProbableDeleteRecord *prob_rec = new MockS3ProbableDeleteRecord(
      action_under_test->new_oid_str, {}, "abc_obj", oid, layout_id,
      object_list_indx_oid, objects_version_list_idx_oid,
      "",     // Version does not exists yet
      false,  // force_delete
      false,  // is_multipart
      {});
  action_under_test->new_probable_del_rec.reset(prob_rec);

  EXPECT_CALL(*ptr_mock_motr_writer_factory->mock_motr_writer, get_state())
      .Times(1)
      .WillOnce(Return(S3MotrWiterOpState::failed));
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(500, _)).Times(1);

  action_under_test->write_data_block_failed();

  EXPECT_STREQ("InternalError", action_under_test->get_s3_error_code().c_str());
  EXPECT_FALSE(action_under_test->write_in_progress);
}

TEST_F(S3CopyObjectActionTest, WriteObjectFailedDuetoEntityOpenFailure) {
  action_under_test->motr_writer =
      ptr_mock_motr_writer_factory->mock_motr_writer;
  action_under_test->_set_layout_id(layout_id);

  // mock mark progress
  action_under_test->write_in_progress = true;
  action_under_test->new_oid_str = S3M0Uint128Helper::to_string(oid);
  MockS3ProbableDeleteRecord *prob_rec = new MockS3ProbableDeleteRecord(
      action_under_test->new_oid_str, {}, "abc_obj", oid, layout_id,
      object_list_indx_oid, objects_version_list_idx_oid,
      "",     // Version does not exists yet
      false,  // force_delete
      false,  // is_multipart
      {});
  action_under_test->new_probable_del_rec.reset(prob_rec);

  EXPECT_CALL(*ptr_mock_motr_writer_factory->mock_motr_writer, get_state())
      .Times(1)
      .WillOnce(Return(S3MotrWiterOpState::failed_to_launch));
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(503, _)).Times(1);

  action_under_test->write_data_block_failed();

  EXPECT_FALSE(action_under_test->write_in_progress);
  EXPECT_STREQ("ServiceUnavailable",
               action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3CopyObjectActionTest, WriteObjectSuccessfulWhileShuttingDown) {
  S3Option::get_instance()->set_is_s3_shutting_down(true);
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(503, _)).Times(1);

  action_under_test->_set_layout_id(layout_id);

  // mock mark progress
  action_under_test->write_in_progress = true;

  action_under_test->write_data_block_success();

  S3Option::get_instance()->set_is_s3_shutting_down(false);

  EXPECT_FALSE(action_under_test->write_in_progress);
}

TEST_F(S3CopyObjectActionTest, WriteObjectSuccessfulShouldRestartWritingData) {
  action_under_test->motr_writer =
      ptr_mock_motr_writer_factory->mock_motr_writer;
  action_under_test->motr_reader =
      ptr_mock_motr_reader_factory->mock_motr_reader;

  action_under_test->_set_layout_id(layout_id);
  action_under_test->total_data_to_stream = 5120;
  action_under_test->bytes_left_to_read = 1024;
  action_under_test->write_in_progress = true;
  action_under_test->data_blocks_read.emplace_back(nullptr, 0);

  EXPECT_CALL(*ptr_mock_motr_writer_factory->mock_motr_writer,
              write_content(_, _, _, _)).Times(1);
  EXPECT_CALL(*ptr_mock_motr_writer_factory->mock_motr_writer, get_state())
      .Times(1)
      .WillOnce(Return(S3MotrWiterOpState::start));

  action_under_test->write_data_block_success();

  EXPECT_TRUE(action_under_test->write_in_progress);
}

TEST_F(S3CopyObjectActionTest,
       WriteObjectSuccessfulDoNextStepWhenAllIsWritten) {
  action_under_test->bytes_left_to_read = 0;
  action_under_test->write_in_progress = true;

  EXPECT_CALL(*ptr_mock_motr_writer_factory->mock_motr_writer,
              write_content(_, _, _, _)).Times(0);
  EXPECT_CALL(*ptr_mock_motr_reader_factory->mock_motr_reader,
              read_object_data(_, _, _)).Times(0);

  action_under_test->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(action_under_test,
                         S3CopyObjectActionTest::func_callback_one, this);

  action_under_test->write_data_block_success();

  EXPECT_EQ(1, call_count_one);
  EXPECT_FALSE(action_under_test->write_in_progress);
  EXPECT_EQ(action_under_test->s3_put_action_state,
            S3PutObjectActionState::writeComplete);
}

TEST_F(S3CopyObjectActionTest, SaveMetadata) {
  action_under_test->total_data_to_stream = 1024;

  create_dst_bucket_metadata();
  create_src_object_metadata();

  ptr_mock_bucket_meta_factory->mock_bucket_metadata->set_object_list_index_oid(
      object_list_indx_oid);

  action_under_test->new_object_metadata =
      ptr_mock_object_meta_factory->mock_object_metadata;
  action_under_test->new_oid_str = S3M0Uint128Helper::to_string(oid);

  action_under_test->motr_writer =
      ptr_mock_motr_writer_factory->mock_motr_writer;

  EXPECT_CALL(*ptr_mock_object_meta_factory->mock_object_metadata,
              reset_date_time_to_current()).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_object_meta_factory->mock_object_metadata,
              set_content_length(Eq("1024"))).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_motr_writer_factory->mock_motr_writer,
              get_content_md5())
      .Times(AtLeast(1))
      .WillOnce(Return("abcd1234abcd"));
  EXPECT_CALL(*ptr_mock_object_meta_factory->mock_object_metadata,
              set_md5(Eq("abcd1234abcd"))).Times(AtLeast(1));

  EXPECT_CALL(*ptr_mock_object_meta_factory->mock_object_metadata, setacl(_))
      .Times(1);

  std::map<std::string, std::string> tagsmap;
  EXPECT_CALL(*ptr_mock_object_meta_factory->mock_object_metadata, get_tags())
      .Times(1)
      .WillOnce(ReturnRef(tagsmap));
  EXPECT_CALL(*ptr_mock_object_meta_factory->mock_object_metadata,
              set_tags(tagsmap)).Times(1);

  std::map<std::string, std::string> meta_map{{"key", "value"}};
  EXPECT_CALL(*ptr_mock_object_meta_factory->mock_object_metadata,
              get_user_attributes())
      .Times(1)
      .WillOnce(ReturnRef(meta_map));
  EXPECT_CALL(*ptr_mock_object_meta_factory->mock_object_metadata,
              add_user_defined_attribute("key", "value")).Times(1);
  /* TODO: uncomment once handling x-amz-metadata-directive input header feature
     is implemented
  input_headers["x-amz-meta-item-1"] = "1024";
  input_headers["x-amz-meta-item-2"] = "s3.seagate.com";

  EXPECT_CALL(*ptr_mock_request, get_in_headers_copy()).Times(1).WillOnce(
      ReturnRef(input_headers));

  EXPECT_CALL(*ptr_mock_object_meta_factory->mock_object_metadata,
              add_user_defined_attribute(Eq("x-amz-meta-item-1"), Eq("1024")))
      .Times(AtLeast(1));
  EXPECT_CALL(
      *ptr_mock_object_meta_factory->mock_object_metadata,
      add_user_defined_attribute(Eq("x-amz-meta-item-2"), Eq("s3.seagate.com")))
      .Times(AtLeast(1));
  */
  EXPECT_CALL(*ptr_mock_object_meta_factory->mock_object_metadata, save(_, _))
      .Times(AtLeast(1));

  action_under_test->save_metadata();
}

TEST_F(S3CopyObjectActionTest, SaveObjectMetadataFailed) {
  create_dst_object_metadata();
  ptr_mock_bucket_meta_factory->mock_bucket_metadata->set_object_list_index_oid(
      object_list_indx_oid);
  action_under_test->new_object_metadata =
      ptr_mock_object_meta_factory->mock_object_metadata;

  action_under_test->new_oid_str = S3M0Uint128Helper::to_string(oid);

  action_under_test->motr_writer =
      ptr_mock_motr_writer_factory->mock_motr_writer;

  EXPECT_CALL(*ptr_mock_object_meta_factory->mock_object_metadata, get_state())
      .WillRepeatedly(Return(S3ObjectMetadataState::failed));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(500, _)).Times(AtLeast(1));

  MockS3ProbableDeleteRecord *prob_rec = new MockS3ProbableDeleteRecord(
      action_under_test->new_oid_str, {0ULL, 0ULL}, "abc_obj", oid, layout_id,
      object_list_indx_oid, objects_version_list_idx_oid,
      "",     // Version does not exists yet
      false,  // force_delete
      false,  // is_multipart
      {0ULL, 0ULL});
  action_under_test->new_probable_del_rec.reset(prob_rec);

  action_under_test->clear_tasks();
  action_under_test->save_object_metadata_failed();

  EXPECT_STREQ("InternalError", action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3CopyObjectActionTest, SendResponseWhenShuttingDown) {
  S3Option::get_instance()->set_is_s3_shutting_down(true);

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request,
              set_out_header_value(Eq("Retry-After"), Eq("1"))).Times(1);
  EXPECT_CALL(*ptr_mock_request, send_response(503, _)).Times(AtLeast(1));

  // send_response_to_s3_client is called in check_shutdown_and_rollback
  action_under_test->check_shutdown_and_rollback();

  S3Option::get_instance()->set_is_s3_shutting_down(false);
}

TEST_F(S3CopyObjectActionTest, SendErrorResponse) {
  action_under_test->set_s3_error("InternalError");

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(500, _)).Times(AtLeast(1));

  action_under_test->send_response_to_s3_client();
}

TEST_F(S3CopyObjectActionTest, SendSuccessResponse) {
  create_dst_object_metadata();
  ptr_mock_bucket_meta_factory->mock_bucket_metadata->set_object_list_index_oid(
      object_list_indx_oid);
  action_under_test->new_object_metadata =
      ptr_mock_object_meta_factory->mock_object_metadata;

  action_under_test->motr_writer =
      ptr_mock_motr_writer_factory->mock_motr_writer;

  // Simulate success
  action_under_test->s3_put_action_state =
      S3PutObjectActionState::metadataSaved;

  // expectations for remove_new_oid_probable_record()
  action_under_test->new_oid_str = S3M0Uint128Helper::to_string(oid);

  EXPECT_CALL(*(ptr_mock_object_meta_factory->mock_object_metadata), get_md5())
      .Times(1);
  EXPECT_CALL(*(ptr_mock_object_meta_factory->mock_object_metadata),
              get_last_modified_iso()).Times(1);
  EXPECT_CALL(*ptr_mock_request, send_response(200, _)).Times(AtLeast(1));

  action_under_test->send_response_to_s3_client();

  EXPECT_EQ(action_under_test->s3_put_action_state,
            S3PutObjectActionState::completed);
}

TEST_F(S3CopyObjectActionTest, SendFailedResponse) {
  action_under_test->set_s3_error("InternalError");
  action_under_test->s3_put_action_state =
      S3PutObjectActionState::validationFailed;

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(500, _)).Times(AtLeast(1));

  action_under_test->send_response_to_s3_client();
}

TEST_F(S3CopyObjectActionTest, DestinationAuthorization) {

  create_dst_bucket_metadata();
  action_under_test->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(action_under_test,
                         S3CopyObjectActionTest::func_callback_one, this);
  action_under_test->bucket_metadata =
      action_under_test->bucket_metadata_factory->create_bucket_metadata_obj(
          ptr_mock_request);

  std::string MockJsonResponse("Mockresponse");
  EXPECT_CALL(*(ptr_mock_bucket_meta_factory->mock_bucket_metadata),
              get_state())
      .WillRepeatedly(Return(S3BucketMetadataState::present));
  EXPECT_CALL(*(ptr_mock_bucket_meta_factory->mock_bucket_metadata),
              get_policy_as_json()).WillRepeatedly(ReturnRef(MockJsonResponse));
  action_under_test->set_authorization_meta();
  EXPECT_EQ(1, call_count_one);
}
