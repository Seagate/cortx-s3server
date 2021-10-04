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
  struct s3_motr_idx_layout objects_version_list_index_layout = {
      {0x1ffff, 0x11fff}};
  struct s3_motr_idx_layout object_list_index_layout = {{0x11ffff, 0x1ffff}};
  struct s3_motr_idx_layout zero_index_layout = {};
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

  EXPECT_CALL(*ptr_mock_request, get_object_name())
      .Times(AtLeast(1))
      .WillRepeatedly(ReturnRef(destination_object_name));

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
  action_under_test->fetch_additional_bucket_info();
}

void S3CopyObjectActionTest::create_src_object_metadata() {

  create_src_bucket_metadata();

  EXPECT_CALL(*(ptr_mock_bucket_meta_factory->mock_bucket_metadata),
              get_object_list_index_layout())
      .Times(1)
      .WillOnce(ReturnRef(object_list_index_layout));
  EXPECT_CALL(*(ptr_mock_bucket_meta_factory->mock_bucket_metadata),
              get_objects_version_list_index_layout())
      .Times(1)
      .WillOnce(ReturnRef(objects_version_list_index_layout));
  EXPECT_CALL(*ptr_mock_object_meta_factory->mock_object_metadata, load(_, _))
      .Times(AtLeast(1));

  action_under_test->fetch_additional_object_info();
}

void S3CopyObjectActionTest::create_dst_bucket_metadata() {

  EXPECT_CALL(*ptr_mock_bucket_meta_factory->mock_bucket_metadata, load(_, _))
      .Times(AtLeast(1));
  action_under_test->fetch_bucket_info();
}

void S3CopyObjectActionTest::create_dst_object_metadata() {
  create_dst_bucket_metadata();

  EXPECT_CALL(*(ptr_mock_bucket_meta_factory->mock_bucket_metadata),
              get_object_list_index_layout())
      .Times(AtLeast(1))
      .WillRepeatedly(ReturnRef(object_list_index_layout));
  EXPECT_CALL(*(ptr_mock_bucket_meta_factory->mock_bucket_metadata),
              get_objects_version_list_index_layout())
      .Times(AtLeast(1))
      .WillRepeatedly(ReturnRef(objects_version_list_index_layout));
  EXPECT_CALL(*(ptr_mock_object_meta_factory->mock_object_metadata), load(_, _))
      .Times(AtLeast(1));
  EXPECT_CALL(*(ptr_mock_request), http_verb())
      .WillOnce(Return(S3HttpVerb::PUT));

  action_under_test->fetch_object_info();
}

TEST_F(S3CopyObjectActionTest, ValidateCopyObjectRequestSourceBucketEmpty) {
  action_under_test->additional_bucket_name = std::string();
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

  action_under_test->additional_bucket_name = "my-bucket";
  action_under_test->additional_object_name = "my-object";

  EXPECT_CALL(*ptr_mock_request, get_header_value("x-amz-metadata-directive"))
      .Times(1)
      .WillOnce(Return(""));

  EXPECT_CALL(*ptr_mock_request, get_header_value("x-amz-tagging-directive"))
      .Times(1)
      .WillOnce(Return(""));

  EXPECT_CALL(*ptr_mock_request, get_bucket_name())
      .Times(AtLeast(1))
      .WillRepeatedly(ReturnRef(destination_bucket));

  EXPECT_CALL(*ptr_mock_request, get_object_name())
      .Times(AtLeast(1))
      .WillRepeatedly(ReturnRef(destination_object));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(400, _)).Times(1);

  action_under_test->validate_copyobject_request();

  EXPECT_EQ(action_under_test->s3_put_action_state,
            S3PutObjectActionState::validationFailed);
  EXPECT_STREQ("InvalidRequest",
               action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3CopyObjectActionTest, ValidateCopyObjectRequestSuccess) {
  std::string destination_bucket = "my-destination-bucket";
  action_under_test->additional_bucket_name = "my-source-bucket";
  action_under_test->additional_object_name = "my-source-object";
  action_under_test->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(action_under_test,
                         S3CopyObjectActionTest::func_callback_one, this);

  EXPECT_CALL(*ptr_mock_request, get_header_value("x-amz-metadata-directive"))
      .Times(1)
      .WillOnce(Return(""));
  EXPECT_CALL(*ptr_mock_request, get_header_value("x-amz-tagging-directive"))
      .Times(1)
      .WillOnce(Return(""));

  action_under_test->additional_object_metadata =
      action_under_test->object_metadata_factory->create_object_metadata_obj(
          ptr_mock_request);

  EXPECT_CALL(*ptr_mock_object_meta_factory->mock_object_metadata,
              get_content_length())
      .Times(2)
      .WillRepeatedly(Return(1024));

  EXPECT_CALL(*ptr_mock_request, get_bucket_name())
      .Times(AtLeast(1))
      .WillRepeatedly(ReturnRef(destination_bucket));

  EXPECT_CALL(*ptr_mock_request, get_object_name()).Times(0);

  action_under_test->validate_copyobject_request();

  EXPECT_EQ(1, call_count_one);
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

TEST_F(S3CopyObjectActionTest, CreateOneOrMoreObjects) {

  EXPECT_CALL(*ptr_mock_motr_writer_factory->mock_motr_writer,
              create_object(_, _, _, _)).Times(AtLeast(1));

  action_under_test->create_object();
}

TEST_F(S3CopyObjectActionTest, CreateObjectFirstAttempt) {
  action_under_test->total_data_to_stream = 1024;

  EXPECT_CALL(*ptr_mock_motr_writer_factory->mock_motr_writer,
              create_object(_, _, _, _)).Times(1);

  action_under_test->create_object();

  EXPECT_TRUE(action_under_test->motr_writer);
  EXPECT_TRUE(action_under_test->layout_id > 0);
  EXPECT_TRUE(action_under_test->motr_unit_size > 0);
}

TEST_F(S3CopyObjectActionTest, CreateObjectSecondAttempt) {
  action_under_test->total_data_to_stream = 1024;
  action_under_test->create_object();

  action_under_test->tried_count = 1;

  EXPECT_CALL(*ptr_mock_motr_writer_factory->mock_motr_writer,
              create_object(_, _, _, _)).Times(1);

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
  EXPECT_CALL(*ptr_mock_motr_writer_factory->mock_motr_writer,
              create_object(_, _, _, _)).Times(1);

  action_under_test->create_object_failed();
}

TEST_F(S3CopyObjectActionTest, CreateObjectFailedTest) {
  action_under_test->total_data_to_stream = 1024;
  EXPECT_CALL(*ptr_mock_motr_writer_factory->mock_motr_writer,
              create_object(_, _, _, _)).Times(1);
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
              create_object(_, _, _, _)).Times(1);
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

TEST_F(S3CopyObjectActionTest, CopyFragments) {
  action_under_test->total_parts_fragment_to_be_copied = 3;

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(0);
  EXPECT_CALL(*ptr_mock_request, send_reply_start(_)).Times(0);
  EXPECT_CALL(*ptr_mock_request, send_reply_body(_, _)).Times(0);

  action_under_test->copy_fragments();
}

TEST_F(S3CopyObjectActionTest, ZeroSizeObject) {
  action_under_test->total_data_to_stream = 0;
  action_under_test->clear_tasks();

  ACTION_TASK_ADD_OBJPTR(action_under_test,
                         S3CopyObjectActionTest::func_callback_one, this);
  action_under_test->copy_object();

  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3CopyObjectActionTest, SaveMetadata) {
  action_under_test->total_data_to_stream = 1024;

  create_dst_bucket_metadata();
  create_src_object_metadata();

  ptr_mock_bucket_meta_factory->mock_bucket_metadata
      ->set_object_list_index_layout(object_list_index_layout);

  action_under_test->new_object_metadata =
      ptr_mock_object_meta_factory->mock_object_metadata;
  action_under_test->new_oid_str = S3M0Uint128Helper::to_string(oid);

  action_under_test->motr_writer =
      ptr_mock_motr_writer_factory->mock_motr_writer;

  EXPECT_CALL(*ptr_mock_object_meta_factory->mock_object_metadata,
              reset_date_time_to_current()).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_object_meta_factory->mock_object_metadata,
              set_content_length(Eq("1024"))).Times(AtLeast(1));

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
  ptr_mock_bucket_meta_factory->mock_bucket_metadata
      ->set_object_list_index_layout(object_list_index_layout);
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
      "mock_pvid", object_list_index_layout.oid,
      objects_version_list_index_layout.oid, "",  // Version does not exists yet
      false,                                      // force_delete
      false,                                      // is_multipart
      {0ULL, 0ULL});
  action_under_test->new_probable_del_rec.reset(prob_rec);

  action_under_test->clear_tasks();
  action_under_test->save_object_metadata_failed();

  EXPECT_STREQ("InternalError", action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3CopyObjectActionTest, SendResponseWhenShuttingDown) {
  S3Option::get_instance()->set_is_s3_shutting_down(true);
  int retry_after_period = S3Option::get_instance()->get_s3_retry_after_sec();
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request,
              set_out_header_value(Eq("Retry-After"),
                                   Eq(std::to_string(retry_after_period))))
      .Times(1);
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
  ptr_mock_bucket_meta_factory->mock_bucket_metadata
      ->set_object_list_index_layout(object_list_index_layout);
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

TEST_F(S3CopyObjectActionTest, SendSuccessResponseAtEnd) {
  // Simulate success
  action_under_test->s3_put_action_state =
      S3PutObjectActionState::metadataSaved;
  action_under_test->new_object_metadata =
      ptr_mock_object_meta_factory->mock_object_metadata;

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(200, _)).Times(1);

  EXPECT_CALL(*ptr_mock_request, send_reply_body(_, _)).Times(0);
  EXPECT_CALL(*ptr_mock_request, send_reply_end()).Times(0);
  EXPECT_CALL(*ptr_mock_request, close_connection()).Times(0);

  action_under_test->send_response_to_s3_client();
}

TEST_F(S3CopyObjectActionTest, SendSuccessResponseSpread) {
  // Simulate success
  action_under_test->s3_put_action_state =
      S3PutObjectActionState::metadataSaved;
  action_under_test->new_object_metadata =
      ptr_mock_object_meta_factory->mock_object_metadata;
  action_under_test->response_started = true;

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(0);
  EXPECT_CALL(*ptr_mock_request, send_response(_, _)).Times(0);

  EXPECT_CALL(*ptr_mock_request, send_reply_body(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_reply_end()).Times(1);
  EXPECT_CALL(*ptr_mock_request, close_connection()).Times(1);

  action_under_test->send_response_to_s3_client();
}

TEST_F(S3CopyObjectActionTest, SendFailedResponseAtEnd) {
  // Simulate success
  action_under_test->s3_put_action_state =
      S3PutObjectActionState::validationFailed;
  action_under_test->set_s3_error("InternalError");

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(_, _)).Times(1);

  EXPECT_CALL(*ptr_mock_request, send_reply_body(_, _)).Times(0);
  EXPECT_CALL(*ptr_mock_request, send_reply_end()).Times(0);
  EXPECT_CALL(*ptr_mock_request, close_connection()).Times(0);

  action_under_test->send_response_to_s3_client();
}

TEST_F(S3CopyObjectActionTest, SendFailedResponseSpread) {
  action_under_test->s3_put_action_state = S3PutObjectActionState::writeFailed;
  action_under_test->set_s3_error("InternalError");
  action_under_test->response_started = true;

  EXPECT_CALL(*ptr_mock_request, send_response(_, _)).Times(0);

  EXPECT_CALL(*ptr_mock_request, send_reply_body(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_reply_end()).Times(1);
  EXPECT_CALL(*ptr_mock_request, close_connection()).Times(1);

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

  action_under_test->additional_object_metadata =
      action_under_test->object_metadata_factory->create_object_metadata_obj(
          ptr_mock_request);

  const std::map<std::string, std::string> tagsmap;
  EXPECT_CALL(*ptr_mock_object_meta_factory->mock_object_metadata, get_tags())
      .Times(1)
      .WillOnce(ReturnRef(tagsmap));
  EXPECT_CALL(*ptr_mock_request, get_header_value("x-amz-acl"))
      .Times(1)
      .WillOnce(Return(""));
  std::string MockJsonResponse("Mockresponse");
  EXPECT_CALL(*(ptr_mock_bucket_meta_factory->mock_bucket_metadata),
              get_state())
      .WillRepeatedly(Return(S3BucketMetadataState::present));
  EXPECT_CALL(*(ptr_mock_bucket_meta_factory->mock_bucket_metadata),
              get_policy_as_json()).WillRepeatedly(ReturnRef(MockJsonResponse));

  action_under_test->set_authorization_meta();

  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3CopyObjectActionTest, DestinationAuthorizationSourceTagsAndACLHeader) {
  create_dst_bucket_metadata();
  action_under_test->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(action_under_test,
                         S3CopyObjectActionTest::func_callback_one, this);
  action_under_test->bucket_metadata =
      action_under_test->bucket_metadata_factory->create_bucket_metadata_obj(
          ptr_mock_request);

  action_under_test->additional_object_metadata =
      action_under_test->object_metadata_factory->create_object_metadata_obj(
          ptr_mock_request);

  std::map<std::string, std::string> tagsmap{{"tag1", "value1"}};

  EXPECT_CALL(*ptr_mock_object_meta_factory->mock_object_metadata, get_tags())
      .Times(1)
      .WillOnce(ReturnRef(tagsmap));

  EXPECT_CALL(*ptr_mock_request, get_header_value("x-amz-acl"))
      .Times(1)
      .WillOnce(Return("test-acl"));

  EXPECT_CALL(*ptr_mock_request, set_action_list(_)).Times(2);

  std::string MockJsonResponse("Mockresponse");
  EXPECT_CALL(*(ptr_mock_bucket_meta_factory->mock_bucket_metadata),
              get_state())
      .WillRepeatedly(Return(S3BucketMetadataState::present));
  EXPECT_CALL(*(ptr_mock_bucket_meta_factory->mock_bucket_metadata),
              get_policy_as_json()).WillRepeatedly(ReturnRef(MockJsonResponse));

  action_under_test->set_authorization_meta();

  EXPECT_EQ(1, call_count_one);
}
