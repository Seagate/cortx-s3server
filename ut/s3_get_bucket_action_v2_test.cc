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
#include "s3_get_bucket_action_v2.h"

using ::testing::Invoke;
using ::testing::AtLeast;
using ::testing::ReturnRef;
using ::testing::Return;
using ::testing::_;

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

#define CREATE_ACTION_UNDER_TEST_OBJ                                     \
  do {                                                                   \
    EXPECT_CALL(*request_mock, get_query_string_value("encoding-type"))  \
        .Times(AtLeast(1))                                               \
        .WillRepeatedly(Return("url"));                                  \
    std::map<std::string, std::string> input_headers;                    \
    input_headers["Authorization"] = "1";                                \
    EXPECT_CALL(*request_mock, get_in_headers_copy()).Times(1).WillOnce( \
        ReturnRef(input_headers));                                       \
    action_under_test_ptr = std::make_shared<S3GetBucketActionV2>(       \
        request_mock, s3_motr_api_mock, motr_kvs_reader_factory,         \
        bucket_meta_factory, object_meta_factory, obfuscator);           \
  } while (0)

#define SET_NEXT_OBJ_SUCCESSFUL_EXPECTATIONS                                  \
  do {                                                                        \
    EXPECT_CALL(*(motr_kvs_reader_factory->mock_motr_kvs_reader),             \
                get_key_values())                                             \
        .WillRepeatedly(ReturnRef(result_keys_values));                       \
    EXPECT_CALL(*(object_meta_factory->mock_object_metadata), from_json(_))   \
        .WillRepeatedly(Return(0));                                           \
    EXPECT_CALL(*(object_meta_factory->mock_object_metadata),                 \
                get_content_length_str()).WillRepeatedly(Return("0"));        \
    EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())    \
        .WillRepeatedly(Return(S3BucketMetadataState::present));              \
    EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1)); \
    EXPECT_CALL(*request_mock, send_response(200, _)).Times(AtLeast(1));      \
  } while (0)

#define OBJ_METADATA_EXPECTATIONS                                              \
  do {                                                                         \
    EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_md5())       \
        .WillRepeatedly(Return(""));                                           \
    EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_user_name()) \
        .WillRepeatedly(Return("s3user"));                                     \
    EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_user_id())   \
        .WillRepeatedly(Return("1"));                                          \
    EXPECT_CALL(*(object_meta_factory->mock_object_metadata),                  \
                get_storage_class()).WillRepeatedly(Return("STANDARD"));       \
    EXPECT_CALL(*(object_meta_factory->mock_object_metadata),                  \
                get_last_modified_iso())                                       \
        .WillRepeatedly(Return("last_modified"));                              \
  } while (0)

class S3GetBucketActionV2Test : public testing::Test {
 protected:  // You should make the members protected s.t. they can be
             // accessed from sub-classes.
  S3GetBucketActionV2Test() {
    evhtp_request_t *req = NULL;
    EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();
    object_list_indx_oid = {0x11ffff, 0x1ffff};
    bucket_name = "seagatebucket";
    object_name = "objname";
    request_mock = std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr);
    EXPECT_CALL(*request_mock, get_bucket_name())
        .WillRepeatedly(ReturnRef(bucket_name));
    EXPECT_CALL(*request_mock, get_object_name())
        .WillRepeatedly(ReturnRef(object_name));

    s3_motr_api_mock = std::make_shared<MockS3Motr>();
    bucket_meta_factory = std::make_shared<MockS3BucketMetadataFactory>(
        request_mock, s3_motr_api_mock);
    motr_kvs_reader_factory = std::make_shared<MockS3MotrKVSReaderFactory>(
        request_mock, s3_motr_api_mock);
    object_meta_factory =
        std::make_shared<MockS3ObjectMetadataFactory>(request_mock);
    obfuscator = std::make_shared<S3CommonUtilities::S3XORObfuscator>();
    object_meta_factory->set_object_list_index_oid(object_list_indx_oid);
  }

  std::shared_ptr<MockS3RequestObject> request_mock;
  std::shared_ptr<S3GetBucketActionV2> action_under_test_ptr;
  std::shared_ptr<MockS3BucketMetadataFactory> bucket_meta_factory;
  std::shared_ptr<MockS3MotrKVSReaderFactory> motr_kvs_reader_factory;
  std::shared_ptr<MockS3ObjectMetadataFactory> object_meta_factory;
  std::shared_ptr<S3CommonUtilities::S3Obfuscator> obfuscator;
  std::shared_ptr<MockS3Motr> s3_motr_api_mock;

  struct m0_uint128 object_list_indx_oid;
  std::map<std::string, std::pair<int, std::string>> result_keys_values;
  std::string bucket_name, object_name;
};

TEST_F(S3GetBucketActionV2Test, Constructor) {
  CREATE_ACTION_UNDER_TEST_OBJ;
  EXPECT_NE(0, action_under_test_ptr->number_of_tasks());
  EXPECT_TRUE(action_under_test_ptr->obfuscator != nullptr);
  // Ensure 'S3GetBucketActionV2' instance is using instance of
  // 'S3ObjectListResponseV2'
  std::shared_ptr<S3ObjectListResponseV2> obj_v2_list =
      std::dynamic_pointer_cast<S3ObjectListResponseV2>(
          action_under_test_ptr->object_list);
  EXPECT_TRUE(obj_v2_list != nullptr);
}

TEST_F(S3GetBucketActionV2Test, V2ValidateRequest) {
  CREATE_ACTION_UNDER_TEST_OBJ;
  CREATE_BUCKET_METADATA_OBJ;

  EXPECT_CALL(*request_mock, get_query_string_value("prefix"))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(""));
  EXPECT_CALL(*request_mock, get_query_string_value("delimiter"))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(""));
  EXPECT_CALL(*request_mock, get_query_string_value("max-keys"))
      .Times(AtLeast(1))
      .WillRepeatedly(Return("10"));
  EXPECT_CALL(*request_mock, get_query_string_value("marker"))
      .Times(AtLeast(1))
      .WillRepeatedly(Return("Gello"));

  EXPECT_CALL(*request_mock, get_query_string_value("continuation-token"))
      .Times(AtLeast(1))
      .WillRepeatedly(Return(""));
  EXPECT_CALL(*request_mock, get_query_string_value("fetch-owner"))
      .Times(AtLeast(1))
      .WillRepeatedly(Return("true"));
  EXPECT_CALL(*request_mock, get_query_string_value("start-after"))
      .Times(AtLeast(1))
      .WillRepeatedly(Return("Hello/"));
  // last_key should be with value = "Hello/" i.e value of 'start-after' request
  // parameter
  // as 'continuation-token' is empty

  // Expect fetch-owner to be set to 'false' by default, before calling
  // validate_request()
  EXPECT_TRUE(action_under_test_ptr->request_fetch_owner == false);
  // Remove all tasks to avoid call to the action's task pipeline
  action_under_test_ptr->clear_tasks();
  action_under_test_ptr->validate_request();
  EXPECT_EQ(std::string("Hello/"), action_under_test_ptr->last_key);
  // Expect fetch-owner to be set to true
  EXPECT_TRUE(action_under_test_ptr->request_fetch_owner == true);
}

TEST_F(S3GetBucketActionV2Test, after_validate_request_cont_token_non_empty) {
  CREATE_ACTION_UNDER_TEST_OBJ;
  CREATE_BUCKET_METADATA_OBJ;
  // Remove all tasks to avoid call to the action's task pipeline
  action_under_test_ptr->clear_tasks();

  // Below is bae64 and obfuscated string of 'key1/key2'
  action_under_test_ptr->request_cont_token = "Y21xOSdjbXE6";
  action_under_test_ptr->after_validate_request();
  // Expect 'last_key' with value 'key1/key2'
  EXPECT_EQ(std::string("key1/key2"), action_under_test_ptr->last_key);
}

TEST_F(S3GetBucketActionV2Test, after_validate_request_cont_token_empty) {
  CREATE_ACTION_UNDER_TEST_OBJ;
  CREATE_BUCKET_METADATA_OBJ;
  // Remove all tasks to avoid call to the action's task pipeline
  action_under_test_ptr->clear_tasks();

  action_under_test_ptr->request_cont_token = "";
  action_under_test_ptr->request_start_after = "key1/key2";
  action_under_test_ptr->after_validate_request();
  // Expect 'last_key' with value 'key1/key2'
  EXPECT_EQ(std::string("key1/key2"), action_under_test_ptr->last_key);
}

TEST_F(S3GetBucketActionV2Test, SendResponseToClientServiceUnavailable) {
  CREATE_ACTION_UNDER_TEST_OBJ;
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

TEST_F(S3GetBucketActionV2Test, SendResponseToClientNoSuchBucket) {
  CREATE_ACTION_UNDER_TEST_OBJ;
  CREATE_BUCKET_METADATA_OBJ;

  action_under_test_ptr->set_s3_error("NoSuchBucket");

  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(404, _)).Times(AtLeast(1));
  action_under_test_ptr->send_response_to_s3_client();
}

TEST_F(S3GetBucketActionV2Test, SendResponseToClientSuccess) {
  CREATE_ACTION_UNDER_TEST_OBJ;
  CREATE_BUCKET_METADATA_OBJ;

  action_under_test_ptr->fetch_successful = true;
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(200, _)).Times(AtLeast(1));
  action_under_test_ptr->send_response_to_s3_client();
}

TEST_F(S3GetBucketActionV2Test, SendResponseToClientInternalError) {
  CREATE_ACTION_UNDER_TEST_OBJ;
  CREATE_BUCKET_METADATA_OBJ;

  action_under_test_ptr->fetch_successful = false;

  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillRepeatedly(Return(S3BucketMetadataState::present));
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(500, _)).Times(AtLeast(1));
  action_under_test_ptr->send_response_to_s3_client();
}
