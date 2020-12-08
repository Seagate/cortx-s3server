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
#include <gtest/gtest.h>

#include "mock_s3_factory.h"
#include "s3_copy_object_action.h"
#include "s3_probable_delete_record.h"
#include "s3_ut_common.h"
#include "s3_test_utils.h"

using ::testing::AtLeast;
using ::testing::Invoke;
using ::testing::ReturnRef;

#define CREATE_SOURCE_BUCKET_METADATA                                  \
  do {                                                                 \
    EXPECT_CALL(*(ptr_mock_bucket_meta_factory->mock_bucket_metadata), \
                load(_, _)).Times(AtLeast(1));                         \
    action_under_test->fetch_source_bucket_info();                     \
  } while (0)

#define CREATE_SOURCE_OBJECT_METADATA                                  \
  do {                                                                 \
    CREATE_SOURCE_BUCKET_METADATA;                                     \
    EXPECT_CALL(*(ptr_mock_bucket_meta_factory->mock_bucket_metadata), \
                get_object_list_index_oid())                           \
        .Times(1)                                                      \
        .WillOnce(Return(object_list_indx_oid));                       \
    EXPECT_CALL(*(ptr_mock_bucket_meta_factory->mock_bucket_metadata), \
                get_objects_version_list_index_oid())                  \
        .Times(1)                                                      \
        .WillOnce(Return(objects_version_list_idx_oid));               \
    EXPECT_CALL(*(ptr_mock_object_meta_factory->mock_object_metadata), \
                load(_, _)).Times(AtLeast(1));                         \
    action_under_test->fetch_source_object_info();                     \
  } while (0)

#define CREATE_DESTINATION_BUCKET_METADATA                             \
  do {                                                                 \
    EXPECT_CALL(*(ptr_mock_bucket_meta_factory->mock_bucket_metadata), \
                load(_, _)).Times(AtLeast(1));                         \
    action_under_test->fetch_bucket_info();                            \
  } while (0)

class S3CopyObjectActionTest : public testing::Test {
 protected:
  S3CopyObjectActionTest() {
    S3Option::get_instance()->disable_auth();
    evhtp_request_t *req = NULL;
    EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();

    oid = {0x1ffff, 0x1ffff};
    object_list_indx_oid = {0x11ffff, 0x1ffff};
    objects_version_list_idx_oid = {0x1ffff, 0x11fff};
    zero_oid_idx = {0ULL, 0ULL};

    destination_bucket_name = "detination-bucket";
    destination_object_name = "destination-object";
    layout_id =
        S3MotrLayoutMap::get_instance()->get_best_layout_for_object_size();
    call_count_one = 0;

    ptr_mock_s3_motr_api = std::make_shared<MockS3Motr>();
    EXPECT_CALL(*ptr_mock_s3_motr_api, m0_h_ufid_next(_))
        .WillRepeatedly(Invoke(dummy_helpers_ufid_next));

    ptr_mock_request =
        std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr);
    std::map<std::string, std::string> input_headers;
    input_headers["Authorization"] = "1";
    EXPECT_CALL(*ptr_mock_request, get_in_headers_copy()).Times(1).WillOnce(
        ReturnRef(input_headers));

    EXPECT_CALL(*ptr_mock_request, get_bucket_name())
        .WillRepeatedly(ReturnRef(destination_bucket_name));

    EXPECT_CALL(*ptr_mock_request, get_object_name())
        .Times(AtLeast(1))
        .WillOnce(ReturnRef(destination_object_name));

    ptr_mock_bucket_meta_factory =
        std::make_shared<MockS3BucketMetadataFactory>(ptr_mock_request);

    ptr_mock_object_meta_factory =
        std::make_shared<MockS3ObjectMetadataFactory>(ptr_mock_request,
                                                      ptr_mock_s3_motr_api);

    ptr_mock_motr_writer_factory = std::make_shared<MockS3MotrWriterFactory>(
        ptr_mock_request, oid, ptr_mock_s3_motr_api);

    ptr_mock_motr_reader_factory = std::make_shared<MockS3MotrReaderFactory>(
        ptr_mock_request, oid, layout_id);

    ptr_mock_motr_kvs_writer_factory =
        std::make_shared<MockS3MotrKVSWriterFactory>(ptr_mock_request,
                                                     ptr_mock_s3_motr_api);

    action_under_test.reset(new S3CopyObjectAction(
        ptr_mock_request, ptr_mock_s3_motr_api, ptr_mock_bucket_meta_factory,
        ptr_mock_object_meta_factory, ptr_mock_motr_writer_factory,
        ptr_mock_motr_reader_factory, ptr_mock_motr_kvs_writer_factory));
  }

  std::shared_ptr<MockS3RequestObject> ptr_mock_request;
  std::shared_ptr<MockS3Motr> ptr_mock_s3_motr_api;
  std::shared_ptr<MockS3BucketMetadataFactory> ptr_mock_bucket_meta_factory;
  std::shared_ptr<MockS3ObjectMetadataFactory> ptr_mock_object_meta_factory;
  std::shared_ptr<MockS3MotrReaderFactory> ptr_mock_motr_reader_factory;
  std::shared_ptr<MockS3MotrWriterFactory> ptr_mock_motr_writer_factory;
  std::shared_ptr<MockS3MotrKVSWriterFactory> ptr_mock_motr_kvs_writer_factory;

  std::shared_ptr<S3CopyObjectAction> action_under_test;

  int call_count_one;
  struct m0_uint128 oid;
  struct m0_uint128 objects_version_list_idx_oid;
  struct m0_uint128 object_list_indx_oid;
  struct m0_uint128 zero_oid_idx;
  int layout_id;
  std::string destination_bucket_name, destination_object_name;

 public:
  void func_callback_one() { call_count_one += 1; }
};

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
  CREATE_SOURCE_BUCKET_METADATA;
  EXPECT_TRUE(action_under_test->source_bucket_metadata != NULL);
}

TEST_F(S3CopyObjectActionTest, FetchSourceBucketInfoFailedMissing) {
  CREATE_SOURCE_BUCKET_METADATA;

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
  CREATE_SOURCE_BUCKET_METADATA;

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
  CREATE_SOURCE_BUCKET_METADATA;

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
  CREATE_SOURCE_OBJECT_METADATA;

  EXPECT_TRUE(action_under_test->source_object_metadata != NULL);
}

TEST_F(S3CopyObjectActionTest, FetchSourceObjectInfoListIndexNull) {
  CREATE_SOURCE_BUCKET_METADATA;

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
  CREATE_SOURCE_OBJECT_METADATA;
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
  CREATE_SOURCE_OBJECT_METADATA;

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
  CREATE_DESTINATION_BUCKET_METADATA;
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
  CREATE_DESTINATION_BUCKET_METADATA;
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
  CREATE_DESTINATION_BUCKET_METADATA;
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
/*
 * The tests need to be adjusted
 *
TEST_F(S3CopyObjectActionTest, FetchDestinationObjectInfoFailed) {
  if (!action_under_test->bucket_metadata) {
    action_under_test->bucket_metadata =
        ptr_mock_bucket_meta_factory->create_bucket_metadata_obj(
            ptr_mock_request, "bucket_name");
  }
  struct m0_uint128 m0_oid = {1, 1};

  EXPECT_CALL(*action_under_test->bucket_metadata, get_object_list_index_oid())
      .WillOnce(Return(m0_oid));

  call_count_one = 0;
  action_under_test->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(action_under_test,
                         S3CopyObjectActionTest::func_callback_one, this);

  action_under_test->fetch_object_info_failed();

  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3CopyObjectActionTest, FetchDestinationObjectInfoSuccess) {
  if (!action_under_test->object_metadata) {
    action_under_test->object_metadata =
        ptr_mock_object_meta_factory->create_object_metadata_obj(
            ptr_mock_request);
  }
  call_count_one = 0;
  action_under_test->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(action_under_test,
                         S3CopyObjectActionTest::func_callback_one, this);

  action_under_test->fetch_object_info_success();

  EXPECT_EQ(1, call_count_one);
}
*/
