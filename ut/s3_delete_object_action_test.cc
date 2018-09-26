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
 * Original author:  Kaustubh Deorukhkar   <kaustubh.deorukhkar@seagate.com>
 * Original creation date: 24-Apr-2017
 */

#include <memory>

#include "mock_s3_clovis_wrapper.h"
#include "mock_s3_factory.h"
#include "s3_delete_object_action.h"
#include "s3_error_codes.h"
#include "s3_test_utils.h"
#include "s3_ut_common.h"

using ::testing::Eq;
using ::testing::Return;
using ::testing::Invoke;
using ::testing::_;
using ::testing::AtLeast;

#define CREATE_BUCKET_METADATA                                            \
  do {                                                                    \
    EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), load(_, _)) \
        .Times(AtLeast(1));                                               \
    action_under_test->fetch_bucket_info();                               \
  } while (0)

#define CREATE_OBJECT_METADATA                                             \
  do {                                                                     \
    CREATE_BUCKET_METADATA;                                                \
    EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state()) \
        .WillOnce(Return(S3BucketMetadataState::present));                 \
    bucket_meta_factory->mock_bucket_metadata->set_object_list_index_oid(  \
        object_list_indx_oid);                                             \
    EXPECT_CALL(*(object_meta_factory->mock_object_metadata), load(_, _))  \
        .Times(AtLeast(1));                                                \
    action_under_test->fetch_object_info();                                \
  } while (0)

class S3DeleteObjectActionTest : public testing::Test {
 protected:
  S3DeleteObjectActionTest() {
    S3Option::get_instance()->disable_auth();

    evhtp_request_t *req = NULL;
    EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();

    oid = {0x1ffff, 0x1ffff};
    object_list_indx_oid = {0x11ffff, 0x1ffff};

    call_count_one = 0;

    layout_id =
        S3ClovisLayoutMap::get_instance()->get_best_layout_for_object_size();

    async_buffer_factory =
        std::make_shared<MockS3AsyncBufferOptContainerFactory>(
            S3Option::get_instance()->get_libevent_pool_buffer_size());

    mock_request = std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr,
                                                         async_buffer_factory);

    ptr_mock_s3_clovis_api = std::make_shared<MockS3Clovis>();

    EXPECT_CALL(*ptr_mock_s3_clovis_api, m0_h_ufid_next(_))
        .WillRepeatedly(Invoke(dummy_helpers_ufid_next));

    // Owned and deleted by shared_ptr in S3DeleteObjectAction
    bucket_meta_factory = std::make_shared<MockS3BucketMetadataFactory>(
        mock_request, ptr_mock_s3_clovis_api);

    object_meta_factory = std::make_shared<MockS3ObjectMetadataFactory>(
        mock_request, object_list_indx_oid, ptr_mock_s3_clovis_api);

    clovis_writer_factory = std::make_shared<MockS3ClovisWriterFactory>(
        mock_request, oid, ptr_mock_s3_clovis_api);

    action_under_test.reset(
        new S3DeleteObjectAction(mock_request, bucket_meta_factory,
                                 object_meta_factory, clovis_writer_factory));
  }

  std::shared_ptr<MockS3RequestObject> mock_request;
  std::shared_ptr<MockS3Clovis> ptr_mock_s3_clovis_api;
  std::shared_ptr<MockS3BucketMetadataFactory> bucket_meta_factory;
  std::shared_ptr<MockS3ObjectMetadataFactory> object_meta_factory;
  std::shared_ptr<MockS3ClovisWriterFactory> clovis_writer_factory;
  std::shared_ptr<MockS3AsyncBufferOptContainerFactory> async_buffer_factory;

  std::shared_ptr<S3DeleteObjectAction> action_under_test;

  struct m0_uint128 object_list_indx_oid;
  struct m0_uint128 oid;

  int call_count_one;
  int layout_id;

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

  action_under_test->fetch_object_info();

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

  action_under_test->fetch_object_info();

  EXPECT_STREQ("InternalError", action_under_test->get_s3_error_code().c_str());
  EXPECT_TRUE(action_under_test->bucket_metadata != NULL);
  EXPECT_TRUE(action_under_test->object_metadata == NULL);
}

TEST_F(S3DeleteObjectActionTest, FetchObjectInfoWhenBucketAndObjIndexPresent) {
  CREATE_BUCKET_METADATA;

  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillRepeatedly(Return(S3BucketMetadataState::present));
  bucket_meta_factory->mock_bucket_metadata->set_object_list_index_oid(
      object_list_indx_oid);

  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), load(_, _))
      .Times(AtLeast(1));

  action_under_test->fetch_object_info();

  EXPECT_TRUE(action_under_test->bucket_metadata != NULL);
  EXPECT_TRUE(action_under_test->object_metadata != NULL);
}

TEST_F(S3DeleteObjectActionTest,
       FetchObjectInfoWhenBucketPresentAndObjIndexAbsent) {
  CREATE_BUCKET_METADATA;

  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillRepeatedly(Return(S3BucketMetadataState::present));

  EXPECT_CALL(*mock_request, send_response(S3HttpSuccess204, _)).Times(1);

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

  action_under_test->delete_metadata();
}

TEST_F(S3DeleteObjectActionTest, DeleteMetadataWhenObjectMetadataFetchFailed) {
  CREATE_OBJECT_METADATA;

  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_state())
      .WillRepeatedly(Return(S3ObjectMetadataState::failed));

  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, send_response(S3HttpFailed500, _)).Times(1);

  action_under_test->delete_metadata();

  EXPECT_STREQ("InternalError", action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3DeleteObjectActionTest, DeleteObjectWhenMetadataDeleteFailed) {
  CREATE_OBJECT_METADATA;

  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_state())
      .WillRepeatedly(Return(S3ObjectMetadataState::failed));

  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, send_response(S3HttpFailed500, _)).Times(1);

  action_under_test->delete_object();

  EXPECT_STREQ("InternalError", action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3DeleteObjectActionTest, DeleteObjectWhenMetadataDeleteSucceeded) {
  CREATE_OBJECT_METADATA;

  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_state())
      .WillRepeatedly(Return(S3ObjectMetadataState::deleted));
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_oid())
      .Times(1)
      .WillOnce(Return(oid));
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_layout_id())
      .WillOnce(Return(layout_id));

  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer),
              delete_object(_, _, _))
      .Times(1);

  action_under_test->delete_object();
}

TEST_F(S3DeleteObjectActionTest, DeleteObjectMissingShouldReportDeleted) {
  CREATE_OBJECT_METADATA;
  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;

  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer), get_state())
      .WillRepeatedly(Return(S3ClovisWriterOpState::missing));

  // Mock out the next calls on action.
  action_under_test->clear_tasks();
  action_under_test->add_task(
      std::bind(&S3DeleteObjectActionTest::func_callback_one, this));

  action_under_test->delete_object_failed();

  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3DeleteObjectActionTest, DeleteObjectFailedShouldReportDeleted) {
  CREATE_OBJECT_METADATA;

  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer), get_state())
      .WillRepeatedly(Return(S3ClovisWriterOpState::failed));
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_oid())
      .WillRepeatedly(Return(oid));

  EXPECT_CALL(*mock_request, send_response(S3HttpSuccess204, _)).Times(1);

  action_under_test->delete_object_failed();
}

TEST_F(S3DeleteObjectActionTest,
       DeleteObjectFailedToLaunchShouldReportDeleted) {
  CREATE_OBJECT_METADATA;

  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer), get_state())
      .WillRepeatedly(Return(S3ClovisWriterOpState::failed_to_launch));
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_oid())
      .WillRepeatedly(Return(oid));

  EXPECT_CALL(*mock_request, send_response(503, _)).Times(1);

  action_under_test->delete_object_failed();
}

TEST_F(S3DeleteObjectActionTest, SendErrorResponse) {
  action_under_test->set_s3_error("InternalError");

  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, send_response(S3HttpFailed500, _))
      .Times(AtLeast(1));

  action_under_test->send_response_to_s3_client();
}

TEST_F(S3DeleteObjectActionTest, SendAnyFailedResponse) {
  action_under_test->set_s3_error("NoSuchKey");

  EXPECT_CALL(*mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*mock_request, send_response(S3HttpFailed404, _))
      .Times(AtLeast(1));

  action_under_test->send_response_to_s3_client();
}

TEST_F(S3DeleteObjectActionTest, SendSuccessResponse) {
  CREATE_OBJECT_METADATA;

  EXPECT_CALL(*mock_request, send_response(S3HttpSuccess204, _))
      .Times(AtLeast(1));

  action_under_test->send_response_to_s3_client();
}
