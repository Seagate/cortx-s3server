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

#include "s3_post_multipartobject_action.h"
#include <memory>
#include "mock_s3_factory.h"
#include "mock_s3_request_object.h"
#include "s3_common.h"
#include "s3_test_utils.h"
#include "s3_ut_common.h"

using ::testing::Return;
using ::testing::Invoke;
using ::testing::_;
using ::testing::ReturnRef;
using ::testing::AtLeast;
using ::testing::DefaultValue;
using ::testing::HasSubstr;

class S3PostMultipartObjectTest : public testing::Test {
 protected:
  S3PostMultipartObjectTest() {
    evhtp_request_t *req = NULL;
    EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();
    mp_indx_oid = {0xffff, 0xffff};
    oid = {0x1ffff, 0x1ffff};
    object_list_indx_oid = {0x11ffff, 0x1ffff};
    upload_id = "upload_id";
    call_count_one = 0;
    layout_id =
        S3MotrLayoutMap::get_instance()->get_best_layout_for_object_size();
    bucket_name = "seagatebucket";
    object_name = "objname";

    ptr_mock_request =
        std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr);
    EXPECT_CALL(*ptr_mock_request, get_bucket_name())
        .WillRepeatedly(ReturnRef(bucket_name));
    EXPECT_CALL(*ptr_mock_request, get_object_name())
        .WillRepeatedly(ReturnRef(object_name));

    ptr_mock_s3_motr_api = std::make_shared<MockS3Motr>();

    EXPECT_CALL(*ptr_mock_s3_motr_api, m0_h_ufid_next(_))
        .WillRepeatedly(Invoke(dummy_helpers_ufid_next));

    // Owned and deleted by shared_ptr in S3PostMultipartObjectAction
    bucket_meta_factory =
        std::make_shared<MockS3BucketMetadataFactory>(ptr_mock_request);
    object_mp_meta_factory =
        std::make_shared<MockS3ObjectMultipartMetadataFactory>(
            ptr_mock_request, ptr_mock_s3_motr_api, upload_id);
    object_mp_meta_factory->set_object_list_index_oid(mp_indx_oid);
    object_meta_factory = std::make_shared<MockS3ObjectMetadataFactory>(
        ptr_mock_request, ptr_mock_s3_motr_api);
    object_meta_factory->set_object_list_index_oid(object_list_indx_oid);

    part_meta_factory = std::make_shared<MockS3PartMetadataFactory>(
        ptr_mock_request, oid, upload_id, 0);
    bucket_tag_body_factory_mock = std::make_shared<MockS3PutTagBodyFactory>(
        MockObjectTagsStr, MockRequestId);
    motr_kvs_writer_factory = std::make_shared<MockS3MotrKVSWriterFactory>(
        ptr_mock_request, ptr_mock_s3_motr_api);

    EXPECT_CALL(*ptr_mock_request, get_header_value(_));
    std::map<std::string, std::string> input_headers;
    input_headers["Authorization"] = "1";
    EXPECT_CALL(*ptr_mock_request, get_in_headers_copy()).Times(1).WillOnce(
        ReturnRef(input_headers));

    action_under_test.reset(new S3PostMultipartObjectAction(
        ptr_mock_request, bucket_meta_factory, object_mp_meta_factory,
        object_meta_factory, part_meta_factory, bucket_tag_body_factory_mock,
        ptr_mock_s3_motr_api, motr_kvs_writer_factory));
  }

  std::shared_ptr<MockS3RequestObject> ptr_mock_request;
  std::shared_ptr<MockS3Motr> ptr_mock_s3_motr_api;
  std::shared_ptr<MockS3BucketMetadataFactory> bucket_meta_factory;
  std::shared_ptr<MockS3ObjectMetadataFactory> object_meta_factory;
  std::shared_ptr<MockS3PartMetadataFactory> part_meta_factory;
  std::shared_ptr<MockS3ObjectMultipartMetadataFactory> object_mp_meta_factory;
  std::shared_ptr<MockS3MotrKVSWriterFactory> motr_kvs_writer_factory;
  std::shared_ptr<MockS3PutTagBodyFactory> bucket_tag_body_factory_mock;
  std::shared_ptr<S3PostMultipartObjectAction> action_under_test;
  std::map<std::string, std::string> request_header_map;
  std::string MockObjectTagsStr;
  std::string MockRequestId;
  struct m0_uint128 mp_indx_oid;
  struct m0_uint128 object_list_indx_oid;
  struct m0_uint128 oid;
  std::string upload_id;
  int call_count_one;
  int layout_id;
  std::string bucket_name, object_name;

 public:
  void func_callback_one() { call_count_one += 1; }
};

TEST_F(S3PostMultipartObjectTest, ConstructorTest) {
  EXPECT_EQ(0, action_under_test->tried_count);
  EXPECT_NE(0, action_under_test->number_of_tasks());
}

TEST_F(S3PostMultipartObjectTest, ValidateRequestTags) {
  call_count_one = 0;
  request_header_map.clear();
  request_header_map["x-amz-tagging"] = "key1=value1&key2=value2";
  EXPECT_CALL(*ptr_mock_request, get_header_value(_))
      .WillOnce(Return("key1=value1&key2=value2"));
  action_under_test->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(action_under_test,
                         S3PostMultipartObjectTest::func_callback_one, this);

  action_under_test->validate_x_amz_tagging_if_present();

  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3PostMultipartObjectTest, VaidateEmptyTags) {
  request_header_map.clear();
  request_header_map["x-amz-tagging"] = "";
  EXPECT_CALL(*ptr_mock_request, get_header_value(_)).WillOnce(Return(""));
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(_, _)).Times(1);
  action_under_test->clear_tasks();

  action_under_test->validate_x_amz_tagging_if_present();
  EXPECT_STREQ("InvalidTagError",
               action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3PostMultipartObjectTest, RollbackPartMetadataIndex) {
  action_under_test->part_metadata = part_meta_factory->mock_part_metadata;
  EXPECT_CALL(*(part_meta_factory->mock_part_metadata), remove_index(_, _))
      .Times(AtLeast(1));
  action_under_test->rollback_create_part_meta_index();
}

TEST_F(S3PostMultipartObjectTest, RollbackPartMetadataIndexFailed) {
  action_under_test->part_metadata = part_meta_factory->mock_part_metadata;
  action_under_test->add_task_rollback(
      std::bind(&S3PostMultipartObjectTest::func_callback_one, this));

  action_under_test->rollback_create_part_meta_index_failed();
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3PostMultipartObjectTest, SaveUploadMetadataFailed) {
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .WillRepeatedly(Return(S3ObjectMetadataState::failed));
  action_under_test->add_task_rollback(
      std::bind(&S3PostMultipartObjectTest::func_callback_one, this));
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(500, _)).Times(1);

  action_under_test->save_upload_metadata_failed();
  EXPECT_EQ(1, call_count_one);
  EXPECT_STREQ("InternalError", action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3PostMultipartObjectTest, SaveUploadMetadataFailedToLaunch) {
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .WillRepeatedly(Return(S3ObjectMetadataState::failed_to_launch));
  action_under_test->add_task_rollback(
      std::bind(&S3PostMultipartObjectTest::func_callback_one, this));
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(503, _)).Times(1);

  action_under_test->save_upload_metadata_failed();
  EXPECT_EQ(1, call_count_one);
  EXPECT_STREQ("ServiceUnavailable",
               action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3PostMultipartObjectTest, CreatePartMetadataIndex) {
  EXPECT_CALL(*(part_meta_factory->mock_part_metadata), create_index(_, _))
      .Times(AtLeast(1));
  action_under_test->create_part_meta_index();
}

TEST_F(S3PostMultipartObjectTest, Send500ResponseToClient) {
  action_under_test->set_s3_error("InternalError");
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(500, _)).Times(AtLeast(1));
  action_under_test->send_response_to_s3_client();
}

TEST_F(S3PostMultipartObjectTest, Send404ResponseToClient) {
  action_under_test->set_s3_error("NoSuchBucket");

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(404, _)).Times(AtLeast(1));
  action_under_test->send_response_to_s3_client();
}

TEST_F(S3PostMultipartObjectTest, Send200ResponseToClient) {
  action_under_test->bucket_metadata =
      bucket_meta_factory->mock_bucket_metadata;
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;
  action_under_test->part_metadata = part_meta_factory->mock_part_metadata;
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .WillRepeatedly(Return(S3ObjectMetadataState::saved));
  EXPECT_CALL(*(part_meta_factory->mock_part_metadata), get_state())
      .WillRepeatedly(Return(S3PartMetadataState::store_created));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(200, _)).Times(AtLeast(1));
  action_under_test->send_response_to_s3_client();
}

