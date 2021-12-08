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
#include "mock_s3_motr_wrapper.h"
#include "mock_s3_factory.h"
#include "mock_s3_request_object.h"
#include "s3_list_object_versions_action.h"

using ::testing::AtLeast;
using ::testing::ReturnRef;
using ::testing::Return;

#define CREATE_BUCKET_METADATA_OBJ                      \
  do {                                                  \
    action_under_test_ptr->bucket_metadata =            \
        action_under_test_ptr->bucket_metadata_factory  \
            ->create_bucket_metadata_obj(request_mock); \
  } while (0)

#define CREATE_KVS_READER_OBJ                                         \
  do {                                                                \
    action_under_test_ptr->motr_kv_reader =                           \
        action_under_test_ptr->s3_motr_kvs_reader_factory             \
            ->create_motr_kvs_reader(request_mock, s3_motr_api_mock); \
  } while (0)

const struct s3_motr_idx_layout zero_index_layout = {};

class S3ListObjectVersionsTest : public testing::Test {
 protected:  // The members should be protected so that
             // they can be accessed from sub-classes.
  S3ListObjectVersionsTest() {
    evhtp_request_t *req = NULL;
    EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();
    index_layout = {{0x11ffff, 0x1ffff}};
    bucket_name = "seagatebucket";
    object_name = "none";

    request_mock = std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr);
    EXPECT_CALL(*request_mock, get_bucket_name())
        .WillRepeatedly(ReturnRef(bucket_name));
    EXPECT_CALL(*request_mock, get_object_name())
        .WillRepeatedly(ReturnRef(object_name));

    s3_motr_api_mock = std::make_shared<MockS3Motr>();
    bucket_meta_factory =
        std::make_shared<MockS3BucketMetadataFactory>(request_mock);
    EXPECT_CALL(*bucket_meta_factory->mock_bucket_metadata,
                get_object_list_index_layout())
        .WillRepeatedly(ReturnRef(index_layout));
    EXPECT_CALL(*bucket_meta_factory->mock_bucket_metadata,
                get_objects_version_list_index_layout())
        .WillRepeatedly(ReturnRef(index_layout));

    motr_kvs_reader_factory = std::make_shared<MockS3MotrKVSReaderFactory>(
        request_mock, s3_motr_api_mock);
    object_meta_factory =
        std::make_shared<MockS3ObjectMetadataFactory>(request_mock);
    object_meta_factory->set_object_list_index_oid(index_layout.oid);

    input_headers["Authorization"] = "1";
    EXPECT_CALL(*request_mock, get_in_headers_copy()).Times(1).WillOnce(
        ReturnRef(input_headers));
  }
  void SetUp() override;

  std::shared_ptr<MockS3RequestObject> request_mock;
  std::unique_ptr<S3ListObjectVersionsAction> action_under_test_ptr;
  std::shared_ptr<MockS3BucketMetadataFactory> bucket_meta_factory;
  std::shared_ptr<MockS3MotrKVSReaderFactory> motr_kvs_reader_factory;
  std::shared_ptr<MockS3ObjectMetadataFactory> object_meta_factory;
  std::shared_ptr<MockS3Motr> s3_motr_api_mock;

  struct s3_motr_idx_layout index_layout;
  std::map<std::string, std::pair<int, std::string>> return_keys_values;
  std::map<std::string, std::string> input_headers;
  std::string bucket_name, object_name;
  int call_count_one;

  void func_callback_one() { call_count_one += 1; }
};

void S3ListObjectVersionsTest::SetUp() {
  EXPECT_CALL(*request_mock, get_query_string_value("encoding-type"))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(""));

  action_under_test_ptr.reset(new S3ListObjectVersionsAction(
      request_mock, s3_motr_api_mock, motr_kvs_reader_factory,
      bucket_meta_factory, object_meta_factory));
}

TEST_F(S3ListObjectVersionsTest, Constructor) {
  EXPECT_NE(0, action_under_test_ptr->number_of_tasks());
  EXPECT_TRUE(action_under_test_ptr->bucket_metadata_factory != nullptr);
  EXPECT_TRUE(action_under_test_ptr->s3_motr_kvs_reader_factory != nullptr);
  EXPECT_TRUE(action_under_test_ptr->object_metadata_factory != nullptr);
  EXPECT_FALSE(action_under_test_ptr->fetch_successful);
  EXPECT_FALSE(action_under_test_ptr->response_is_truncated);
  EXPECT_EQ(0, action_under_test_ptr->key_Count);
  EXPECT_EQ("", action_under_test_ptr->last_key);
  EXPECT_EQ("", action_under_test_ptr->last_object_checked);
  EXPECT_EQ(0, action_under_test_ptr->versions_list.size());
}

TEST_F(S3ListObjectVersionsTest, FetchBucketInfo) {
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), load(_, _))
      .Times(AtLeast(1));
  action_under_test_ptr->fetch_bucket_info();
}

TEST_F(S3ListObjectVersionsTest, FetchBucketInfoFailedMissing) {
  CREATE_BUCKET_METADATA_OBJ;
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(S3BucketMetadataState::missing));
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(404, _)).Times(AtLeast(1));
  action_under_test_ptr->fetch_bucket_info_failed();
  EXPECT_STREQ("NoSuchBucket",
               action_under_test_ptr->get_s3_error_code().c_str());
}

TEST_F(S3ListObjectVersionsTest, FetchBucketInfoFailedToLaunch) {
  CREATE_BUCKET_METADATA_OBJ;
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(S3BucketMetadataState::failed_to_launch));
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(503, _)).Times(AtLeast(1));
  action_under_test_ptr->fetch_bucket_info_failed();
  EXPECT_STREQ("ServiceUnavailable",
               action_under_test_ptr->get_s3_error_code().c_str());
}

TEST_F(S3ListObjectVersionsTest, FetchBucketInfoFailedInternalError) {
  CREATE_BUCKET_METADATA_OBJ;
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(S3BucketMetadataState::failed));
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(500, _)).Times(AtLeast(1));
  action_under_test_ptr->fetch_bucket_info_failed();
  EXPECT_STREQ("InternalError",
               action_under_test_ptr->get_s3_error_code().c_str());
}

TEST_F(S3ListObjectVersionsTest, ValidateRequestInvalidArgument) {
  EXPECT_CALL(*request_mock, get_query_string_value("prefix"))
      .Times(1)
      .WillOnce(Return(""));
  EXPECT_CALL(*request_mock, get_query_string_value("delimiter"))
      .Times(1)
      .WillOnce(Return(""));
  EXPECT_CALL(*request_mock, get_query_string_value("key-marker"))
      .Times(1)
      .WillOnce(Return(""));
  EXPECT_CALL(*request_mock, get_query_string_value("max-keys"))
      .Times(1)
      .WillOnce(Return("ABC"));
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(400, _)).Times(AtLeast(1));
  action_under_test_ptr->validate_request();
}

TEST_F(S3ListObjectVersionsTest, GetNextVersionsZeroMaxkeys) {
  CREATE_BUCKET_METADATA_OBJ;
  CREATE_KVS_READER_OBJ;

  action_under_test_ptr->bucket_metadata->set_object_list_index_layout(
      index_layout);
  // Set requested keys (max_keys) to zero.
  action_under_test_ptr->max_keys = 0;
  EXPECT_CALL(*(motr_kvs_reader_factory->mock_motr_kvs_reader), get_state())
      .WillRepeatedly(Return(S3MotrKVSReaderOpState::missing));
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(200, _)).Times(AtLeast(1));
  action_under_test_ptr->get_next_versions();
  EXPECT_TRUE(action_under_test_ptr->fetch_successful);
}

TEST_F(S3ListObjectVersionsTest, GetNextVersionsZeroOid) {
  CREATE_BUCKET_METADATA_OBJ;
  CREATE_KVS_READER_OBJ;

  EXPECT_CALL(*bucket_meta_factory->mock_bucket_metadata,
              get_object_list_index_layout())
      .WillRepeatedly(ReturnRef(zero_index_layout));
  // Return zero Oid for objects version list.
  EXPECT_CALL(*bucket_meta_factory->mock_bucket_metadata,
              get_objects_version_list_index_layout())
      .WillRepeatedly(ReturnRef(zero_index_layout));

  action_under_test_ptr->max_keys = 10;
  EXPECT_CALL(*(motr_kvs_reader_factory->mock_motr_kvs_reader), get_state())
      .WillRepeatedly(Return(S3MotrKVSReaderOpState::missing));
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(200, _)).Times(AtLeast(1));
  action_under_test_ptr->get_next_versions();
  EXPECT_TRUE(action_under_test_ptr->fetch_successful);
}

TEST_F(S3ListObjectVersionsTest, GetNextVersions) {
  CREATE_BUCKET_METADATA_OBJ;
  CREATE_KVS_READER_OBJ;

  action_under_test_ptr->max_keys = 10;
  EXPECT_CALL(*(motr_kvs_reader_factory->mock_motr_kvs_reader), get_state())
      .WillRepeatedly(Return(S3MotrKVSReaderOpState::empty));
  EXPECT_CALL(*(motr_kvs_reader_factory->mock_motr_kvs_reader),
              next_keyval(_, _, _, _, _, _)).Times(1);
  action_under_test_ptr->get_next_versions();
}

TEST_F(S3ListObjectVersionsTest, GetNextVersionsFailed) {
  CREATE_BUCKET_METADATA_OBJ;
  CREATE_KVS_READER_OBJ;

  // Return motr KVS reader state as failed.
  EXPECT_CALL(*(motr_kvs_reader_factory->mock_motr_kvs_reader), get_state())
      .WillRepeatedly(Return(S3MotrKVSReaderOpState::failed));
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillRepeatedly(Return(S3BucketMetadataState::present));
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(500, _)).Times(AtLeast(1));

  action_under_test_ptr->get_next_versions_failed();
  EXPECT_STREQ("InternalError",
               action_under_test_ptr->get_s3_error_code().c_str());
}

TEST_F(S3ListObjectVersionsTest, GetNextVersionsSuccessful) {
  CREATE_BUCKET_METADATA_OBJ;
  CREATE_KVS_READER_OBJ;

  return_keys_values.insert(
      std::make_pair("testkey0", std::make_pair(10, "keyval")));
  return_keys_values.insert(
      std::make_pair("testkey1", std::make_pair(10, "keyval")));
  return_keys_values.insert(
      std::make_pair("testkey2", std::make_pair(10, "keyval")));

  action_under_test_ptr->request_prefix.assign("");
  action_under_test_ptr->request_delimiter.assign("");
  action_under_test_ptr->max_keys = 3;

  EXPECT_CALL(*(motr_kvs_reader_factory->mock_motr_kvs_reader),
              get_key_values()).WillRepeatedly(ReturnRef(return_keys_values));
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), from_json(_))
      .WillRepeatedly(Return(0));

  action_under_test_ptr->max_record_count =
      S3Option::get_instance()->get_motr_idx_fetch_count();
  action_under_test_ptr->clear_tasks();
  action_under_test_ptr->get_next_versions_successful();
  EXPECT_EQ(3, action_under_test_ptr->versions_list.size());
}

TEST_F(S3ListObjectVersionsTest, GetNextVersionsSuccessfulJsonError) {
  CREATE_BUCKET_METADATA_OBJ;
  CREATE_KVS_READER_OBJ;

  return_keys_values.insert(
      std::make_pair("testkey0", std::make_pair(10, "keyval")));
  return_keys_values.insert(
      std::make_pair("testkey1", std::make_pair(10, "keyval")));
  return_keys_values.insert(
      std::make_pair("testkey2", std::make_pair(10, "keyval")));

  action_under_test_ptr->request_prefix.assign("");
  action_under_test_ptr->request_delimiter.assign("");
  action_under_test_ptr->max_keys = 3;

  EXPECT_CALL(*(motr_kvs_reader_factory->mock_motr_kvs_reader),
              get_key_values()).WillRepeatedly(ReturnRef(return_keys_values));
  // Return json error.
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), from_json(_))
      .WillRepeatedly(Return(-1));

  action_under_test_ptr->max_record_count =
      S3Option::get_instance()->get_motr_idx_fetch_count();
  action_under_test_ptr->clear_tasks();
  action_under_test_ptr->get_next_versions_successful();
  // Expect no versions in the version_list.
  EXPECT_EQ(0, action_under_test_ptr->versions_list.size());
}

TEST_F(S3ListObjectVersionsTest, SendResponseToClientServiceUnavailable) {
  CREATE_BUCKET_METADATA_OBJ;

  S3Option::get_instance()->set_is_s3_shutting_down(true);
  EXPECT_CALL(*request_mock, pause()).Times(1);
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(503, _)).Times(AtLeast(1));

  action_under_test_ptr->check_shutdown_and_rollback();
  EXPECT_STREQ("ServiceUnavailable",
               action_under_test_ptr->get_s3_error_code().c_str());
  S3Option::get_instance()->set_is_s3_shutting_down(false);
}

TEST_F(S3ListObjectVersionsTest, SendResponseToClientNoSuchBucket) {
  CREATE_BUCKET_METADATA_OBJ;

  action_under_test_ptr->set_s3_error("NoSuchBucket");

  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(404, _)).Times(AtLeast(1));
  action_under_test_ptr->send_response_to_s3_client();
}

TEST_F(S3ListObjectVersionsTest, SendResponseToClientSuccess) {
  CREATE_BUCKET_METADATA_OBJ;

  action_under_test_ptr->fetch_successful = true;
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(200, _)).Times(AtLeast(1));
  action_under_test_ptr->send_response_to_s3_client();
}

TEST_F(S3ListObjectVersionsTest, SendResponseToClientInternalError) {
  CREATE_BUCKET_METADATA_OBJ;

  action_under_test_ptr->fetch_successful = false;
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillRepeatedly(Return(S3BucketMetadataState::present));
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(500, _)).Times(AtLeast(1));
  action_under_test_ptr->send_response_to_s3_client();
}