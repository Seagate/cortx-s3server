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

#include "mock_s3_motr_wrapper.h"
#include "mock_s3_factory.h"
#include "mock_s3_probable_delete_record.h"
#include "mock_s3_request_object.h"
#include "s3_abort_multipart_action.h"
#include "s3_ut_common.h"
#include "s3_m0_uint128_helper.h"
#include <cstdlib>

using ::testing::Return;
using ::testing::Invoke;
using ::testing::_;
using ::testing::ReturnRef;
using ::testing::AtLeast;
using ::testing::DefaultValue;
using ::testing::HasSubstr;

class S3AbortMultipartActionTest : public testing::Test {
 protected:
  S3AbortMultipartActionTest() {
    evhtp_request_t *req = NULL;
    EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();
    mp_indx_oid = {0xffff, 0xffff};
    oid = {0x1ffff, 0x1ffff};
    old_object_oid = {0x1ffff, 0x1ffff};
    new_layout_id = 9;
    old_layout_id = 2;
    object_list_indx_oid = {0x11ffff, 0x1ffff};
    objects_version_list_idx_oid = {0x11ffff, 0x1fff0};
    part_list_idx_oid = {0x11ffff, 0x1ff00};
    upload_id = "upload_id";
    bucket_name = "seagatebucket";
    object_name = "objname";
    call_count_one = 0;
    ptr_mock_request =
        std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr);
    EXPECT_CALL(*ptr_mock_request, get_object_name())
        .WillRepeatedly(ReturnRef(object_name));
    EXPECT_CALL(*ptr_mock_request, get_bucket_name())
        .WillRepeatedly(ReturnRef(bucket_name));

    ptr_mock_s3_motr_api = std::make_shared<MockS3Motr>();

    EXPECT_CALL(*ptr_mock_request, get_query_string_value("uploadId"))
        .WillRepeatedly(Return("upload_id"));

    EXPECT_CALL(*ptr_mock_s3_motr_api, m0_h_ufid_next(_))
        .WillOnce(Invoke(dummy_helpers_ufid_next));

    bucket_meta_factory = std::make_shared<MockS3BucketMetadataFactory>(
        ptr_mock_request, ptr_mock_s3_motr_api);
    object_mp_meta_factory =
        std::make_shared<MockS3ObjectMultipartMetadataFactory>(
            ptr_mock_request, ptr_mock_s3_motr_api, upload_id);
    object_mp_meta_factory->set_object_list_index_oid(mp_indx_oid);
    part_meta_factory = std::make_shared<MockS3PartMetadataFactory>(
        ptr_mock_request, oid, upload_id, 0);
    motr_writer_factory = std::make_shared<MockS3MotrWriterFactory>(
        ptr_mock_request, oid, ptr_mock_s3_motr_api);
    motr_kvs_reader_factory = std::make_shared<MockS3MotrKVSReaderFactory>(
        ptr_mock_request, ptr_mock_s3_motr_api);
    motr_kvs_writer_factory = std::make_shared<MockS3MotrKVSWriterFactory>(
        ptr_mock_request, ptr_mock_s3_motr_api);
    std::map<std::string, std::string> input_headers;
    input_headers["Authorization"] = "1";
    EXPECT_CALL(*ptr_mock_request, get_in_headers_copy()).Times(1).WillOnce(
        ReturnRef(input_headers));
    action_under_test.reset(new S3AbortMultipartAction(
        ptr_mock_request, ptr_mock_s3_motr_api, bucket_meta_factory,
        object_mp_meta_factory, part_meta_factory, motr_writer_factory,
        motr_kvs_reader_factory, motr_kvs_writer_factory));
  }

  std::shared_ptr<MockS3RequestObject> ptr_mock_request;
  std::shared_ptr<MockS3Motr> ptr_mock_s3_motr_api;
  std::shared_ptr<MockS3BucketMetadataFactory> bucket_meta_factory;
  std::shared_ptr<MockS3PartMetadataFactory> part_meta_factory;
  std::shared_ptr<MockS3ObjectMultipartMetadataFactory> object_mp_meta_factory;
  std::shared_ptr<MockS3MotrWriterFactory> motr_writer_factory;
  std::shared_ptr<MockS3MotrKVSReaderFactory> motr_kvs_reader_factory;
  std::shared_ptr<MockS3MotrKVSWriterFactory> motr_kvs_writer_factory;
  std::shared_ptr<S3AbortMultipartAction> action_under_test;
  struct m0_uint128 mp_indx_oid;
  struct m0_uint128 object_list_indx_oid, objects_version_list_idx_oid;
  struct m0_uint128 oid, old_object_oid, part_list_idx_oid;
  int new_layout_id, old_layout_id;
  std::string upload_id;
  std::string object_name;
  std::string bucket_name;
  int call_count_one;

 public:
  void func_callback_one() { call_count_one += 1; }
};

TEST_F(S3AbortMultipartActionTest, ConstructorTest) {
  EXPECT_NE(0, action_under_test->number_of_tasks());
  EXPECT_TRUE(action_under_test->s3_motr_api != NULL);
}

TEST_F(S3AbortMultipartActionTest, GetMultiPartMetadataTest1) {
  struct m0_uint128 oid = {0xffff, 0xffff};

  action_under_test->bucket_metadata =
      bucket_meta_factory->mock_bucket_metadata;
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillOnce(Return(S3BucketMetadataState::present));
  action_under_test->bucket_metadata->set_multipart_index_oid(oid);
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), load(_, _))
      .Times(1);
  action_under_test->get_multipart_metadata();
}

TEST_F(S3AbortMultipartActionTest, GetMultiPartMetadataTest2) {
  struct m0_uint128 empty_oid = {0ULL, 0ULL};

  action_under_test->bucket_metadata =
      bucket_meta_factory->mock_bucket_metadata;
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(S3BucketMetadataState::present));
  action_under_test->bucket_metadata->set_multipart_index_oid(empty_oid);
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), load(_, _))
      .Times(0);

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  /* No such upload error */
  EXPECT_CALL(*ptr_mock_request, send_response(404, _)).Times(1);

  action_under_test->get_multipart_metadata();
}

TEST_F(S3AbortMultipartActionTest, GetMultiPartMetadataTest3) {
  action_under_test->bucket_metadata =
      bucket_meta_factory->mock_bucket_metadata;
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(S3BucketMetadataState::missing));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(404, _)).Times(1);

  action_under_test->fetch_bucket_info_failed();
  EXPECT_EQ(action_under_test->s3_abort_mp_action_state,
            S3AbortMultipartActionState::validationFailed);
}

TEST_F(S3AbortMultipartActionTest, GetMultiPartMetadataTest5) {
  action_under_test->bucket_metadata =
      bucket_meta_factory->mock_bucket_metadata;
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(S3BucketMetadataState::failed));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(500, _)).Times(1);

  action_under_test->fetch_bucket_info_failed();
  EXPECT_EQ(action_under_test->s3_abort_mp_action_state,
            S3AbortMultipartActionState::validationFailed);
}

TEST_F(S3AbortMultipartActionTest, GetMultiPartMetadataTest6) {
  action_under_test->bucket_metadata =
      bucket_meta_factory->mock_bucket_metadata;
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(S3BucketMetadataState::failed_to_launch));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(503, _)).Times(1);

  action_under_test->fetch_bucket_info_failed();
  EXPECT_EQ(action_under_test->s3_abort_mp_action_state,
            S3AbortMultipartActionState::validationFailed);
}

TEST_F(S3AbortMultipartActionTest, DeleteMultipartMetadataTest1) {
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), remove(_, _))
      .Times(1);
  action_under_test->delete_multipart_metadata();
}

TEST_F(S3AbortMultipartActionTest, DeleteMultipartMetadataFailedTest) {
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(S3ObjectMetadataState::failed));
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(500, _)).Times(1);

  action_under_test->oid_str = S3M0Uint128Helper::to_string(old_object_oid);

  action_under_test->delete_multipart_metadata_failed();
  action_under_test->check_shutdown_signal_for_next_task(false);
  EXPECT_STREQ("InternalError", action_under_test->get_s3_error_code().c_str());
  EXPECT_EQ(action_under_test->s3_abort_mp_action_state,
            S3AbortMultipartActionState::uploadMetadataDeleteFailed);
}

TEST_F(S3AbortMultipartActionTest, DeleteMultipartMetadataFailedToLaunchTest) {
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(S3ObjectMetadataState::failed_to_launch));
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(503, _)).Times(1);

  action_under_test->oid_str = S3M0Uint128Helper::to_string(old_object_oid);
  action_under_test->delete_multipart_metadata_failed();
  action_under_test->check_shutdown_signal_for_next_task(false);
  EXPECT_STREQ("ServiceUnavailable",
               action_under_test->get_s3_error_code().c_str());
  EXPECT_EQ(action_under_test->s3_abort_mp_action_state,
            S3AbortMultipartActionState::uploadMetadataDeleteFailed);
}

TEST_F(S3AbortMultipartActionTest, DeletePartIndexWithPartsMissingIndexTest) {
  action_under_test->part_index_oid = {0ULL, 0ULL};

  // Mock out the next calls on action.
  action_under_test->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(action_under_test,
                         S3AbortMultipartActionTest::func_callback_one, this);

  action_under_test->delete_part_index_with_parts();
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3AbortMultipartActionTest, DeletePartIndexWithPartsTest) {
  action_under_test->part_index_oid = {0xffff, 0xffff};
  EXPECT_CALL(*(part_meta_factory->mock_part_metadata), remove_index(_, _))
      .Times(1);
  action_under_test->delete_part_index_with_parts();
}

TEST_F(S3AbortMultipartActionTest, DeletePartIndexWithPartsFailed) {
  // Mock out the next calls on action.
  action_under_test->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(action_under_test,
                         S3AbortMultipartActionTest::func_callback_one, this);

  action_under_test->delete_part_index_with_parts_failed();
  EXPECT_EQ(1, call_count_one);
  EXPECT_EQ(action_under_test->s3_abort_mp_action_state,
            S3AbortMultipartActionState::partsListIndexDeleteFailed);
}

TEST_F(S3AbortMultipartActionTest, DeletePartIndexWithPartsFailedToLaunch) {
  // Mock out the next calls on action.
  action_under_test->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(action_under_test,
                         S3AbortMultipartActionTest::func_callback_one, this);

  action_under_test->delete_part_index_with_parts_failed();
  EXPECT_EQ(1, call_count_one);
  EXPECT_EQ(action_under_test->s3_abort_mp_action_state,
            S3AbortMultipartActionState::partsListIndexDeleteFailed);
}

TEST_F(S3AbortMultipartActionTest, Send200SuccessToS3Client) {
  action_under_test->oid_str = S3M0Uint128Helper::to_string(old_object_oid);
  MockS3ProbableDeleteRecord *prob_rec = new MockS3ProbableDeleteRecord(
      "oid_str", {0ULL, 0ULL}, "abc_obj", oid, new_layout_id,
      object_list_indx_oid, objects_version_list_idx_oid,
      "" /* Version does not exists yet */, false /* force_delete */,
      true /* is_multipart */, part_list_idx_oid);
  action_under_test->probable_delete_rec.reset(prob_rec);
  action_under_test->motr_kv_writer =
      motr_kvs_writer_factory->mock_motr_kvs_writer;

  // expectations for mark_oid_for_deletion()
  EXPECT_CALL(*prob_rec, set_force_delete(true)).Times(1);
  EXPECT_CALL(*prob_rec, to_json()).Times(1);
  EXPECT_CALL(*(motr_kvs_writer_factory->mock_motr_kvs_writer),
              put_keyval(_, _, _, _, _)).Times(1);

  EXPECT_CALL(*ptr_mock_request, send_response(200, _)).Times(1);
  action_under_test->send_response_to_s3_client();
}

TEST_F(S3AbortMultipartActionTest, Send503InternalErrorToS3Client) {
  action_under_test->set_s3_error("InternalError");
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request,
              send_response(500, HasSubstr("<Code>InternalError</Code>")))
      .Times(1);
  action_under_test->send_response_to_s3_client();
}

