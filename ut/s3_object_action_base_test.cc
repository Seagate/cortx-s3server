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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "mock_s3_bucket_metadata.h"
#include "mock_s3_object_metadata.h"
#include "mock_s3_motr_wrapper.h"
#include "mock_s3_factory.h"
#include "mock_s3_request_object.h"
#include "s3_object_action_base.h"

using ::testing::AtLeast;
using ::testing::ReturnRef;

#define CREATE_BUCKET_METADATA                          \
  do {                                                  \
    action_under_test_ptr->bucket_metadata =            \
        action_under_test_ptr->bucket_metadata_factory  \
            ->create_bucket_metadata_obj(request_mock); \
  } while (0)

#define CREATE_OBJECT_METADATA                                                 \
  do {                                                                         \
    action_under_test_ptr->object_metadata =                                   \
        object_meta_factory->create_object_metadata_obj(request_mock,          \
                                                        object_list_indx_oid); \
  } while (0)

class S3ObjectActionTestBase : public S3ObjectAction {
 public:
  S3ObjectActionTestBase(
      std::shared_ptr<S3RequestObject> req,
      std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory,
      std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory,
      bool check_shutdown, std::shared_ptr<S3AuthClientFactory> auth_factory,
      bool skip_auth);

  void fetch_bucket_info_failed() { fetch_bucket_info_failed_called = 1; }
  void fetch_object_info_failed() { fetch_object_info_failed_called = 1; }

  void send_response_to_s3_client() {
    response_called += 1;
  }

  int fetch_bucket_info_failed_called = 0;
  int fetch_object_info_failed_called = 0;
  int response_called = 0;
};

S3ObjectActionTestBase::S3ObjectActionTestBase(
    std::shared_ptr<S3RequestObject> req,
    std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory,
    std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory,
    bool check_shutdown, std::shared_ptr<S3AuthClientFactory> auth_factory,
    bool skip_auth)
    : S3ObjectAction(std::move(req), std::move(bucket_meta_factory),
                     std::move(object_meta_factory), check_shutdown,
                     std::move(auth_factory), skip_auth) {}

class S3ObjectActionTest : public testing::Test {
 protected:  // You should make the members protected s.t. they can be
             // accessed from sub-classes.
  S3ObjectActionTest();
  void SetUp() override;

  struct m0_uint128 object_list_indx_oid = {0x11ffff, 0x1ffff};
  struct m0_uint128 objects_version_list_index_oid = {0x11ffff, 0x1ffff};
  struct m0_uint128 zero_oid_idx = {0ULL, 0ULL};

  std::shared_ptr<MockS3RequestObject> request_mock;
  std::shared_ptr<MockS3BucketMetadataFactory> bucket_meta_factory;
  std::shared_ptr<MockS3ObjectMetadataFactory> object_meta_factory;
  std::shared_ptr<MockS3AuthClientFactory> mock_auth_factory;
  // std::shared_ptr<MockS3MotrKVSReaderFactory> motr_kvs_reader_factory;
  // std::shared_ptr<MotrAPI> s3_motr_api_mock;

  std::unique_ptr<S3ObjectActionTestBase> action_under_test_ptr;

  int call_count_one = 0;
  std::string bucket_name, object_name;

 public:
  void func_callback_one() { call_count_one += 1; }
};

S3ObjectActionTest::S3ObjectActionTest()
    : bucket_name("seagatebucket"), object_name("objname") {

  request_mock =
      std::make_shared<MockS3RequestObject>(nullptr, new EvhtpWrapper);

  EXPECT_CALL(*request_mock, get_bucket_name())
      .WillRepeatedly(ReturnRef(bucket_name));
  EXPECT_CALL(*request_mock, get_object_name())
      .WillRepeatedly(ReturnRef(object_name));

  mock_auth_factory = std::make_shared<MockS3AuthClientFactory>(request_mock);
  bucket_meta_factory =
      std::make_shared<MockS3BucketMetadataFactory>(request_mock);
  object_meta_factory =
      std::make_shared<MockS3ObjectMetadataFactory>(request_mock);
  object_meta_factory->set_object_list_index_oid(object_list_indx_oid);
}

void S3ObjectActionTest::SetUp() {
  std::map<std::string, std::string> input_headers;
  input_headers["Authorization"] = "1";

  EXPECT_CALL(*request_mock, get_in_headers_copy()).Times(1).WillOnce(
      ReturnRef(input_headers));

  action_under_test_ptr.reset(new S3ObjectActionTestBase(
      request_mock, bucket_meta_factory, object_meta_factory, true,
      mock_auth_factory, false));
}

TEST_F(S3ObjectActionTest, Constructor) {
  EXPECT_NE(0, action_under_test_ptr->number_of_tasks());
  EXPECT_TRUE(action_under_test_ptr->bucket_metadata_factory != nullptr);
  EXPECT_TRUE(action_under_test_ptr->object_metadata_factory != nullptr);
}

TEST_F(S3ObjectActionTest, FetchBucketInfo) {
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), load(_, _))
      .Times(AtLeast(1));
  action_under_test_ptr->fetch_bucket_info();
}

TEST_F(S3ObjectActionTest, LoadMetadata) {
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), load(_, _))
      .Times(AtLeast(1));
  EXPECT_TRUE(action_under_test_ptr->bucket_metadata_factory != nullptr);

  action_under_test_ptr->load_metadata();
}

TEST_F(S3ObjectActionTest, FetchBucketInfoSuccess) {
  S3AuditInfo s3_audit_info;

  action_under_test_ptr->load_metadata();
  action_under_test_ptr->clear_tasks();

  EXPECT_CALL(*request_mock, http_verb()).WillOnce(Return(S3HttpVerb::GET));
  EXPECT_CALL(*(request_mock), get_operation_code())
      .WillOnce(Return(S3OperationCode::tagging));
  EXPECT_CALL(*request_mock, get_audit_info())
      .WillOnce(ReturnRef(s3_audit_info));

  EXPECT_CALL(*dynamic_cast<MockS3BucketMetadata*>(
                   action_under_test_ptr->bucket_metadata.get()),
              get_owner_canonical_id()).Times(1);

  action_under_test_ptr->fetch_bucket_info_success();
}

TEST_F(S3ObjectActionTest, FetchObjectInfoFailed) {
  CREATE_BUCKET_METADATA;

  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillRepeatedly(Return(S3BucketMetadataState::present));
  bucket_meta_factory->mock_bucket_metadata->set_object_list_index_oid(
      zero_oid_idx);

  // EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  // EXPECT_CALL(*request_mock, send_response(_, _)).Times(1);
  EXPECT_CALL(*(request_mock), http_verb()).WillOnce(Return(S3HttpVerb::GET));
  EXPECT_CALL(*(request_mock), get_operation_code())
      .WillOnce(Return(S3OperationCode::tagging));
  action_under_test_ptr->fetch_object_info();
  EXPECT_EQ(1, action_under_test_ptr->fetch_object_info_failed_called);
}

TEST_F(S3ObjectActionTest, FetchObjectInfoSuccess) {
  CREATE_BUCKET_METADATA;
  CREATE_OBJECT_METADATA;
  action_under_test_ptr->bucket_metadata->set_object_list_index_oid(
      object_list_indx_oid);
  action_under_test_ptr->bucket_metadata->set_objects_version_list_index_oid(
      objects_version_list_index_oid);
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), load(_, _))
      .Times(AtLeast(1));
  EXPECT_CALL(*(request_mock), http_verb()).WillOnce(Return(S3HttpVerb::GET));
  EXPECT_CALL(*(request_mock), get_operation_code())
      .WillOnce(Return(S3OperationCode::tagging));

  action_under_test_ptr->fetch_object_info();

  EXPECT_TRUE(action_under_test_ptr->bucket_metadata != NULL);
  EXPECT_TRUE(action_under_test_ptr->object_metadata != NULL);
}

TEST_F(S3ObjectActionTest, SetAuthorizationMeta) {
  CREATE_OBJECT_METADATA;
  action_under_test_ptr->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(action_under_test_ptr,
                         S3ObjectActionTest::func_callback_one, this);
  action_under_test_ptr->bucket_metadata =
      action_under_test_ptr->bucket_metadata_factory
          ->create_bucket_metadata_obj(request_mock);

  std::string MockJsonResponse("Mockresponse");
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillRepeatedly(Return(S3BucketMetadataState::present));
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata),
              get_policy_as_json()).WillRepeatedly(ReturnRef(MockJsonResponse));
  action_under_test_ptr->set_authorization_meta();
  EXPECT_EQ(1, call_count_one);
}

