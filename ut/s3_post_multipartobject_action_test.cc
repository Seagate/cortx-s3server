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
 * Original creation date: 06-Feb-2017
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
        S3ClovisLayoutMap::get_instance()->get_best_layout_for_object_size();

    ptr_mock_request =
        std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr);

    ptr_mock_s3_clovis_api = std::make_shared<MockS3Clovis>();

    EXPECT_CALL(*ptr_mock_s3_clovis_api, m0_h_ufid_next(_))
        .WillRepeatedly(Invoke(dummy_helpers_ufid_next));

    // Owned and deleted by shared_ptr in S3PostMultipartObjectAction
    bucket_meta_factory = new MockS3BucketMetadataFactory(ptr_mock_request);
    object_mp_meta_factory = new MockS3ObjectMultipartMetadataFactory(
        ptr_mock_request, ptr_mock_s3_clovis_api, mp_indx_oid, true, upload_id);
    object_meta_factory = new MockS3ObjectMetadataFactory(
        ptr_mock_request, object_list_indx_oid, ptr_mock_s3_clovis_api);
    part_meta_factory =
        new MockS3PartMetadataFactory(ptr_mock_request, oid, upload_id, 0);
    clovis_writer_factory = new MockS3ClovisWriterFactory(
        ptr_mock_request, oid, ptr_mock_s3_clovis_api);

    action_under_test.reset(new S3PostMultipartObjectAction(
        ptr_mock_request, bucket_meta_factory, object_mp_meta_factory,
        object_meta_factory, part_meta_factory, clovis_writer_factory,
        ptr_mock_s3_clovis_api));
  }

  std::shared_ptr<MockS3RequestObject> ptr_mock_request;
  std::shared_ptr<MockS3Clovis> ptr_mock_s3_clovis_api;
  MockS3BucketMetadataFactory *bucket_meta_factory;
  MockS3ObjectMetadataFactory *object_meta_factory;
  MockS3PartMetadataFactory *part_meta_factory;
  MockS3ObjectMultipartMetadataFactory *object_mp_meta_factory;
  MockS3ClovisWriterFactory *clovis_writer_factory;
  std::shared_ptr<S3PostMultipartObjectAction> action_under_test;
  struct m0_uint128 mp_indx_oid;
  struct m0_uint128 object_list_indx_oid;
  struct m0_uint128 oid;
  std::string upload_id;
  int call_count_one;
  int layout_id;

 public:
  void func_callback_one() { call_count_one += 1; }
};

TEST_F(S3PostMultipartObjectTest, ConstructorTest) {
  EXPECT_EQ(0, action_under_test->tried_count);
  EXPECT_EQ("uri_salt_", action_under_test->salt);
  EXPECT_NE(0, action_under_test->number_of_tasks());
}

TEST_F(S3PostMultipartObjectTest, FetchBucketInfo) {
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), load(_, _))
      .Times(AtLeast(1));
  action_under_test->fetch_bucket_info();
  EXPECT_TRUE(action_under_test->bucket_metadata != NULL);
}

TEST_F(S3PostMultipartObjectTest, UploadInProgress) {
  struct m0_uint128 oid = {0xffff, 0xffff};
  struct m0_uint128 empty_oid = {0ULL, 0ULL};

  action_under_test->bucket_metadata =
      bucket_meta_factory->mock_bucket_metadata;

  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillRepeatedly(Return(S3BucketMetadataState::present));
  action_under_test->bucket_metadata->set_multipart_index_oid(oid);
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), load(_, _))
      .Times(1);
  action_under_test->check_upload_is_inprogress();

  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillRepeatedly(Return(S3BucketMetadataState::present));
  action_under_test->clear_tasks();
  action_under_test->add_task(
      std::bind(&S3PostMultipartObjectTest::func_callback_one, this));
  action_under_test->bucket_metadata->set_multipart_index_oid(empty_oid);
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), load(_, _))
      .Times(0);
  action_under_test->check_upload_is_inprogress();

  EXPECT_EQ(1, call_count_one);

  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillRepeatedly(Return(S3BucketMetadataState::missing));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(_, _)).Times(1);
  action_under_test->check_upload_is_inprogress();
}

TEST_F(S3PostMultipartObjectTest, FetchObjectInfoMultipartPresent) {
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;
  action_under_test->bucket_metadata =
      bucket_meta_factory->mock_bucket_metadata;

  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .Times(1)
      .WillRepeatedly(Return(S3ObjectMetadataState::present));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  S3Option::get_instance()->set_is_s3_shutting_down(false);

  EXPECT_CALL(*ptr_mock_request,
              send_response(403, HasSubstr("<Code>InvalidObjectState</Code>")))
      .Times(1);
  action_under_test->fetch_object_info();
  EXPECT_STREQ("InvalidObjectState",
               action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3PostMultipartObjectTest, FetchObjectInfoMultipartNotPresent) {
  struct m0_uint128 oid = {0xffff, 0xffff};
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .Times(1)
      .WillRepeatedly(Return(S3ObjectMetadataState::missing));

  action_under_test->bucket_metadata =
      bucket_meta_factory->mock_bucket_metadata;
  bucket_meta_factory->mock_bucket_metadata->set_object_list_index_oid(oid);

  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), load(_, _))
      .Times(AtLeast(1));
  action_under_test->fetch_object_info();
}

TEST_F(S3PostMultipartObjectTest, FetchObjectInfoObjectNotPresent) {
  struct m0_uint128 empty_oid = {0ULL, 0ULL};
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .Times(1)
      .WillRepeatedly(Return(S3ObjectMetadataState::missing));

  action_under_test->bucket_metadata =
      bucket_meta_factory->mock_bucket_metadata;

  bucket_meta_factory->mock_bucket_metadata->set_object_list_index_oid(
      empty_oid);
  action_under_test->clear_tasks();
  action_under_test->add_task(
      std::bind(&S3PostMultipartObjectTest::func_callback_one, this));
  action_under_test->fetch_object_info();
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3PostMultipartObjectTest, FetchObjectInfoStatusObjectPresent) {
  struct m0_uint128 old_oid = {0x1ffff, 0x1ffff};
  struct m0_uint128 empty_oid = {0ULL, 0ULL};
  action_under_test->object_metadata =
      object_meta_factory->mock_object_metadata;
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_state())
      .Times(1)
      .WillRepeatedly(Return(S3ObjectMetadataState::present));
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_oid())
      .Times(1)
      .WillOnce(Return(old_oid));
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_layout_id())
      .WillOnce(Return(layout_id));

  action_under_test->clear_tasks();
  action_under_test->add_task(
      std::bind(&S3PostMultipartObjectTest::func_callback_one, this));
  struct m0_uint128 oid_before_regen = action_under_test->oid;
  action_under_test->fetch_object_info_status();
  EXPECT_EQ(1, call_count_one);
  EXPECT_OID_NE(empty_oid, action_under_test->old_oid);
  EXPECT_OID_NE(oid_before_regen, action_under_test->oid);
}

TEST_F(S3PostMultipartObjectTest, FetchObjectInfoStatusObjectNotPresent) {
  struct m0_uint128 zero_oid = {0ULL, 0ULL};
  action_under_test->object_metadata =
      object_meta_factory->mock_object_metadata;
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_state())
      .Times(1)
      .WillRepeatedly(Return(S3ObjectMetadataState::missing));

  action_under_test->clear_tasks();
  action_under_test->add_task(
      std::bind(&S3PostMultipartObjectTest::func_callback_one, this));
  action_under_test->fetch_object_info_status();
  EXPECT_EQ(1, call_count_one);
  EXPECT_OID_EQ(zero_oid, action_under_test->old_oid);
}

TEST_F(S3PostMultipartObjectTest, CreateObject) {
  action_under_test->bucket_metadata =
      bucket_meta_factory->mock_bucket_metadata;
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;

  action_under_test->tried_count = 0;
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer),
              create_object(_, _, _))
      .Times(1);
  action_under_test->create_object();

  action_under_test->tried_count = 1;
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer),
              create_object(_, _, _))
      .Times(1);
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer), set_oid(_))
      .Times(1);
  action_under_test->create_object();
}

TEST_F(S3PostMultipartObjectTest, CreateObjectFailed) {
  S3Option::get_instance()->set_is_s3_shutting_down(true);
  EXPECT_CALL(*ptr_mock_request, pause()).Times(1);
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(503, _)).Times(1);

  action_under_test->create_object_failed();
  EXPECT_TRUE(action_under_test->clovis_writer == NULL);
  S3Option::get_instance()->set_is_s3_shutting_down(false);

  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer), get_state())
      .Times(1)
      .WillRepeatedly(Return(S3ClovisWriterOpState::failed));
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(500, _)).Times(1);
  action_under_test->create_object_failed();
  EXPECT_STREQ("InternalError", action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3PostMultipartObjectTest, CreateObjectFailedDueToCollision) {
  S3Option::get_instance()->set_is_s3_shutting_down(false);
  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;

  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer), get_state())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(S3ClovisWriterOpState::exists));
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer), set_oid(_))
      .Times(1);
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer),
              create_object(_, _, _))
      .Times(1);
  action_under_test->tried_count = 1;
  action_under_test->create_object_failed();
  EXPECT_EQ(2, action_under_test->tried_count);
}

TEST_F(S3PostMultipartObjectTest, CollisionOccured) {
  S3Option::get_instance()->set_is_s3_shutting_down(false);
  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;
  action_under_test->tried_count = 1;
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer),
              create_object(_, _, _))
      .Times(AtLeast(1));
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer), set_oid(_))
      .Times(1);
  action_under_test->collision_occured();
  EXPECT_EQ(2, action_under_test->tried_count);

  action_under_test->tried_count = MAX_COLLISION_RETRY_COUNT + 1;
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(500, _)).Times(1);
  action_under_test->collision_occured();
  EXPECT_STREQ("InternalError", action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3PostMultipartObjectTest, CreateNewOid) {
  struct m0_uint128 new_oid, old_oid;
  S3Option::get_instance()->enable_murmurhash_oid();
  old_oid = {0xffff, 0xffff};
  action_under_test->set_oid(old_oid);
  action_under_test->create_new_oid(old_oid);
  new_oid = action_under_test->get_oid();
  EXPECT_OID_NE(old_oid, new_oid);
  S3Option::get_instance()->disable_murmurhash_oid();
}

TEST_F(S3PostMultipartObjectTest, RollbackCreate) {
  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer),
              delete_object(_, _, _))
      .Times(1);
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer), set_oid(_))
      .Times(1);
  action_under_test->rollback_create();
}

TEST_F(S3PostMultipartObjectTest, RollbackCreateFailedMetadataMissing) {
  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer), get_state())
      .WillRepeatedly(Return(S3ClovisWriterOpState::missing));
  action_under_test->add_task_rollback(
      std::bind(&S3PostMultipartObjectTest::func_callback_one, this));
  action_under_test->rollback_create_failed();
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3PostMultipartObjectTest, RollbackCreateFailedMetadataFailed) {
  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;
  action_under_test->add_task_rollback(
      std::bind(&S3PostMultipartObjectTest::func_callback_one, this));
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer), get_state())
      .WillRepeatedly(Return(S3ClovisWriterOpState::failed));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(500, _)).Times(1);
  action_under_test->set_s3_error("InternalError");

  action_under_test->rollback_create_failed();
  EXPECT_STREQ("InternalError", action_under_test->get_s3_error_code().c_str());
  EXPECT_EQ(0, call_count_one);
}

TEST_F(S3PostMultipartObjectTest, RollbackPartMetadataIndex) {
  action_under_test->part_metadata = part_meta_factory->mock_part_metadata;
  EXPECT_CALL(*(part_meta_factory->mock_part_metadata), remove_index(_, _))
      .Times(AtLeast(1));
  action_under_test->rollback_create_part_meta_index();
}

TEST_F(S3PostMultipartObjectTest, RollbackPartMetadataIndexFailed) {
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(_, _)).Times(1);
  action_under_test->set_s3_error("InternalError");
  action_under_test->rollback_create_part_meta_index_failed();
  EXPECT_STREQ("InternalError", action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3PostMultipartObjectTest, SaveUploadMetadataFailed) {
  action_under_test->add_task_rollback(
      std::bind(&S3PostMultipartObjectTest::func_callback_one, this));
  action_under_test->save_upload_metadata_failed();
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3PostMultipartObjectTest, RollbackUploadMetadata) {
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), remove(_, _))
      .Times(1);
  action_under_test->rollback_upload_metadata();
}

TEST_F(S3PostMultipartObjectTest, RollbackUploadMetadataFailMetadataMissing) {
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .WillRepeatedly(Return(S3ObjectMetadataState::missing));

  action_under_test->add_task_rollback(
      std::bind(&S3PostMultipartObjectTest::func_callback_one, this));
  action_under_test->rollback_upload_metadata_failed();
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3PostMultipartObjectTest, RollbackUploadMetadataFailMetadataPresent) {
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .WillRepeatedly(Return(S3ObjectMetadataState::present));
  action_under_test->set_s3_error("InternalError");

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(500, _)).Times(1);
  action_under_test->rollback_upload_metadata_failed();
  EXPECT_STREQ("InternalError", action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3PostMultipartObjectTest, CreatePartMetadataIndex) {
  EXPECT_CALL(*(part_meta_factory->mock_part_metadata), create_index(_, _))
      .Times(AtLeast(1));
  action_under_test->create_part_meta_index();
}

TEST_F(S3PostMultipartObjectTest, SaveMultipartMetadata) {
  action_under_test->bucket_metadata =
      bucket_meta_factory->mock_bucket_metadata;
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;

  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), save(_, _))
      .Times(AtLeast(1));
  action_under_test->save_multipart_metadata();
  EXPECT_EQ(1, action_under_test->number_of_rollback_tasks());
}

TEST_F(S3PostMultipartObjectTest, SaveMultipartMetadataFailed) {
  action_under_test->add_task_rollback(
      std::bind(&S3PostMultipartObjectTest::func_callback_one, this));
  action_under_test->save_multipart_metadata_failed();
  EXPECT_EQ(1, call_count_one);
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
  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .WillRepeatedly(Return(S3ObjectMetadataState::saved));
  EXPECT_CALL(*(part_meta_factory->mock_part_metadata), get_state())
      .WillRepeatedly(Return(S3PartMetadataState::store_created));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(200, _)).Times(AtLeast(1));
  action_under_test->send_response_to_s3_client();
}
