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

#include <memory>
#include "mock_s3_motr_wrapper.h"
#include "mock_s3_factory.h"
#include "mock_s3_probable_delete_record.h"
#include "s3_put_chunk_upload_object_action.h"
#include "s3_test_utils.h"
#include "s3_ut_common.h"
#include "s3_m0_uint128_helper.h"

using ::testing::Eq;
using ::testing::Return;
using ::testing::Invoke;
using ::testing::_;
using ::testing::ReturnRef;
using ::testing::StrEq;
using ::testing::AtLeast;
using ::testing::DefaultValue;

#define CREATE_BUCKET_METADATA                                            \
  do {                                                                    \
    EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), load(_, _)) \
        .Times(AtLeast(1));                                               \
    action_under_test->fetch_bucket_info();                               \
  } while (0)

#define CREATE_OBJECT_METADATA                                                \
  do {                                                                        \
    CREATE_BUCKET_METADATA;                                                   \
    bucket_meta_factory->mock_bucket_metadata->set_object_list_index_oid(     \
        object_list_indx_oid);                                                \
    bucket_meta_factory->mock_bucket_metadata                                 \
        ->set_objects_version_list_index_oid(objects_version_list_index_oid); \
    EXPECT_CALL(*(object_meta_factory->mock_object_metadata), load(_, _))     \
        .Times(AtLeast(1));                                                   \
    EXPECT_CALL(*(mock_request), http_verb())                                 \
        .WillOnce(Return(S3HttpVerb::PUT));                                   \
    action_under_test->fetch_object_info();                                   \
  } while (0)

class S3PutChunkUploadObjectActionTestBase : public testing::Test {
 protected:
  S3PutChunkUploadObjectActionTestBase() {
    evhtp_request_t *req = NULL;
    EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();

    bucket_name = "seagatebucket";
    object_name = "objname";
    oid = {0x1ffff, 0x1ffff};
    object_list_indx_oid = {0x11ffff, 0x1ffff};
    objects_version_list_index_oid = {0x111fff, 0x1ffff};
    zero_oid_idx = {0ULL, 0ULL};

    layout_id =
        S3MotrLayoutMap::get_instance()->get_best_layout_for_object_size();

    call_count_one = 0;

    async_buffer_factory =
        std::make_shared<MockS3AsyncBufferOptContainerFactory>(
            S3Option::get_instance()->get_libevent_pool_buffer_size());

    mock_request = std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr,
                                                         async_buffer_factory);

    ptr_mock_s3_motr_api = std::make_shared<MockS3Motr>();

    EXPECT_CALL(*mock_request, get_bucket_name())
        .WillRepeatedly(ReturnRef(bucket_name));
    EXPECT_CALL(*mock_request, get_object_name())
        .WillRepeatedly(ReturnRef(object_name));
    EXPECT_CALL(*(mock_request), get_header_value(StrEq("x-amz-tagging")))
        .WillOnce(Return(""));
    EXPECT_CALL(*ptr_mock_s3_motr_api, m0_h_ufid_next(_))
        .WillRepeatedly(Invoke(dummy_helpers_ufid_next));

    // Owned and deleted by shared_ptr in S3PutChunkUploadObjectAction
    bucket_meta_factory = std::make_shared<MockS3BucketMetadataFactory>(
        mock_request, ptr_mock_s3_motr_api);

    object_meta_factory = std::make_shared<MockS3ObjectMetadataFactory>(
        mock_request, ptr_mock_s3_motr_api);
    object_meta_factory->set_object_list_index_oid(object_list_indx_oid);

    motr_writer_factory = std::make_shared<MockS3MotrWriterFactory>(
        mock_request, oid, ptr_mock_s3_motr_api);

    motr_kvs_writer_factory = std::make_shared<MockS3MotrKVSWriterFactory>(
        mock_request, ptr_mock_s3_motr_api);
  }

  std::shared_ptr<MockS3RequestObject> mock_request;
  std::shared_ptr<MockS3BucketMetadataFactory> bucket_meta_factory;
  std::shared_ptr<MockS3ObjectMetadataFactory> object_meta_factory;
  std::shared_ptr<MockS3MotrWriterFactory> motr_writer_factory;
  std::shared_ptr<MockS3AsyncBufferOptContainerFactory> async_buffer_factory;
  std::shared_ptr<MockS3Motr> ptr_mock_s3_motr_api;
  std::shared_ptr<MockS3MotrKVSWriterFactory> motr_kvs_writer_factory;

  std::shared_ptr<S3PutChunkUploadObjectAction> action_under_test;

  struct m0_uint128 object_list_indx_oid;
  struct m0_uint128 objects_version_list_index_oid;
  struct m0_uint128 oid;
  struct m0_uint128 zero_oid_idx;
  int layout_id;
  std::map<std::string, std::string> request_header_map;
  std::string bucket_name, object_name;
  int call_count_one;

 public:
  void func_callback_one() { call_count_one += 1; }
};

class S3PutChunkUploadObjectActionTestNoAuth
    : public S3PutChunkUploadObjectActionTestBase {
 protected:
  S3PutChunkUploadObjectActionTestNoAuth()
      : S3PutChunkUploadObjectActionTestBase() {
    S3Option::get_instance()->disable_auth();
    std::map<std::string, std::string> input_headers;
    input_headers["Authorization"] = "1";
    EXPECT_CALL(*mock_request, get_in_headers_copy()).Times(1).WillOnce(
        ReturnRef(input_headers));
    action_under_test.reset(new S3PutChunkUploadObjectAction(
        mock_request, bucket_meta_factory, object_meta_factory,
        motr_writer_factory, nullptr, ptr_mock_s3_motr_api, nullptr,
        motr_kvs_writer_factory));
  }
};

class S3PutChunkUploadObjectActionTestWithAuth
    : public S3PutChunkUploadObjectActionTestBase {
 protected:
  S3PutChunkUploadObjectActionTestWithAuth()
      : S3PutChunkUploadObjectActionTestBase() {
    S3Option::get_instance()->enable_auth();
    mock_auth_factory = std::make_shared<MockS3AuthClientFactory>(mock_request);
    std::map<std::string, std::string> input_headers;
    input_headers["Authorization"] = "1";
    EXPECT_CALL(*mock_request, get_in_headers_copy()).Times(1).WillOnce(
        ReturnRef(input_headers));
    action_under_test.reset(new S3PutChunkUploadObjectAction(
        mock_request, bucket_meta_factory, object_meta_factory,
        motr_writer_factory, mock_auth_factory, ptr_mock_s3_motr_api, nullptr,
        motr_kvs_writer_factory));
  }
  std::shared_ptr<MockS3AuthClientFactory> mock_auth_factory;
};

TEST_F(S3PutChunkUploadObjectActionTestNoAuth, ConstructorTest) {
  EXPECT_EQ(0, action_under_test->tried_count);
  EXPECT_FALSE(action_under_test->auth_failed);
  EXPECT_FALSE(action_under_test->write_failed);
  EXPECT_FALSE(action_under_test->motr_write_in_progress);
  EXPECT_FALSE(action_under_test->motr_write_completed);
  EXPECT_FALSE(action_under_test->auth_in_progress);
  EXPECT_TRUE(action_under_test->auth_completed);
  EXPECT_EQ("uri_salt_", action_under_test->salt);
  EXPECT_NE(0, action_under_test->number_of_tasks());
}

TEST_F(S3PutChunkUploadObjectActionTestNoAuth, ValidateRequestTags) {
  call_count_one = 0;
  request_header_map.clear();
  request_header_map["x-amz-tagging"] = "key1=value1&key2=value2";
  EXPECT_CALL(*mock_request, get_header_value(_))
      .WillOnce(Return("key1=value1&key2=value2"));
  action_under_test->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(
      action_under_test,
      S3PutChunkUploadObjectActionTestBase::func_callback_one, this);

  action_under_test->validate_x_amz_tagging_if_present();

  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3PutChunkUploadObjectActionTestNoAuth, VaidateEmptyTags) {
  request_header_map.clear();
  request_header_map["x-amz-tagging"] = "";
  EXPECT_CALL(*mock_request, get_header_value(_)).WillOnce(Return(""));
  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, send_response(_, _)).Times(1);
  EXPECT_CALL(*mock_request, resume(_)).Times(1);
  action_under_test->clear_tasks();

  action_under_test->validate_x_amz_tagging_if_present();
  EXPECT_STREQ("InvalidTagError",
               action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3PutChunkUploadObjectActionTestNoAuth, VaidateInvalidTagsCase1) {
  request_header_map.clear();
  request_header_map["x-amz-tagging"] = "key1=";
  EXPECT_CALL(*mock_request, get_header_value(_)).WillOnce(Return("key1="));
  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, send_response(_, _)).Times(1);
  EXPECT_CALL(*mock_request, resume(_)).Times(1);
  action_under_test->clear_tasks();

  action_under_test->validate_x_amz_tagging_if_present();
  EXPECT_STREQ("InvalidTagError",
               action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3PutChunkUploadObjectActionTestNoAuth, VaidateInvalidTagsCase2) {
  request_header_map.clear();
  request_header_map["x-amz-tagging"] = "key1=value1&=value2";
  EXPECT_CALL(*mock_request, get_header_value(_))
      .WillOnce(Return("key1=value1&=value2"));
  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, send_response(_, _)).Times(1);
  EXPECT_CALL(*mock_request, resume(_)).Times(1);
  action_under_test->clear_tasks();

  action_under_test->validate_x_amz_tagging_if_present();
  EXPECT_STREQ("InvalidTagError",
               action_under_test->get_s3_error_code().c_str());
}

// Count of tags exceding limit.

TEST_F(S3PutChunkUploadObjectActionTestNoAuth, VaidateInvalidTagsCase3) {
  request_header_map.clear();
  request_header_map["x-amz-tagging"] =
      "key1=value1&key2=value2&key3=value3&key4=value4&key5=value5&key6=value6&"
      "key7=value7&key8=value8&key9=value9&key10=value10&key11=value11";
  EXPECT_CALL(*mock_request, get_header_value(_)).WillOnce(Return(
      "key1=value1&key2=value2&key3=value3&key4=value4&key5=value5&key6=value6&"
      "key7=value7&key8=value8&key9=value9&key10=value10&key11=value11"));
  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, send_response(_, _)).Times(1);
  EXPECT_CALL(*mock_request, resume(_)).Times(1);
  action_under_test->clear_tasks();

  action_under_test->validate_x_amz_tagging_if_present();
  EXPECT_STREQ("InvalidTagError",
               action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3PutChunkUploadObjectActionTestNoAuth,
       FetchObjectInfoWhenBucketNotPresent) {
  CREATE_OBJECT_METADATA;
  action_under_test->object_metadata =
      object_meta_factory->mock_object_metadata;
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillOnce(Return(S3BucketMetadataState::missing));
  action_under_test->motr_kv_writer =
      motr_kvs_writer_factory->mock_motr_kvs_writer;
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;

  action_under_test->old_object_oid = {0xff1f, 0xffff};
  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, send_response(_, _)).Times(1);
  EXPECT_CALL(*mock_request, resume(_)).Times(1);
  action_under_test->auth_in_progress = false;
  action_under_test->fetch_bucket_info_failed();

  EXPECT_STREQ("NoSuchBucket", action_under_test->get_s3_error_code().c_str());
  EXPECT_TRUE(action_under_test->bucket_metadata != NULL);
  EXPECT_TRUE(action_under_test->object_metadata != NULL);
}

TEST_F(S3PutChunkUploadObjectActionTestNoAuth,
       FetchObjectInfoWhenBucketFailed) {
  CREATE_OBJECT_METADATA;

  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillRepeatedly(Return(S3BucketMetadataState::failed));
  action_under_test->old_object_oid = {0xff1f, 0xffff};
  action_under_test->motr_kv_writer =
      motr_kvs_writer_factory->mock_motr_kvs_writer;
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;

  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, send_response(_, _)).Times(1);
  EXPECT_CALL(*mock_request, resume(_)).Times(1);

  action_under_test->fetch_bucket_info_failed();

  EXPECT_STREQ("InternalError", action_under_test->get_s3_error_code().c_str());
  EXPECT_TRUE(action_under_test->bucket_metadata != NULL);
  EXPECT_TRUE(action_under_test->object_metadata != NULL);
}

TEST_F(S3PutChunkUploadObjectActionTestNoAuth,
       FetchObjectInfoWhenBucketFailedToLaunch) {
  CREATE_BUCKET_METADATA;

  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillRepeatedly(Return(S3BucketMetadataState::failed_to_launch));

  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, send_response(_, _)).Times(1);
  EXPECT_CALL(*mock_request, resume(_)).Times(1);

  action_under_test->fetch_bucket_info_failed();

  EXPECT_STREQ("ServiceUnavailable",
               action_under_test->get_s3_error_code().c_str());
  EXPECT_TRUE(action_under_test->bucket_metadata != NULL);
  EXPECT_TRUE(action_under_test->object_metadata == NULL);
}

TEST_F(S3PutChunkUploadObjectActionTestNoAuth,
       FetchObjectInfoReturnedFoundShouldHaveNewOID) {
  CREATE_OBJECT_METADATA;

  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_state())
      .WillOnce(Return(S3ObjectMetadataState::present));
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_layout_id())
      .Times(1)
      .WillOnce(Return(layout_id));

  struct m0_uint128 old_oid = {0x1ffff, 0x1ffff};
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_oid())
      .Times(1)
      .WillOnce(Return(old_oid));

  // Mock out the next calls on action.
  action_under_test->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(
      action_under_test,
      S3PutChunkUploadObjectActionTestBase::func_callback_one, this);

  // Remember default generated OID
  struct m0_uint128 oid_before_regen = action_under_test->new_object_oid;
  action_under_test->fetch_object_info_success();

  EXPECT_EQ(1, call_count_one);
  EXPECT_OID_NE(zero_oid_idx, action_under_test->old_object_oid);
  EXPECT_OID_NE(oid_before_regen, action_under_test->new_object_oid);
}

TEST_F(S3PutChunkUploadObjectActionTestNoAuth,
       FetchObjectInfoReturnedNotFoundShouldUseURL2OID) {
  CREATE_OBJECT_METADATA;

  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_state())
      .WillRepeatedly(Return(S3ObjectMetadataState::missing));

  // Mock out the next calls on action.
  action_under_test->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(
      action_under_test,
      S3PutChunkUploadObjectActionTestBase::func_callback_one, this);

  // Remember default generated OID
  struct m0_uint128 oid_before_regen = action_under_test->new_object_oid;
  action_under_test->fetch_object_info_success();

  EXPECT_EQ(1, call_count_one);
  EXPECT_OID_EQ(zero_oid_idx, action_under_test->old_object_oid);
  EXPECT_OID_EQ(oid_before_regen, action_under_test->new_object_oid);
}

TEST_F(S3PutChunkUploadObjectActionTestNoAuth,
       FetchObjectInfoReturnedInvalidStateReportsError) {
  CREATE_OBJECT_METADATA;

  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_state())
      .WillRepeatedly(Return(S3ObjectMetadataState::failed));

  // Mock out the next calls on action.
  action_under_test->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(
      action_under_test,
      S3PutChunkUploadObjectActionTestBase::func_callback_one, this);

  // Remember default generated OID
  struct m0_uint128 oid_before_regen = action_under_test->new_object_oid;

  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, send_response(_, _)).Times(1);
  EXPECT_CALL(*mock_request, resume(_)).Times(1);

  action_under_test->fetch_object_info_success();

  EXPECT_STREQ("InternalError", action_under_test->get_s3_error_code().c_str());
  EXPECT_EQ(0, call_count_one);
  EXPECT_OID_EQ(zero_oid_idx, action_under_test->old_object_oid);
  EXPECT_OID_EQ(oid_before_regen, action_under_test->new_object_oid);
}

TEST_F(S3PutChunkUploadObjectActionTestNoAuth, CreateObjectFirstAttempt) {
  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), create_object(_, _, _))
      .Times(1);
  EXPECT_CALL(*mock_request, get_data_length()).Times(1).WillOnce(Return(0));

  action_under_test->create_object();
  EXPECT_TRUE(action_under_test->motr_writer != nullptr);
}

TEST_F(S3PutChunkUploadObjectActionTestNoAuth, CreateObjectSecondAttempt) {
  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), create_object(_, _, _))
      .Times(2);
  EXPECT_CALL(*mock_request, get_data_length()).Times(2).WillRepeatedly(
      Return(0));

  action_under_test->create_object();
  action_under_test->tried_count = 1;
  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), set_oid(_)).Times(1);
  action_under_test->create_object();
  EXPECT_TRUE(action_under_test->motr_writer != nullptr);
}

TEST_F(S3PutChunkUploadObjectActionTestNoAuth,
       CreateObjectFailedTestWhileShutdown) {
  S3Option::get_instance()->set_is_s3_shutting_down(true);
  EXPECT_CALL(*mock_request, pause()).Times(1);
  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, send_response(503, _)).Times(1);
  EXPECT_CALL(*mock_request, resume(_)).Times(1);

  action_under_test->create_object_failed();
  EXPECT_TRUE(action_under_test->motr_writer == NULL);
  S3Option::get_instance()->set_is_s3_shutting_down(false);
}

TEST_F(S3PutChunkUploadObjectActionTestNoAuth,
       CreateObjectFailedWithCollisionExceededRetry) {
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;
  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), get_state())
      .Times(1)
      .WillOnce(Return(S3MotrWiterOpState::exists));

  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, send_response(500, _)).Times(1);
  EXPECT_CALL(*mock_request, resume(_)).Times(1);

  action_under_test->tried_count = MAX_COLLISION_RETRY_COUNT + 1;
  action_under_test->create_object_failed();

  EXPECT_STREQ("InternalError", action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3PutChunkUploadObjectActionTestNoAuth,
       CreateObjectFailedWithCollisionRetry) {
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;
  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), get_state())
      .Times(1)
      .WillOnce(Return(S3MotrWiterOpState::exists));
  EXPECT_CALL(*mock_request, get_data_length()).Times(1).WillOnce(Return(0));

  action_under_test->tried_count = MAX_COLLISION_RETRY_COUNT - 1;
  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), set_oid(_)).Times(1);
  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), create_object(_, _, _))
      .Times(1);

  action_under_test->create_object_failed();
}

TEST_F(S3PutChunkUploadObjectActionTestNoAuth, CreateObjectFailedTest) {
  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), create_object(_, _, _))
      .Times(1);
  EXPECT_CALL(*mock_request, get_data_length()).Times(1).WillOnce(Return(0));

  action_under_test->create_object();

  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), get_state())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(S3MotrWiterOpState::failed));

  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, send_response(_, _)).Times(1);
  EXPECT_CALL(*mock_request, resume(_)).Times(1);

  action_under_test->create_object_failed();
  EXPECT_STREQ("InternalError", action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3PutChunkUploadObjectActionTestNoAuth, CreateObjectFailedToLaunchTest) {
  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), create_object(_, _, _))
      .Times(1);
  EXPECT_CALL(*mock_request, get_data_length()).Times(1).WillOnce(Return(0));

  action_under_test->create_object();

  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), get_state())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(S3MotrWiterOpState::failed_to_launch));

  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, send_response(503, _)).Times(1);
  EXPECT_CALL(*mock_request, resume(_)).Times(1);

  action_under_test->create_object_failed();
  EXPECT_STREQ("ServiceUnavailable",
               action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3PutChunkUploadObjectActionTestNoAuth, CreateNewOidTest) {
  struct m0_uint128 old_oid = action_under_test->new_object_oid;

  action_under_test->create_new_oid(old_oid);

  EXPECT_OID_NE(old_oid, action_under_test->new_object_oid);
}

TEST_F(S3PutChunkUploadObjectActionTestNoAuth,
       InitiateDataStreamingForZeroSizeObject) {
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;
  EXPECT_CALL(*mock_request, get_data_length()).Times(1).WillOnce(Return(0));

  // Mock out the next calls on action.
  action_under_test->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(
      action_under_test,
      S3PutChunkUploadObjectActionTestBase::func_callback_one, this);

  action_under_test->initiate_data_streaming();
  EXPECT_FALSE(action_under_test->motr_write_in_progress);
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3PutChunkUploadObjectActionTestNoAuth,
       InitiateDataStreamingExpectingMoreData) {
  EXPECT_CALL(*mock_request, get_data_length()).Times(1).WillOnce(Return(1024));
  EXPECT_CALL(*mock_request, has_all_body_content()).Times(1).WillOnce(
      Return(false));
  EXPECT_CALL(*mock_request, listen_for_incoming_data(_, _)).Times(1);

  action_under_test->initiate_data_streaming();

  EXPECT_FALSE(action_under_test->motr_write_in_progress);
}

TEST_F(S3PutChunkUploadObjectActionTestNoAuth,
       InitiateDataStreamingWeHaveAllData) {
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;
  EXPECT_CALL(*mock_request, get_data_length())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(1024));
  EXPECT_CALL(*mock_request, has_all_body_content()).Times(1).WillOnce(
      Return(true));

  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), write_content(_, _, _))
      .Times(1);
  EXPECT_CALL(*mock_request, is_chunk_detail_ready()).WillOnce(Return(false));

  action_under_test->initiate_data_streaming();

  EXPECT_TRUE(action_under_test->motr_write_in_progress);
}

// Write not in progress and we have all the data
TEST_F(S3PutChunkUploadObjectActionTestNoAuth,
       ConsumeIncomingShouldWriteIfWeAllData) {
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;
  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), is_freezed())
      .WillRepeatedly(Return(true));
  // S3Option::get_instance()->get_motr_write_payload_size() = 1048576 * 1
  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), get_content_length())
      .WillRepeatedly(Return(1024));

  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), write_content(_, _, _))
      .Times(1);
  EXPECT_CALL(*mock_request, is_chunk_detail_ready()).WillOnce(Return(false));

  action_under_test->consume_incoming_content();

  EXPECT_TRUE(action_under_test->motr_write_in_progress);
}

// Write not in progress, expecting more, we have exact what we can write
TEST_F(S3PutChunkUploadObjectActionTestNoAuth,
       ConsumeIncomingShouldWriteIfWeExactData) {
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;

  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), is_freezed())
      .WillRepeatedly(Return(false));
  // S3Option::get_instance()->get_motr_write_payload_size() = 1048576 * 1
  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), get_content_length())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(
           S3Option::get_instance()->get_motr_write_payload_size(layout_id)));

  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), write_content(_, _, _))
      .Times(1);
  EXPECT_CALL(*mock_request, is_chunk_detail_ready()).WillOnce(Return(false));

  EXPECT_CALL(*mock_request, pause()).Times(1);

  action_under_test->consume_incoming_content();

  EXPECT_TRUE(action_under_test->motr_write_in_progress);
}

// Write not in progress, expecting more, we have more than we can write
TEST_F(S3PutChunkUploadObjectActionTestNoAuth,
       ConsumeIncomingShouldWriteIfWeHaveMoreData) {
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;

  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), is_freezed())
      .WillRepeatedly(Return(false));
  // S3Option::get_instance()->get_motr_write_payload_size() = 1048576 * 1
  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), get_content_length())
      .WillRepeatedly(Return(
           S3Option::get_instance()->get_motr_write_payload_size(layout_id) +
           1024));

  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), write_content(_, _, _))
      .Times(1);
  EXPECT_CALL(*mock_request, is_chunk_detail_ready()).WillOnce(Return(false));

  EXPECT_CALL(*mock_request, pause()).Times(1);

  action_under_test->consume_incoming_content();

  EXPECT_TRUE(action_under_test->motr_write_in_progress);
}

// we are expecting more data
TEST_F(S3PutChunkUploadObjectActionTestNoAuth,
       ConsumeIncomingShouldPauseWhenWeHaveTooMuch) {
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;
  action_under_test->_set_layout_id(layout_id);

  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), is_freezed())
      .WillRepeatedly(Return(false));
  // S3Option::get_instance()->get_motr_write_payload_size() = 1048576 * 1
  // S3_READ_AHEAD_MULTIPLE: 1
  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), get_content_length())
      .WillRepeatedly(Return(
           S3Option::get_instance()->get_motr_write_payload_size(layout_id) *
           S3Option::get_instance()->get_read_ahead_multiple() * 2));

  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), write_content(_, _, _))
      .Times(1);
  EXPECT_CALL(*mock_request, is_chunk_detail_ready()).WillOnce(Return(false));

  EXPECT_CALL(*mock_request, pause()).Times(1);
  action_under_test->consume_incoming_content();

  EXPECT_TRUE(action_under_test->motr_write_in_progress);
}

TEST_F(S3PutChunkUploadObjectActionTestNoAuth,
       ConsumeIncomingShouldNotWriteWhenWriteInprogress) {
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;
  action_under_test->motr_write_in_progress = true;
  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), is_freezed())
      .WillRepeatedly(Return(true));

  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), write_content(_, _, _))
      .Times(0);

  action_under_test->consume_incoming_content();
}

TEST_F(S3PutChunkUploadObjectActionTestNoAuth,
       WriteObjectShouldWriteContentAndMarkProgress) {
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;

  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), write_content(_, _, _))
      .Times(1);
  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), get_content_length())
      .WillRepeatedly(Return(1024));
  EXPECT_CALL(*mock_request, is_chunk_detail_ready()).WillOnce(Return(false));

  action_under_test->write_object(async_buffer_factory->get_mock_buffer());

  EXPECT_TRUE(action_under_test->motr_write_in_progress);
}

TEST_F(S3PutChunkUploadObjectActionTestNoAuth,
       WriteObjectFailedShouldUndoMarkProgress) {
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;

  // mock mark progress
  action_under_test->motr_write_in_progress = true;
  action_under_test->new_oid_str = S3M0Uint128Helper::to_string(oid);
  MockS3ProbableDeleteRecord *prob_rec = new MockS3ProbableDeleteRecord(
      action_under_test->new_oid_str, {0ULL, 0ULL}, "abc_obj", oid, layout_id,
      object_list_indx_oid, objects_version_list_index_oid,
      "" /* Version does not exists yet */, false /* force_delete */,
      false /* is_multipart */, {0ULL, 0ULL});
  action_under_test->new_probable_del_rec.reset(prob_rec);
  EXPECT_CALL(*prob_rec, set_force_delete(true)).Times(1);
  EXPECT_CALL(*prob_rec, to_json()).Times(1);
  EXPECT_CALL(*(motr_kvs_writer_factory->mock_motr_kvs_writer),
              put_keyval(_, _, _, _, _)).Times(1);

  EXPECT_CALL(*mock_request, pause()).Times(1);
  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), get_state())
      .Times(1)
      .WillOnce(Return(S3MotrWiterOpState::failed));
  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, send_response(500, _)).Times(1);
  EXPECT_CALL(*mock_request, resume(_)).Times(1);
  action_under_test->write_object_failed();

  EXPECT_STREQ("InternalError", action_under_test->get_s3_error_code().c_str());
  EXPECT_FALSE(action_under_test->motr_write_in_progress);
}

TEST_F(S3PutChunkUploadObjectActionTestNoAuth,
       WriteObjectEntityFailedShouldUndoMarkProgress) {
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;
  // mock mark progress
  action_under_test->motr_write_in_progress = true;
  action_under_test->new_oid_str = S3M0Uint128Helper::to_string(oid);
  MockS3ProbableDeleteRecord *prob_rec = new MockS3ProbableDeleteRecord(
      action_under_test->new_oid_str, {0ULL, 0ULL}, "abc_obj", oid, layout_id,
      object_list_indx_oid, objects_version_list_index_oid,
      "" /* Version does not exists yet */, false /* force_delete */,
      false /* is_multipart */, {0ULL, 0ULL});
  action_under_test->new_probable_del_rec.reset(prob_rec);
  EXPECT_CALL(*prob_rec, set_force_delete(true)).Times(1);
  EXPECT_CALL(*prob_rec, to_json()).Times(1);
  EXPECT_CALL(*(motr_kvs_writer_factory->mock_motr_kvs_writer),
              put_keyval(_, _, _, _, _)).Times(1);

  EXPECT_CALL(*mock_request, pause()).Times(1);
  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), get_state())
      .Times(1)
      .WillOnce(Return(S3MotrWiterOpState::failed_to_launch));
  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, send_response(503, _)).Times(1);
  EXPECT_CALL(*mock_request, resume(_)).Times(1);

  action_under_test->write_object_failed();

  EXPECT_FALSE(action_under_test->motr_write_in_progress);
  EXPECT_STREQ("ServiceUnavailable",
               action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3PutChunkUploadObjectActionTestNoAuth,
       WriteObjectSuccessfulWhileShuttingDown) {
  S3Option::get_instance()->set_is_s3_shutting_down(true);
  EXPECT_CALL(*mock_request, pause()).Times(1);
  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, send_response(503, _)).Times(1);
  EXPECT_CALL(*mock_request, resume(_)).Times(1);

  action_under_test->write_object_successful();

  S3Option::get_instance()->set_is_s3_shutting_down(false);

  EXPECT_FALSE(action_under_test->motr_write_in_progress);
}

TEST_F(S3PutChunkUploadObjectActionTestNoAuth,
       WriteObjectSuccessfulWhileShuttingDownAndRollback) {
  S3Option::get_instance()->set_is_s3_shutting_down(true);
  EXPECT_CALL(*mock_request, pause()).Times(1);

  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, send_response(503, _)).Times(1);
  EXPECT_CALL(*mock_request, resume(_)).Times(1);
  action_under_test->write_object_successful();

  S3Option::get_instance()->set_is_s3_shutting_down(false);

  EXPECT_FALSE(action_under_test->motr_write_in_progress);
}

// We have all the data: Freezed
TEST_F(S3PutChunkUploadObjectActionTestNoAuth,
       WriteObjectSuccessfulShouldWriteStateAllData) {
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;

  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), is_freezed())
      .WillRepeatedly(Return(true));
  // S3Option::get_instance()->get_motr_write_payload_size() = 1048576 * 1
  // S3_READ_AHEAD_MULTIPLE: 1
  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), get_content_length())
      .WillRepeatedly(Return(1024));
  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), write_content(_, _, _))
      .Times(1);
  EXPECT_CALL(*mock_request, is_chunk_detail_ready()).WillOnce(Return(false));

  action_under_test->write_object_successful();

  EXPECT_TRUE(action_under_test->motr_write_in_progress);
}

// We have some data but not all and exact to write
TEST_F(S3PutChunkUploadObjectActionTestNoAuth,
       WriteObjectSuccessfulShouldWriteWhenExactWritableSize) {
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;
  action_under_test->_set_layout_id(layout_id);

  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), is_freezed())
      .WillRepeatedly(Return(false));
  // S3Option::get_instance()->get_motr_write_payload_size() = 1048576 * 1
  // S3_READ_AHEAD_MULTIPLE: 1
  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), get_content_length())
      .WillRepeatedly(Return(
           S3Option::get_instance()->get_motr_write_payload_size(layout_id)));

  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), write_content(_, _, _))
      .Times(1);
  EXPECT_CALL(*mock_request, resume(_)).Times(1);
  EXPECT_CALL(*mock_request, is_chunk_detail_ready()).WillOnce(Return(false));

  action_under_test->write_object_successful();

  EXPECT_TRUE(action_under_test->motr_write_in_progress);
}

// We have some data but not all and but have more to write
TEST_F(S3PutChunkUploadObjectActionTestNoAuth,
       WriteObjectSuccessfulShouldWriteSomeDataWhenMoreData) {
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;
  action_under_test->_set_layout_id(layout_id);

  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), is_freezed())
      .WillRepeatedly(Return(false));
  // S3Option::get_instance()->get_motr_write_payload_size() = 1048576 * 1
  // S3_READ_AHEAD_MULTIPLE: 1
  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), get_content_length())
      .WillRepeatedly(Return(
           S3Option::get_instance()->get_motr_write_payload_size(layout_id) +
           1024));

  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), write_content(_, _, _))
      .Times(1);
  EXPECT_CALL(*mock_request, resume(_)).Times(1);
  EXPECT_CALL(*mock_request, is_chunk_detail_ready()).WillOnce(Return(false));

  action_under_test->write_object_successful();

  EXPECT_TRUE(action_under_test->motr_write_in_progress);
}

// We have some data but not all and but have more to write
TEST_F(S3PutChunkUploadObjectActionTestNoAuth,
       WriteObjectSuccessfulDoNextStepWhenAllIsWritten) {
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;
  action_under_test->_set_layout_id(layout_id);

  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), is_freezed())
      .WillRepeatedly(Return(true));
  // S3Option::get_instance()->get_motr_write_payload_size() = 1048576 * 1
  // S3_READ_AHEAD_MULTIPLE: 1
  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), get_content_length())
      .WillRepeatedly(Return(0));

  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), write_content(_, _, _))
      .Times(0);

  // Mock out the next calls on action.
  action_under_test->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(
      action_under_test,
      S3PutChunkUploadObjectActionTestBase::func_callback_one, this);

  action_under_test->write_object_successful();

  EXPECT_EQ(1, call_count_one);
  EXPECT_FALSE(action_under_test->motr_write_in_progress);
}

// We expecting more and not enough to write
TEST_F(S3PutChunkUploadObjectActionTestNoAuth,
       WriteObjectSuccessfulShouldRestartReadingData) {
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;
  action_under_test->_set_layout_id(layout_id);

  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), is_freezed())
      .WillRepeatedly(Return(false));
  // S3Option::get_instance()->get_motr_write_payload_size() = 1048576 * 1
  // S3_READ_AHEAD_MULTIPLE: 1
  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), get_content_length())
      .WillRepeatedly(Return(
           S3Option::get_instance()->get_motr_write_payload_size(layout_id) -
           1024));

  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), write_content(_, _, _))
      .Times(0);

  EXPECT_CALL(*mock_request, resume(_)).Times(1);

  action_under_test->write_object_successful();

  EXPECT_FALSE(action_under_test->motr_write_in_progress);
}
/*  TODO
TEST_F(S3PutChunkUploadObjectActionTestNoAuth, SaveMetadata) {
  CREATE_BUCKET_METADATA;
  bucket_meta_factory->mock_bucket_metadata->set_object_list_index_oid(
      object_list_indx_oid);

  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;

  EXPECT_CALL(*mock_request, get_data_length_str())
      .Times(1)
      .WillOnce(Return("1024"));
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata),
              set_content_length(Eq("1024")))
      .Times(AtLeast(1));
  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), get_content_md5())
      .Times(AtLeast(1))
      .WillOnce(Return("abcd1234abcd"));
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata),
              set_md5(Eq("abcd1234abcd")))
      .Times(AtLeast(1));
  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), get_oid())
      .Times(AtLeast(1))
      .WillOnce(Return(oid));
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), set_oid(_))
      .Times(AtLeast(1));

  std::map<std::string, std::string> input_headers;
  input_headers["x-amz-meta-item-1"] = "1024";
  input_headers["x-amz-meta-item-2"] = "s3.seagate.com";

  EXPECT_CALL(*mock_request, get_in_headers_copy())
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

  EXPECT_CALL(*(object_meta_factory->mock_object_metadata),
              reset_date_time_to_current()).Times(AtLeast(1));

  action_under_test->save_metadata();
}
*/
TEST_F(S3PutChunkUploadObjectActionTestNoAuth, SendResponseWhenShuttingDown) {
  S3Option::get_instance()->set_is_s3_shutting_down(true);

  EXPECT_CALL(*mock_request, pause()).Times(1);
  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, set_out_header_value(Eq("Retry-After"), Eq("1")))
      .Times(1);
  EXPECT_CALL(*mock_request, send_response(503, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, resume(_)).Times(1);

  // send_response_to_s3_client is called in check_shutdown_and_rollback
  action_under_test->check_shutdown_and_rollback();

  S3Option::get_instance()->set_is_s3_shutting_down(false);
}

TEST_F(S3PutChunkUploadObjectActionTestNoAuth, SendErrorResponse) {
  action_under_test->set_s3_error("InternalError");

  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, send_response(500, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, resume(false)).Times(1);

  action_under_test->send_response_to_s3_client();
}

TEST_F(S3PutChunkUploadObjectActionTestNoAuth, SendSuccessResponse) {
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;

  action_under_test->s3_put_chunk_action_state =
      S3PutChunkUploadObjectActionState::metadataSaved;

  // expectations for remove_new_oid_probable_record()
  action_under_test->new_oid_str = S3M0Uint128Helper::to_string(oid);
  EXPECT_CALL(*(motr_kvs_writer_factory->mock_motr_kvs_writer),
              delete_keyval(_, _, _, _)).Times(1);
  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), get_content_md5())
      .Times(AtLeast(1))
      .WillOnce(Return("abcd1234abcd"));
  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, send_response(200, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, resume(_)).Times(1);

  action_under_test->send_response_to_s3_client();
}

TEST_F(S3PutChunkUploadObjectActionTestNoAuth, SendFailedResponse) {
  action_under_test->set_s3_error("InternalError");
  action_under_test->s3_put_chunk_action_state =
      S3PutChunkUploadObjectActionState::validationFailed;

  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, send_response(500, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, resume(_)).Times(1);

  action_under_test->send_response_to_s3_client();
}

TEST_F(S3PutChunkUploadObjectActionTestWithAuth, ConstructorTest) {
  EXPECT_EQ(0, action_under_test->tried_count);
  EXPECT_FALSE(action_under_test->auth_failed);
  EXPECT_FALSE(action_under_test->write_failed);
  EXPECT_FALSE(action_under_test->motr_write_in_progress);
  EXPECT_FALSE(action_under_test->motr_write_completed);
  EXPECT_FALSE(action_under_test->auth_in_progress);
  EXPECT_FALSE(action_under_test->auth_completed);
  EXPECT_EQ("uri_salt_", action_under_test->salt);
  EXPECT_NE(0, action_under_test->number_of_tasks());
}
/*
TEST_F(S3PutChunkUploadObjectActionTestWithAuth,
       InitiateDataStreamingShouldInitChunkAuth) {
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;
  EXPECT_CALL(*mock_request, get_data_length()).Times(1).WillOnce(Return(0));
  EXPECT_CALL(*(mock_auth_factory->mock_auth_client),
              init_chunk_auth_cycle(_, _))
      .Times(1);

  // Mock out the next calls on action.
  action_under_test->clear_tasks();
ACTION_TASK_ADD_OBJPTR(action_under_test,
S3PutChunkUploadObjectActionTestBase::func_callback_one, this);

  action_under_test->initiate_data_streaming();
  EXPECT_FALSE(action_under_test->motr_write_in_progress);
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3PutChunkUploadObjectActionTestWithAuth,
       SendChunkDetailsToAuthClientWithSingleChunk) {
  S3ChunkDetail detail;
  detail.add_size(10);
  detail.add_signature(std::string("abcd1234abcd"));
  detail.update_hash("Test data", 9);
  detail.fini_hash();

  EXPECT_CALL(*mock_request, is_chunk_detail_ready())
      .Times(2)
      .WillOnce(Return(true))
      .WillOnce(Return(false));
  EXPECT_CALL(*mock_request, pop_chunk_detail())
      .Times(1)
      .WillOnce(Return(detail));

  EXPECT_CALL(*(mock_auth_factory->mock_auth_client),
              add_checksum_for_chunk(_, _))
      .Times(1);

  action_under_test->send_chunk_details_if_any();

  EXPECT_TRUE(action_under_test->auth_in_progress);
}

TEST_F(S3PutChunkUploadObjectActionTestWithAuth,
       SendChunkDetailsToAuthClientWithTwoChunks) {
  S3ChunkDetail detail;
  detail.add_size(10);
  detail.add_signature(std::string("abcd1234abcd"));
  detail.update_hash("Test data", 9);
  detail.fini_hash();

  EXPECT_CALL(*mock_request, is_chunk_detail_ready())
      .Times(3)
      .WillOnce(Return(true))
      .WillOnce(Return(true))
      .WillOnce(Return(false));
  EXPECT_CALL(*mock_request, pop_chunk_detail())
      .Times(2)
      .WillRepeatedly(Return(detail));

  EXPECT_CALL(*(mock_auth_factory->mock_auth_client),
              add_checksum_for_chunk(_, _))
      .Times(2);

  action_under_test->send_chunk_details_if_any();

  EXPECT_TRUE(action_under_test->auth_in_progress);
}

TEST_F(S3PutChunkUploadObjectActionTestWithAuth,
       SendChunkDetailsToAuthClientWithTwoChunksAndOneZeroChunk) {
  S3ChunkDetail detail, detail_zero;
  detail.add_size(10);
  detail.add_signature(std::string("abcd1234abcd"));
  detail.update_hash("Test data", 9);
  detail.fini_hash();
  detail_zero.add_size(0);
  detail_zero.add_signature(std::string("abcd1234abcd"));
  detail_zero.update_hash("", 0);
  detail_zero.fini_hash();

  EXPECT_CALL(*mock_request, is_chunk_detail_ready())
      .Times(3)
      .WillOnce(Return(true))
      .WillOnce(Return(true))
      .WillOnce(Return(false));
  EXPECT_CALL(*mock_request, pop_chunk_detail())
      .Times(2)
      .WillOnce(Return(detail))
      .WillOnce(Return(detail_zero));

  EXPECT_CALL(*(mock_auth_factory->mock_auth_client),
              add_checksum_for_chunk(_, _))
      .Times(1);
  EXPECT_CALL(*(mock_auth_factory->mock_auth_client),
              add_last_checksum_for_chunk(_, _))
      .Times(1);

  action_under_test->send_chunk_details_if_any();

  EXPECT_TRUE(action_under_test->auth_in_progress);
}

TEST_F(S3PutChunkUploadObjectActionTestWithAuth,
       WriteObjectShouldSendChunkDetailsForAuth) {
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;
  S3ChunkDetail detail;
  detail.add_size(10);
  detail.add_signature(std::string("abcd1234abcd"));
  detail.update_hash("Test data", 9);
  detail.fini_hash();

  EXPECT_CALL(*mock_request, is_chunk_detail_ready())
      .Times(2)
      .WillOnce(Return(true))
      .WillOnce(Return(false));
  EXPECT_CALL(*mock_request, pop_chunk_detail())
      .Times(1)
      .WillOnce(Return(detail));

  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer),
              write_content(_, _, _))
      .Times(1);

  EXPECT_CALL(*(mock_auth_factory->mock_auth_client),
              add_checksum_for_chunk(_, _))
      .Times(1);

  action_under_test->write_object(async_buffer_factory->get_mock_buffer());

  EXPECT_TRUE(action_under_test->auth_in_progress);
}

TEST_F(S3PutChunkUploadObjectActionTestWithAuth,
       WriteObjectSuccessfulShouldSendChunkDetailsForAuth) {
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;

  // mock mark progress
  action_under_test->motr_write_in_progress = true;

  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), is_freezed())
      .WillRepeatedly(Return(true));
  // S3Option::get_instance()->get_motr_write_payload_size() = 1048576 * 1
  // S3_READ_AHEAD_MULTIPLE: 1
  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), get_content_length())
      .WillRepeatedly(Return(0));

  S3ChunkDetail detail;
  detail.add_size(10);
  detail.add_signature(std::string("abcd1234abcd"));
  detail.update_hash("Test data", 9);
  detail.fini_hash();

  EXPECT_CALL(*mock_request, is_chunk_detail_ready())
      .Times(2)
      .WillOnce(Return(true))
      .WillOnce(Return(false));
  EXPECT_CALL(*mock_request, pop_chunk_detail())
      .Times(1)
      .WillOnce(Return(detail));

  EXPECT_CALL(*(mock_auth_factory->mock_auth_client),
              add_checksum_for_chunk(_, _))
      .Times(1);

  action_under_test->write_object_successful();

  EXPECT_TRUE(action_under_test->auth_in_progress);
}
*/
TEST_F(S3PutChunkUploadObjectActionTestWithAuth,
       ChunkAuthSuccessfulWhileShuttingDown) {
  S3Option::get_instance()->set_is_s3_shutting_down(true);
  EXPECT_CALL(*mock_request, pause()).Times(1);
  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, send_response(503, _)).Times(1);
  EXPECT_CALL(*mock_request, resume(_)).Times(1);

  action_under_test->chunk_auth_successful();

  EXPECT_FALSE(action_under_test->auth_in_progress);
  EXPECT_TRUE(action_under_test->auth_completed);
  S3Option::get_instance()->set_is_s3_shutting_down(false);
}

TEST_F(S3PutChunkUploadObjectActionTestWithAuth,
       ChunkAuthFailedWhileShuttingDown) {
  S3Option::get_instance()->set_is_s3_shutting_down(true);
  EXPECT_CALL(*mock_request, pause()).Times(1);
  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, send_response(403, _)).Times(1);
  EXPECT_CALL(*mock_request, resume(_)).Times(1);

  action_under_test->chunk_auth_failed();

  EXPECT_FALSE(action_under_test->auth_in_progress);
  EXPECT_TRUE(action_under_test->auth_failed);
  S3Option::get_instance()->set_is_s3_shutting_down(false);
}

TEST_F(S3PutChunkUploadObjectActionTestWithAuth,
       ChunkAuthSuccessfulWriteInProgress) {
  action_under_test->motr_write_completed = false;

  action_under_test->chunk_auth_successful();

  EXPECT_TRUE(action_under_test->auth_completed);
}

TEST_F(S3PutChunkUploadObjectActionTestWithAuth,
       ChunkAuthSuccessfulWriteFailed) {
  action_under_test->motr_write_completed = true;
  action_under_test->write_failed = true;
  S3Option::get_instance()->set_is_s3_shutting_down(true);
  action_under_test->s3_put_chunk_action_state =
      S3PutChunkUploadObjectActionState::writeFailed;
  action_under_test->set_s3_error("InternalError");
  EXPECT_CALL(*mock_request, pause()).Times(1);
  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, send_response(500, _)).Times(1);
  EXPECT_CALL(*mock_request, resume(_)).Times(1);
  action_under_test->chunk_auth_successful();

  EXPECT_TRUE(action_under_test->auth_completed);
  S3Option::get_instance()->set_is_s3_shutting_down(false);
}

TEST_F(S3PutChunkUploadObjectActionTestWithAuth,
       ChunkAuthSuccessfulWriteSuccessful) {
  action_under_test->motr_write_completed = true;
  action_under_test->write_failed = false;
  action_under_test->s3_put_chunk_action_state =
      S3PutChunkUploadObjectActionState::metadataSaved;
  action_under_test->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(
      action_under_test,
      S3PutChunkUploadObjectActionTestBase::func_callback_one, this);

  action_under_test->chunk_auth_successful();
  EXPECT_EQ(1, call_count_one);

  EXPECT_TRUE(action_under_test->auth_completed);
}

TEST_F(S3PutChunkUploadObjectActionTestWithAuth, ChunkAuthFailedWriteFailed) {
  action_under_test->motr_write_in_progress = false;
  action_under_test->write_failed = true;
  S3Option::get_instance()->set_is_s3_shutting_down(true);
  EXPECT_CALL(*mock_request, pause()).Times(1);
  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, send_response(403, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, resume(_)).Times(1);

  action_under_test->chunk_auth_failed();
  S3Option::get_instance()->set_is_s3_shutting_down(false);
  EXPECT_TRUE(action_under_test->auth_completed);
}

TEST_F(S3PutChunkUploadObjectActionTestWithAuth,
       ChunkAuthFailedWriteSuccessful) {
  action_under_test->motr_write_in_progress = false;
  action_under_test->write_failed = false;
  S3Option::get_instance()->set_is_s3_shutting_down(true);
  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, send_response(_, _)).Times(1);
  EXPECT_CALL(*mock_request, resume(_)).Times(1);

  action_under_test->chunk_auth_failed();

  EXPECT_STREQ("SignatureDoesNotMatch",
               action_under_test->get_s3_error_code().c_str());
  EXPECT_TRUE(action_under_test->auth_completed);
  S3Option::get_instance()->set_is_s3_shutting_down(false);
}
