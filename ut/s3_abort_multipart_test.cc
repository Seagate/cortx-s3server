/*
 * COPYRIGHT 2017 SEAGATE LLC
 *
 * THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
 * HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE TECHNOLOGY
 * LIMITED, ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
 * THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
 * BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
 * USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
 * EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.
 *
 * YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
 * THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
 * http://www.seagate.com/contact
 *
 * Original author:  Rajesh Nambiar   <rajesh.nambiarr@seagate.com>
 * Original creation date: 24-March-2017
 */

#include "mock_s3_clovis_wrapper.h"
#include "mock_s3_factory.h"
#include "mock_s3_request_object.h"
#include "s3_abort_multipart_action.h"
#include "s3_ut_common.h"
#include <cstdlib>

using ::testing::Return;
using ::testing::ReturnRef;
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
    object_list_indx_oid = {0x11ffff, 0x1ffff};
    upload_id = "upload_id";
    call_count_one = 0;
    ptr_mock_request =
        std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr);
    ptr_mock_s3_clovis_api = std::make_shared<MockS3Clovis>();

    EXPECT_CALL(*ptr_mock_request, get_query_string_value("uploadId"))
        .WillRepeatedly(Return("upload_id"));

    EXPECT_CALL(*ptr_mock_s3_clovis_api, m0_h_ufid_next(_))
        .WillOnce(Invoke(dummy_helpers_ufid_next));

    bucket_meta_factory = std::make_shared<MockS3BucketMetadataFactory>(
        ptr_mock_request, ptr_mock_s3_clovis_api);
    object_mp_meta_factory =
        std::make_shared<MockS3ObjectMultipartMetadataFactory>(
            ptr_mock_request, ptr_mock_s3_clovis_api, mp_indx_oid, true,
            upload_id);
    object_meta_factory = std::make_shared<MockS3ObjectMetadataFactory>(
        ptr_mock_request, object_list_indx_oid, ptr_mock_s3_clovis_api);
    part_meta_factory = std::make_shared<MockS3PartMetadataFactory>(
        ptr_mock_request, oid, upload_id, 0);
    clovis_writer_factory = std::make_shared<MockS3ClovisWriterFactory>(
        ptr_mock_request, oid, ptr_mock_s3_clovis_api);
    clovis_kvs_reader_factory = std::make_shared<MockS3ClovisKVSReaderFactory>(
        ptr_mock_request, ptr_mock_s3_clovis_api);

    action_under_test.reset(new S3AbortMultipartAction(
        ptr_mock_request, ptr_mock_s3_clovis_api, bucket_meta_factory,
        object_mp_meta_factory, object_meta_factory, part_meta_factory,
        clovis_writer_factory, clovis_kvs_reader_factory));
  }

  std::shared_ptr<MockS3RequestObject> ptr_mock_request;
  std::shared_ptr<MockS3Clovis> ptr_mock_s3_clovis_api;
  std::shared_ptr<MockS3BucketMetadataFactory> bucket_meta_factory;
  std::shared_ptr<MockS3ObjectMetadataFactory> object_meta_factory;
  std::shared_ptr<MockS3PartMetadataFactory> part_meta_factory;
  std::shared_ptr<MockS3ObjectMultipartMetadataFactory> object_mp_meta_factory;
  std::shared_ptr<MockS3ClovisWriterFactory> clovis_writer_factory;
  std::shared_ptr<MockS3ClovisKVSReaderFactory> clovis_kvs_reader_factory;
  std::shared_ptr<S3AbortMultipartAction> action_under_test;
  struct m0_uint128 mp_indx_oid;
  struct m0_uint128 object_list_indx_oid;
  struct m0_uint128 oid;
  std::string upload_id;
  std::string object_name;
  std::string bucket_name;
  int call_count_one;

 public:
  void func_callback_one() { call_count_one += 1; }
};

TEST_F(S3AbortMultipartActionTest, ConstructorTest) {
  EXPECT_NE(0, action_under_test->number_of_tasks());
  EXPECT_TRUE(action_under_test->s3_clovis_api != NULL);
}

TEST_F(S3AbortMultipartActionTest, FetchBucketInfoTest) {
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), load(_, _))
      .Times(AtLeast(1));
  action_under_test->fetch_bucket_info();
  EXPECT_TRUE(action_under_test->bucket_metadata != NULL);
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

  action_under_test->get_multipart_metadata();
}

TEST_F(S3AbortMultipartActionTest, DeleteMultipartMetadataTest1) {
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .WillRepeatedly(Return(S3ObjectMetadataState::present));
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata),
              get_upload_id())
      .WillOnce(Return("upload_id"));
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), remove(_, _))
      .Times(1);
  action_under_test->delete_multipart_metadata();
}

TEST_F(S3AbortMultipartActionTest, DeleteMultipartMetadataTest2) {
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .WillRepeatedly(Return(S3ObjectMetadataState::present));
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata),
              get_upload_id())
      .WillOnce(Return("upload_id_different"));
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), remove(_, _))
      .Times(0);

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(404, _)).Times(1);
  action_under_test->delete_multipart_metadata();
  EXPECT_STREQ("NoSuchUpload", action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3AbortMultipartActionTest, DeleteMultipartMetadataTest3) {
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .WillRepeatedly(Return(S3ObjectMetadataState::missing));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(404, _)).Times(1);
  action_under_test->delete_multipart_metadata();
  EXPECT_STREQ("NoSuchUpload", action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3AbortMultipartActionTest, DeleteMultipartMetadataTest4) {
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .WillRepeatedly(Return(S3ObjectMetadataState::failed_to_launch));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(503, _)).Times(1);
  action_under_test->delete_multipart_metadata();
  EXPECT_STREQ("ServiceUnavailable",
               action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3AbortMultipartActionTest, DeleteMultipartMetadataTest5) {
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .WillRepeatedly(Return(S3ObjectMetadataState::failed));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(500, _)).Times(1);
  action_under_test->delete_multipart_metadata();
  EXPECT_STREQ("InternalError", action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3AbortMultipartActionTest, DeleteMultipartMetadataFailedTest) {
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(S3ObjectMetadataState::failed));
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(500, _)).Times(1);
  action_under_test->delete_multipart_metadata_failed();
  action_under_test->check_shutdown_signal_for_next_task(false);
  EXPECT_STREQ("InternalError", action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3AbortMultipartActionTest, DeleteMultipartMetadataFailedToLaunchTest) {
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(S3ObjectMetadataState::failed_to_launch));
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(503, _)).Times(1);
  action_under_test->delete_multipart_metadata_failed();
  action_under_test->check_shutdown_signal_for_next_task(false);
  EXPECT_STREQ("ServiceUnavailable",
               action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3AbortMultipartActionTest, DeleteObjectTest1) {
  int layout_id = 1;
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata),
              get_layout_id()).WillRepeatedly(Return(layout_id));
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .WillRepeatedly(Return(S3ObjectMetadataState::present));

  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer),
              delete_object(_, _, _))
      .Times(1);
  action_under_test->delete_object();
}

TEST_F(S3AbortMultipartActionTest, DeleteObjectTest2) {
  int layout_id = 1;
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata),
              get_layout_id()).WillRepeatedly(Return(layout_id));

  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .WillRepeatedly(Return(S3ObjectMetadataState::deleted));

  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer),
              delete_object(_, _, _))
      .Times(1);
  action_under_test->delete_object();
}

TEST_F(S3AbortMultipartActionTest, DeleteObjectTest3) {
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .WillRepeatedly(Return(S3ObjectMetadataState::missing));

  action_under_test->check_shutdown_signal_for_next_task(true);
  S3Option::get_instance()->set_is_s3_shutting_down(true);
  EXPECT_CALL(*ptr_mock_request, pause()).Times(1);
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(_, _)).Times(1);
  action_under_test->delete_object();
  action_under_test->check_shutdown_signal_for_next_task(false);
  S3Option::get_instance()->set_is_s3_shutting_down(false);
}

TEST_F(S3AbortMultipartActionTest, DeleteObjectFailedTest1) {
  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer), get_state())
      .WillRepeatedly(Return(S3ClovisWriterOpState::failed));
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .WillRepeatedly(Return(S3ObjectMetadataState::present));

  S3Option::get_instance()->set_is_s3_shutting_down(true);
  action_under_test->check_shutdown_signal_for_next_task(true);
  EXPECT_CALL(*ptr_mock_request, pause()).Times(1);
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(_, _)).Times(1);

  action_under_test->delete_object_failed();
  action_under_test->check_shutdown_signal_for_next_task(false);
  S3Option::get_instance()->set_is_s3_shutting_down(false);
}

TEST_F(S3AbortMultipartActionTest, DeleteObjectFailedToLaunchTest) {
  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer), get_state())
      .WillRepeatedly(Return(S3ClovisWriterOpState::failed_to_launch));
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;

  S3Option::get_instance()->set_is_s3_shutting_down(true);
  action_under_test->check_shutdown_signal_for_next_task(true);
  EXPECT_CALL(*ptr_mock_request, pause()).Times(1);
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(503, _)).Times(1);

  action_under_test->delete_object_failed();
  action_under_test->check_shutdown_signal_for_next_task(false);
  S3Option::get_instance()->set_is_s3_shutting_down(false);
}

TEST_F(S3AbortMultipartActionTest, DeletePartIndexWithPartsTest1) {
  action_under_test->part_index_oid = {0ULL, 0ULL};

  action_under_test->check_shutdown_signal_for_next_task(true);
  S3Option::get_instance()->set_is_s3_shutting_down(true);
  EXPECT_CALL(*ptr_mock_request, pause()).Times(1);
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(_, _)).Times(1);
  action_under_test->delete_part_index_with_parts();
  action_under_test->check_shutdown_signal_for_next_task(false);
  S3Option::get_instance()->set_is_s3_shutting_down(false);
}

TEST_F(S3AbortMultipartActionTest, DeletePartIndexWithPartsTest2) {
  action_under_test->part_index_oid = {0xffff, 0xffff};
  EXPECT_CALL(*(part_meta_factory->mock_part_metadata), remove_index(_, _))
      .Times(1);
  action_under_test->delete_part_index_with_parts();
}

TEST_F(S3AbortMultipartActionTest, DeletePartIndexWithPartsFailed) {
  S3Option::get_instance()->set_is_s3_shutting_down(true);
  action_under_test->check_shutdown_signal_for_next_task(true);
  action_under_test->part_metadata = part_meta_factory->mock_part_metadata;
  EXPECT_CALL(*(part_meta_factory->mock_part_metadata), get_state())
      .WillRepeatedly(Return(S3PartMetadataState::failed));
  EXPECT_CALL(*ptr_mock_request, pause()).Times(1);
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(_, _)).Times(1);

  action_under_test->delete_part_index_with_parts_failed();
  action_under_test->check_shutdown_signal_for_next_task(false);
  S3Option::get_instance()->set_is_s3_shutting_down(false);
}

TEST_F(S3AbortMultipartActionTest, DeletePartIndexWithPartsFailedToLaunch) {
  S3Option::get_instance()->set_is_s3_shutting_down(true);
  action_under_test->check_shutdown_signal_for_next_task(true);
  action_under_test->part_metadata = part_meta_factory->mock_part_metadata;
  EXPECT_CALL(*(part_meta_factory->mock_part_metadata), get_state())
      .WillRepeatedly(Return(S3PartMetadataState::failed_to_launch));
  EXPECT_CALL(*ptr_mock_request, pause()).Times(1);
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(503, _)).Times(1);

  action_under_test->delete_part_index_with_parts_failed();
  action_under_test->check_shutdown_signal_for_next_task(false);
  S3Option::get_instance()->set_is_s3_shutting_down(false);
}

TEST_F(S3AbortMultipartActionTest, Send200SuccessToS3Client) {
  action_under_test->part_metadata = part_meta_factory->mock_part_metadata;
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
