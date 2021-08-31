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
#include "s3_delete_object_action.h"
#include "s3_error_codes.h"
#include "s3_test_utils.h"
#include "s3_ut_common.h"
#include "s3_m0_uint128_helper.h"

using ::testing::Eq;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::Invoke;
using ::testing::_;
using ::testing::AtLeast;

#define CREATE_BUCKET_METADATA                                            \
  do {                                                                    \
    EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), load(_, _)) \
        .Times(AtLeast(1));                                               \
    action_under_test->fetch_bucket_info();                               \
  } while (0)

#define CREATE_OBJECT_METADATA                                            \
  do {                                                                    \
    CREATE_BUCKET_METADATA;                                               \
    EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata),             \
                get_object_list_index_layout())                           \
        .Times(AtLeast(1))                                                \
        .WillRepeatedly(ReturnRef(index_layout));                         \
    EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata),             \
                get_objects_version_list_index_layout())                  \
        .Times(AtLeast(1))                                                \
        .WillRepeatedly(ReturnRef(index_layout));                         \
    EXPECT_CALL(*(mock_request), http_verb())                             \
        .WillOnce(Return(S3HttpVerb::GET));                               \
    EXPECT_CALL(*(mock_request), get_operation_code())                    \
        .WillOnce(Return(S3OperationCode::tagging));                      \
    EXPECT_CALL(*(object_meta_factory->mock_object_metadata), load(_, _)) \
        .Times(AtLeast(1));                                               \
    action_under_test->fetch_object_info();                               \
  } while (0)

const struct s3_motr_idx_layout zero_index_layout = {};

class S3DeleteObjectActionTest : public testing::Test {
 protected:
  S3DeleteObjectActionTest() {
    S3Option::get_instance()->disable_auth();

    evhtp_request_t *req = NULL;
    EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();
    bucket_name = "seagatebucket";
    object_name = "objname";

    oid = {0x1ffff, 0x1ffff};
    index_layout = {{0x11ffff, 0x1ffff}};

    call_count_one = 0;

    layout_id =
        S3MotrLayoutMap::get_instance()->get_best_layout_for_object_size();

    async_buffer_factory =
        std::make_shared<MockS3AsyncBufferOptContainerFactory>(
            S3Option::get_instance()->get_libevent_pool_buffer_size());

    mock_request = std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr,
                                                         async_buffer_factory);
    EXPECT_CALL(*mock_request, get_bucket_name())
        .WillRepeatedly(ReturnRef(bucket_name));
    EXPECT_CALL(*mock_request, get_object_name())
        .WillRepeatedly(ReturnRef(object_name));

    ptr_mock_s3_motr_api = std::make_shared<MockS3Motr>();

    EXPECT_CALL(*ptr_mock_s3_motr_api, m0_h_ufid_next(_))
        .WillRepeatedly(Invoke(dummy_helpers_ufid_next));

    // Owned and deleted by shared_ptr in S3DeleteObjectAction
    bucket_meta_factory =
        std::make_shared<MockS3BucketMetadataFactory>(mock_request);

    EXPECT_CALL(*bucket_meta_factory->mock_bucket_metadata,
                get_object_list_index_layout())
        .WillRepeatedly(ReturnRef(zero_index_layout));

    EXPECT_CALL(*bucket_meta_factory->mock_bucket_metadata,
                get_objects_version_list_index_layout())
        .WillRepeatedly(ReturnRef(zero_index_layout));

    object_meta_factory = std::make_shared<MockS3ObjectMetadataFactory>(
        mock_request, ptr_mock_s3_motr_api);
    object_meta_factory->set_object_list_index_oid(index_layout.oid);

    motr_writer_factory = std::make_shared<MockS3MotrWriterFactory>(
        mock_request, oid, ptr_mock_s3_motr_api);

    motr_kvs_writer_factory = std::make_shared<MockS3MotrKVSWriterFactory>(
        mock_request, ptr_mock_s3_motr_api);

    std::map<std::string, std::string> input_headers;
    input_headers["Authorization"] = "1";
    EXPECT_CALL(*mock_request, get_in_headers_copy()).Times(1).WillOnce(
        ReturnRef(input_headers));

    action_under_test.reset(
        new S3DeleteObjectAction(mock_request, bucket_meta_factory,
                                 object_meta_factory, motr_writer_factory));
  }

  std::shared_ptr<MockS3RequestObject> mock_request;
  std::shared_ptr<MockS3Motr> ptr_mock_s3_motr_api;
  std::shared_ptr<MockS3BucketMetadataFactory> bucket_meta_factory;
  std::shared_ptr<MockS3ObjectMetadataFactory> object_meta_factory;
  std::shared_ptr<MockS3MotrWriterFactory> motr_writer_factory;
  std::shared_ptr<MockS3MotrKVSWriterFactory> motr_kvs_writer_factory;
  std::shared_ptr<MockS3AsyncBufferOptContainerFactory> async_buffer_factory;

  std::shared_ptr<S3DeleteObjectAction> action_under_test;

  struct s3_motr_idx_layout index_layout;
  struct m0_uint128 oid;

  int call_count_one;
  int layout_id;
  std::string bucket_name, object_name;

 public:
  void func_callback_one() { call_count_one += 1; }
};

TEST_F(S3DeleteObjectActionTest, ConstructorTest) {
  EXPECT_NE(0, action_under_test->number_of_tasks());
}

TEST_F(S3DeleteObjectActionTest, FetchBucketInfo) {
  CREATE_BUCKET_METADATA;
  EXPECT_TRUE(action_under_test->bucket_metadata != NULL);
}

TEST_F(S3DeleteObjectActionTest, FetchObjectInfoWhenBucketNotPresent) {
  CREATE_BUCKET_METADATA;

  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillRepeatedly(Return(S3BucketMetadataState::missing));

  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, send_response(_, _)).Times(1);

  action_under_test->fetch_bucket_info_failed();

  EXPECT_STREQ("NoSuchBucket", action_under_test->get_s3_error_code().c_str());
  EXPECT_TRUE(action_under_test->bucket_metadata != NULL);
  EXPECT_TRUE(action_under_test->object_metadata == NULL);
}

TEST_F(S3DeleteObjectActionTest, FetchObjectInfoWhenBucketFetchFailed) {
  CREATE_BUCKET_METADATA;

  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillRepeatedly(Return(S3BucketMetadataState::failed));

  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, send_response(_, _)).Times(1);

  action_under_test->fetch_bucket_info_failed();

  EXPECT_STREQ("InternalError", action_under_test->get_s3_error_code().c_str());
  EXPECT_TRUE(action_under_test->bucket_metadata != NULL);
  EXPECT_TRUE(action_under_test->object_metadata == NULL);
}

TEST_F(S3DeleteObjectActionTest, FetchObjectInfoWhenBucketFetchFailedToLaunch) {
  CREATE_BUCKET_METADATA;

  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillRepeatedly(Return(S3BucketMetadataState::failed_to_launch));

  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, send_response(_, _)).Times(1);

  action_under_test->fetch_bucket_info_failed();

  EXPECT_STREQ("ServiceUnavailable",
               action_under_test->get_s3_error_code().c_str());
  EXPECT_TRUE(action_under_test->bucket_metadata != NULL);
  EXPECT_TRUE(action_under_test->object_metadata == NULL);
}

TEST_F(S3DeleteObjectActionTest,
       FetchObjectInfoWhenBucketPresentAndObjIndexAbsent) {
  CREATE_BUCKET_METADATA;

  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillRepeatedly(Return(S3BucketMetadataState::present));

  EXPECT_CALL(*mock_request, send_response(S3HttpSuccess204, _)).Times(1);
  EXPECT_CALL(*(mock_request), http_verb()).WillOnce(Return(S3HttpVerb::GET));
  EXPECT_CALL(*(mock_request), get_operation_code())
      .WillOnce(Return(S3OperationCode::tagging));

  action_under_test->fetch_object_info();

  EXPECT_TRUE(action_under_test->bucket_metadata != NULL);
  EXPECT_TRUE(action_under_test->object_metadata == NULL);
}

TEST_F(S3DeleteObjectActionTest, DeleteMetadataWhenObjectPresent) {
  CREATE_OBJECT_METADATA;

  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_state())
      .WillRepeatedly(Return(S3ObjectMetadataState::present));

  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), remove(_, _))
      .Times(AtLeast(1));

  action_under_test->delete_metadata();
}

TEST_F(S3DeleteObjectActionTest, DeleteMetadataWhenObjectAbsent) {
  CREATE_OBJECT_METADATA;

  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_state())
      .WillRepeatedly(Return(S3ObjectMetadataState::missing));

  EXPECT_CALL(*mock_request, send_response(S3HttpSuccess204, _)).Times(1);

  action_under_test->fetch_object_info_failed();
}

TEST_F(S3DeleteObjectActionTest, DeleteMetadataWhenObjectMetadataFetchFailed) {
  CREATE_OBJECT_METADATA;

  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_state())
      .WillRepeatedly(Return(S3ObjectMetadataState::failed));

  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, send_response(S3HttpFailed500, _)).Times(1);

  action_under_test->fetch_object_info_failed();

  EXPECT_STREQ("InternalError", action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3DeleteObjectActionTest, DeleteObjectWhenMetadataDeleteFailedToLaunch) {
  CREATE_OBJECT_METADATA;

  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_state())
      .WillRepeatedly(Return(S3ObjectMetadataState::failed_to_launch));

  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, send_response(S3HttpFailed503, _)).Times(1);

  action_under_test->fetch_object_info_failed();

  EXPECT_STREQ("ServiceUnavailable",
               action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3DeleteObjectActionTest, SendErrorResponse) {
  action_under_test->set_s3_error("InternalError");
  action_under_test->s3_del_obj_action_state =
      S3DeleteObjectActionState::validationFailed;

  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, send_response(S3HttpFailed500, _))
      .Times(AtLeast(1));

  action_under_test->send_response_to_s3_client();
}

TEST_F(S3DeleteObjectActionTest, SendAnyFailedResponse) {
  action_under_test->set_s3_error("NoSuchBucket");
  action_under_test->s3_del_obj_action_state =
      S3DeleteObjectActionState::validationFailed;

  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, send_response(S3HttpFailed404, _))
      .Times(AtLeast(1));

  action_under_test->send_response_to_s3_client();
}

TEST_F(S3DeleteObjectActionTest, DeleteObject) {
  CREATE_OBJECT_METADATA;

  S3Option::get_instance()->set_s3server_obj_delayed_del_enabled(false);

  struct m0_uint128 obj_oid = {0x1ffff, 0x1ffff};
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_oid())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(obj_oid));
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_layout_id())
      .Times(AtLeast(1));
  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer),
              delete_object(_, _, _, _, _)).Times(1);

  action_under_test->delete_object();
}

TEST_F(S3DeleteObjectActionTest, DelayedDeleteObject) {
  CREATE_OBJECT_METADATA;

  S3Option::get_instance()->set_s3server_obj_delayed_del_enabled(true);

  struct m0_uint128 obj_oid = {0x1ffff, 0x1ffff};
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_oid())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(obj_oid));
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_layout_id())
      .Times(0);
  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer),
              delete_object(_, _, _, _, _)).Times(0);
  action_under_test->delete_object();
}

TEST_F(S3DeleteObjectActionTest, SendSuccessResponse) {
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_state())
      .WillRepeatedly(Return(S3ObjectMetadataState::missing));
  action_under_test->s3_del_obj_action_state =
      S3DeleteObjectActionState::validationFailed;

  EXPECT_CALL(*mock_request, send_response(S3HttpSuccess204, _))
      .Times(AtLeast(1));

  action_under_test->send_response_to_s3_client();
}

TEST_F(S3DeleteObjectActionTest, CleanupOnMetadataDeletion) {
  CREATE_OBJECT_METADATA;
  std::string version_key_in_index = "abcd/v1";
  int layout_id = 9;
  struct m0_uint128 oid = {0x1ffff, 0x1ffff};
  std::string oid_str = S3M0Uint128Helper::to_string(oid);
  action_under_test->probable_del_rec_list.push_back(std::move(
      std::unique_ptr<S3ProbableDeleteRecord>(new S3ProbableDeleteRecord(
          oid_str, {0ULL, 0ULL}, "abcd", oid, layout_id, "mock_pvid",
          index_layout.oid, index_layout.oid, version_key_in_index,
          false /* force_delete */))));
  action_under_test->motr_kv_writer =
      motr_kvs_writer_factory->mock_motr_kvs_writer;
  EXPECT_CALL(*(motr_kvs_writer_factory->mock_motr_kvs_writer),
              put_keyval(_, _, _, _)).Times(1);
  // EXPECT_CALL(*(motr_kvs_writer_factory->mock_motr_kvs_writer),
  //            delete_keyval(_, _, _, _)).Times(1);
  action_under_test->s3_del_obj_action_state =
      S3DeleteObjectActionState::metadataDeleted;
  action_under_test->object_metadata->obj_fragments = 2;
  action_under_test->startcleanup();
}

TEST_F(S3DeleteObjectActionTest, MarkOIDSForDeletion) {
  CREATE_OBJECT_METADATA;
  std::string version_key_in_index = "abcd/v1";
  int layout_id = 9;
  struct m0_uint128 oid = {0x1ffff, 0x1ffff};
  std::string oid_str = S3M0Uint128Helper::to_string(oid);
  action_under_test->probable_del_rec_list.push_back(std::move(
      std::unique_ptr<S3ProbableDeleteRecord>(new S3ProbableDeleteRecord(
          oid_str, {0ULL, 0ULL}, "abcd", oid, layout_id, "mock_pvid",
          index_layout.oid, index_layout.oid, version_key_in_index,
          false /* force_delete */))));
  action_under_test->motr_kv_writer =
      motr_kvs_writer_factory->mock_motr_kvs_writer;
  EXPECT_CALL(*(motr_kvs_writer_factory->mock_motr_kvs_writer),
              put_keyval(_, _, _, _)).Times(1);
  action_under_test->mark_oids_for_deletion();
}

TEST_F(S3DeleteObjectActionTest, DeleteObjectsDelayedDisabled) {
  CREATE_OBJECT_METADATA;

  S3Option::get_instance()->set_s3server_obj_delayed_del_enabled(false);
  struct S3ExtendedObjectInfo obj_info;
  action_under_test->extended_objects.push_back(obj_info);

  EXPECT_CALL(*(object_meta_factory->mock_object_metadata),
              remove_version_metadata(_, _)).Times(0);

  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer),
              delete_object(_, _, _, _, _)).Times(1);

  action_under_test->delete_objects();
}

TEST_F(S3DeleteObjectActionTest, DeleteObjectsDelayedEnabled) {
  CREATE_OBJECT_METADATA;

  S3Option::get_instance()->set_s3server_obj_delayed_del_enabled(false);
  struct S3ExtendedObjectInfo obj_info;
  action_under_test->extended_objects.push_back(obj_info);

  EXPECT_CALL(*(object_meta_factory->mock_object_metadata),
              remove_version_metadata(_, _)).Times(1);

  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer),
              delete_object(_, _, _, _, _)).Times(0);
  S3Option::get_instance()->set_s3server_obj_delayed_del_enabled(true);
  action_under_test->delete_objects();
}
