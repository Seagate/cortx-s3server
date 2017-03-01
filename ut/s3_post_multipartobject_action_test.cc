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

#include <memory>

#include "mock_s3_bucket_metadata.h"
#include "mock_s3_clovis_writer.h"
#include "mock_s3_object_metadata.h"
#include "mock_s3_object_multipart_metadata.h"
#include "mock_s3_part_metadata.h"
#include "mock_s3_request_object.h"
#include "s3_factory.h"
#include "s3_post_multipartobject_action.h"

using ::testing::Return;
using ::testing::Invoke;
using ::testing::_;
using ::testing::ReturnRef;
using ::testing::AtLeast;
using ::testing::DefaultValue;

class MockS3BucketMetadataFactory : public S3BucketMetadataFactory {
 public:
  MockS3BucketMetadataFactory(std::shared_ptr<S3RequestObject> req)
      : S3BucketMetadataFactory() {
    //  We create object here since we want to set some expectations
    // Before create_bucket_metadata_obj() is called
    mock_bucket_metadata = std::make_shared<MockS3BucketMetadata>(req);
  }

  std::shared_ptr<S3BucketMetadata> create_bucket_metadata_obj(
      std::shared_ptr<S3RequestObject> req) {
    return mock_bucket_metadata;
  }

  // Use this to setup your expectations.
  std::shared_ptr<MockS3BucketMetadata> mock_bucket_metadata;
};

class MockS3ObjectMetadataFactory : public S3ObjectMetadataFactory {
 public:
  MockS3ObjectMetadataFactory(std::shared_ptr<S3RequestObject> req,
                              m0_uint128 object_list_indx_oid)
      : S3ObjectMetadataFactory() {
    mock_object_metadata =
        std::make_shared<MockS3ObjectMetadata>(req, object_list_indx_oid);
  }

  std::shared_ptr<S3ObjectMetadata> create_object_metadata_obj(
      std::shared_ptr<S3RequestObject> req, struct m0_uint128 indx_oid) {
    return mock_object_metadata;
  }

  std::shared_ptr<MockS3ObjectMetadata> mock_object_metadata;
};

class MockS3PartMetadataFactory : public S3PartMetadataFactory {
 public:
  MockS3PartMetadataFactory(std::shared_ptr<S3RequestObject> req,
                            m0_uint128 indx_oid, std::string upload_id,
                            int part_num)
      : S3PartMetadataFactory() {
    mock_part_metadata = std::make_shared<MockS3PartMetadata>(
        req, indx_oid, upload_id, part_num);
  }

  std::shared_ptr<S3PartMetadata> create_part_metadata_obj(
      std::shared_ptr<S3RequestObject> req, struct m0_uint128 indx_oid,
      std::string upload_id, int part_num) {
    s3_log(S3_LOG_DEBUG, "here....\n");
    return mock_part_metadata;
  }

  std::shared_ptr<S3PartMetadata> create_part_metadata_obj(
      std::shared_ptr<S3RequestObject> req, std::string upload_id,
      int part_num) {
    s3_log(S3_LOG_DEBUG, "here....\n");
    return mock_part_metadata;
  }

  std::shared_ptr<MockS3PartMetadata> mock_part_metadata;
};

class MockS3ObjectMultipartMetadataFactory
    : public S3ObjectMultipartMetadataFactory {
 public:
  MockS3ObjectMultipartMetadataFactory(std::shared_ptr<S3RequestObject> req,
                                       m0_uint128 mp_indx_oid, bool is_mp,
                                       std::string upload_id)
      : S3ObjectMultipartMetadataFactory() {
    //  We create object here since we want to set some expectations
    // Before create_bucket_metadata_obj() is called
    mock_object_mp_metadata = std::make_shared<MockS3ObjectMultipartMetadata>(
        req, mp_indx_oid, is_mp, upload_id);
  }

  std::shared_ptr<S3ObjectMetadata> create_object_mp_metadata_obj(
      std::shared_ptr<S3RequestObject> req, struct m0_uint128 mp_indx_oid,
      bool is_mp, std::string upload_id) {
    s3_log(S3_LOG_DEBUG, "here....\n");
    return mock_object_mp_metadata;
  }

  // Use this to setup your expectations.
  std::shared_ptr<MockS3ObjectMultipartMetadata> mock_object_mp_metadata;
};

class MockS3ClovisWriterFactory : public S3ClovisWriterFactory {
 public:
  MockS3ClovisWriterFactory(std::shared_ptr<S3RequestObject> req,
                            m0_uint128 oid)
      : S3ClovisWriterFactory() {
    mock_clovis_writer = std::make_shared<MockS3ClovisWriter>(req, oid);
  }

  std::shared_ptr<S3ClovisWriter> create_clovis_writer(
      std::shared_ptr<S3RequestObject> req, struct m0_uint128 oid) {
    s3_log(S3_LOG_DEBUG, "here....\n");
    return mock_clovis_writer;
  }

  std::shared_ptr<MockS3ClovisWriter> mock_clovis_writer;
};

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
    ptr_mock_request =
        std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr);

    // Owned and deleted by shared_ptr in S3PostMultipartObjectAction
    bucket_meta_factory = new MockS3BucketMetadataFactory(ptr_mock_request);
    object_mp_meta_factory = new MockS3ObjectMultipartMetadataFactory(
        ptr_mock_request, mp_indx_oid, true, upload_id);
    object_meta_factory =
        new MockS3ObjectMetadataFactory(ptr_mock_request, object_list_indx_oid);
    part_meta_factory =
        new MockS3PartMetadataFactory(ptr_mock_request, oid, upload_id, 0);
    clovis_writer_factory =
        new MockS3ClovisWriterFactory(ptr_mock_request, oid);

    action_under_test.reset(new S3PostMultipartObjectAction(
        ptr_mock_request, bucket_meta_factory, object_mp_meta_factory,
        object_meta_factory, part_meta_factory, clovis_writer_factory));
  }

  std::shared_ptr<MockS3RequestObject> ptr_mock_request;
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

 public:
  void func_callback_one() { call_count_one += 1; }
};

TEST_F(S3PostMultipartObjectTest, ConstructorTest) {
  EXPECT_EQ(0, action_under_test->tried_count);
  EXPECT_EQ("uri_salt_", action_under_test->salt);
  EXPECT_NE(0, action_under_test->number_of_tasks());
}

TEST_F(S3PostMultipartObjectTest, FetchBucketInfo) {
  // Here you can set expectations on mock bucketmetadata, mock request
  // and so on
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), load(_, _))
      .Times(AtLeast(1));
  action_under_test->fetch_bucket_info();
  EXPECT_TRUE(action_under_test->bucket_metadata != NULL);
}

TEST_F(S3PostMultipartObjectTest, UploadInProgressTest) {
  struct m0_uint128 oid = {0xffff, 0xffff};
  struct m0_uint128 empty_oid = {0ULL, 0ULL};
  /*
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), load(_, _))
      .Times(AtLeast(1));
  action_under_test->fetch_bucket_info();
  */
  action_under_test->bucket_metadata =
      bucket_meta_factory->mock_bucket_metadata;

  ON_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillByDefault(Return(S3BucketMetadataState::present));
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata),
              get_multipart_index_oid())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(oid));
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), load(_, _))
      .Times(1);
  action_under_test->check_upload_is_inprogress();

  ON_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillByDefault(Return(S3BucketMetadataState::present));
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata),
              get_multipart_index_oid())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(empty_oid));
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), load(_, _))
      .Times(0);
  action_under_test->check_upload_is_inprogress();

  ON_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillByDefault(Return(S3BucketMetadataState::missing));
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(_, _)).Times(1);
  action_under_test->check_upload_is_inprogress();
}

TEST_F(S3PostMultipartObjectTest, CreateObjectTest) {
  action_under_test->bucket_metadata =
      bucket_meta_factory->mock_bucket_metadata;
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;

  ON_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .WillByDefault(Return(S3ObjectMetadataState::empty));
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .Times(AtLeast(1));

  action_under_test->tried_count = 0;
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer), create_object(_, _))
      .Times(1);
  action_under_test->create_object();

  ON_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .WillByDefault(Return(S3ObjectMetadataState::empty));
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .Times(AtLeast(1));

  action_under_test->tried_count = 1;
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer), create_object(_, _))
      .Times(1);
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer), set_oid(_))
      .Times(1);
  action_under_test->create_object();
}

TEST_F(S3PostMultipartObjectTest, CreateObjectTest2) {
  action_under_test->bucket_metadata =
      bucket_meta_factory->mock_bucket_metadata;
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;

  ON_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .WillByDefault(Return(S3ObjectMetadataState::present));
  ON_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillByDefault(Return(S3BucketMetadataState::missing));
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .Times(AtLeast(1));
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .Times(AtLeast(1));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(_, _)).Times(1);
  action_under_test->create_object();
  EXPECT_TRUE(action_under_test->clovis_writer == NULL);
}

TEST_F(S3PostMultipartObjectTest, CreateObjectFailedTest) {
  S3Option::get_instance()->set_is_s3_shutting_down(true);
  action_under_test->create_object_failed();
  EXPECT_TRUE(action_under_test->clovis_writer == NULL);

  S3Option::get_instance()->set_is_s3_shutting_down(false);
  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;
  ON_CALL(*(clovis_writer_factory->mock_clovis_writer), get_state())
      .WillByDefault(Return(S3ClovisWriterOpState::failed));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(_, _)).Times(1);
  action_under_test->create_object_failed();
}

TEST_F(S3PostMultipartObjectTest, CreateObjectFailedTest2) {
  m0_uint128 oid = {0xffff, 0xfffff};
  S3Option::get_instance()->set_is_s3_shutting_down(false);
  action_under_test->bucket_metadata =
      bucket_meta_factory->mock_bucket_metadata;
  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;
  action_under_test->object_metadata =
      object_meta_factory->mock_object_metadata;
  action_under_test->bucket_metadata->set_object_list_index_oid(oid);
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;
  ON_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .WillByDefault(Return(S3ObjectMetadataState::present));
  ON_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillByDefault(Return(S3BucketMetadataState::missing));
  ON_CALL(*(clovis_writer_factory->mock_clovis_writer), get_state())
      .WillByDefault(Return(S3ClovisWriterOpState::exists));

  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer), get_state())
      .Times(AtLeast(1));
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), load(_, _))
      .Times(AtLeast(1));
  action_under_test->create_object_failed();

  action_under_test->tried_count = 1;
  action_under_test->create_object_failed();
  EXPECT_EQ(2, action_under_test->tried_count);
}

TEST_F(S3PostMultipartObjectTest, CollisionTest) {
  S3Option::get_instance()->set_is_s3_shutting_down(false);
  action_under_test->bucket_metadata =
      bucket_meta_factory->mock_bucket_metadata;
  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;
  action_under_test->object_metadata =
      object_meta_factory->mock_object_metadata;
  action_under_test->bucket_metadata->set_object_list_index_oid(oid);
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;
  action_under_test->tried_count = 1;
  ON_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .WillByDefault(Return(S3ObjectMetadataState::empty));

  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .Times(AtLeast(1));
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer), create_object(_, _))
      .Times(AtLeast(1));
  action_under_test->collision_occured();
  EXPECT_EQ(2, action_under_test->tried_count);

  action_under_test->tried_count = MAX_COLLISION_RETRY + 1;
  ON_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillByDefault(Return(S3BucketMetadataState::missing));
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(_, _)).Times(1);
  action_under_test->collision_occured();
}

TEST_F(S3PostMultipartObjectTest, CreateNewOidTest) {
  struct m0_uint128 new_oid, old_oid;
  old_oid = {0xffff, 0xffff};
  action_under_test->set_oid(old_oid);
  action_under_test->create_new_oid();
  new_oid = action_under_test->get_oid();
  EXPECT_TRUE((old_oid.u_lo != new_oid.u_lo) || (old_oid.u_hi != new_oid.u_hi));
}

TEST_F(S3PostMultipartObjectTest, RollbackTest) {
  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer), delete_object(_, _))
      .Times(1);
  action_under_test->rollback_create();
}

TEST_F(S3PostMultipartObjectTest, RollbackFailedTest1) {
  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;
  ON_CALL(*(clovis_writer_factory->mock_clovis_writer), get_state())
      .WillByDefault(Return(S3ClovisWriterOpState::missing));
  action_under_test->add_task_rollback(
      std::bind(&S3PostMultipartObjectTest::func_callback_one, this));
  action_under_test->rollback_create_failed();
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3PostMultipartObjectTest, RollbackFailedTest2) {
  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;
  action_under_test->bucket_metadata =
      bucket_meta_factory->mock_bucket_metadata;
  action_under_test->add_task_rollback(
      std::bind(&S3PostMultipartObjectTest::func_callback_one, this));
  ON_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .WillByDefault(Return(S3ObjectMetadataState::present));
  ON_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillByDefault(Return(S3BucketMetadataState::missing));
  ON_CALL(*(clovis_writer_factory->mock_clovis_writer), get_state())
      .WillByDefault(Return(S3ClovisWriterOpState::failed));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(1);
  EXPECT_CALL(*ptr_mock_request, send_response(404, _)).Times(1);
  action_under_test->rollback_create_failed();
  EXPECT_EQ(0, call_count_one);
}

TEST_F(S3PostMultipartObjectTest, RollbackPartMetadataIndexTest) {
  action_under_test->part_metadata = part_meta_factory->mock_part_metadata;
  EXPECT_CALL(*(part_meta_factory->mock_part_metadata), remove_index(_, _))
      .Times(AtLeast(1));
  action_under_test->rollback_create_part_meta_index();
}

TEST_F(S3PostMultipartObjectTest, RollbackPartMetadataIndexFailedTest) {
  action_under_test->part_metadata = part_meta_factory->mock_part_metadata;
  action_under_test->bucket_metadata =
      bucket_meta_factory->mock_bucket_metadata;
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;
  ON_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillByDefault(Return(S3BucketMetadataState::missing));
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(_, _)).Times(1);
  action_under_test->rollback_create_part_meta_index_failed();
}

TEST_F(S3PostMultipartObjectTest, SaveUploadMetadataFailedTest) {
  action_under_test->add_task_rollback(
      std::bind(&S3PostMultipartObjectTest::func_callback_one, this));
  action_under_test->save_upload_metadata_failed();
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3PostMultipartObjectTest, RollbackUploadMetadataTest) {
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), remove(_, _))
      .Times(1);
  action_under_test->rollback_upload_metadata();
}

TEST_F(S3PostMultipartObjectTest, RollbackUploadMetadataFailTest) {
  action_under_test->part_metadata = part_meta_factory->mock_part_metadata;
  action_under_test->bucket_metadata =
      bucket_meta_factory->mock_bucket_metadata;
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;
  ON_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .WillByDefault(Return(S3ObjectMetadataState::missing));

  action_under_test->add_task_rollback(
      std::bind(&S3PostMultipartObjectTest::func_callback_one, this));
  action_under_test->rollback_upload_metadata_failed();
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3PostMultipartObjectTest, RollbackUploadMetadataFailTest2) {
  action_under_test->part_metadata = part_meta_factory->mock_part_metadata;
  action_under_test->bucket_metadata =
      bucket_meta_factory->mock_bucket_metadata;
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;
  ON_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .WillByDefault(Return(S3ObjectMetadataState::present));
  ON_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillByDefault(Return(S3BucketMetadataState::missing));

  action_under_test->add_task_rollback(
      std::bind(&S3PostMultipartObjectTest::func_callback_one, this));
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(_, _)).Times(1);
  action_under_test->rollback_upload_metadata_failed();
  EXPECT_EQ(0, call_count_one);
}

TEST_F(S3PostMultipartObjectTest, CreatePartMetadataIndexTest) {
  EXPECT_CALL(*(part_meta_factory->mock_part_metadata), create_index(_, _))
      .Times(AtLeast(1));
  action_under_test->create_part_meta_index();
}

TEST_F(S3PostMultipartObjectTest, SaveMultipartMetadataTest) {
  action_under_test->bucket_metadata =
      bucket_meta_factory->mock_bucket_metadata;
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;

  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), save(_, _))
      .Times(AtLeast(1));
  action_under_test->save_multipart_metadata();
  EXPECT_EQ(1, action_under_test->number_of_rollback_tasks());
}

TEST_F(S3PostMultipartObjectTest, SaveMultipartMetadataFailedTest) {
  action_under_test->add_task_rollback(
      std::bind(&S3PostMultipartObjectTest::func_callback_one, this));
  action_under_test->save_multipart_metadata_failed();
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3PostMultipartObjectTest, Send503ResponseToClientTest) {
  action_under_test->bucket_metadata =
      bucket_meta_factory->mock_bucket_metadata;
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;
  action_under_test->part_metadata = part_meta_factory->mock_part_metadata;
  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;
  action_under_test->object_metadata =
      object_meta_factory->mock_object_metadata;
  S3Option::get_instance()->set_is_s3_shutting_down(true);
  action_under_test->check_shutdown_and_rollback();
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(503, _)).Times(AtLeast(1));
  action_under_test->send_response_to_s3_client();
}

TEST_F(S3PostMultipartObjectTest, SendResponseToClientTest2) {
  action_under_test->bucket_metadata =
      bucket_meta_factory->mock_bucket_metadata;
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;
  action_under_test->part_metadata = part_meta_factory->mock_part_metadata;
  action_under_test->clovis_writer = clovis_writer_factory->mock_clovis_writer;
  action_under_test->object_metadata =
      object_meta_factory->mock_object_metadata;
  S3Option::get_instance()->set_is_s3_shutting_down(true);
  S3Option::get_instance()->set_is_s3_shutting_down(false);
  ON_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillByDefault(Return(S3BucketMetadataState::missing));
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(404, _)).Times(AtLeast(1));
  action_under_test->send_response_to_s3_client();

  ON_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillByDefault(Return(S3BucketMetadataState::failed));
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(500, _)).Times(AtLeast(1));
  action_under_test->send_response_to_s3_client();

  ON_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillByDefault(Return(S3BucketMetadataState::empty));
  ON_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .WillByDefault(Return(S3ObjectMetadataState::present));
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(403, _)).Times(AtLeast(1));
  action_under_test->send_response_to_s3_client();

  ON_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .WillByDefault(Return(S3ObjectMetadataState::empty));
  ON_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .WillByDefault(Return(S3ObjectMetadataState::saved));
  ON_CALL(*(part_meta_factory->mock_part_metadata), get_state())
      .WillByDefault(Return(S3PartMetadataState::store_created));
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(200, _)).Times(AtLeast(1));
  action_under_test->send_response_to_s3_client();
}
