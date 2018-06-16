/*
 * COPYRIGHT 2018 SEAGATE LLC
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
 * Original author:  Prashanth Vanaparthy   <prashanth.vanaparthy@seagate.com>
 * Original creation date: July-2018
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "mock_s3_bucket_metadata.h"
#include "mock_s3_clovis_wrapper.h"
#include "mock_s3_factory.h"
#include "mock_s3_request_object.h"
#include "s3_callback_test_helpers.h"
#include "s3_new_account_register_notify_action.h"
#include "s3_test_utils.h"

using ::testing::Eq;
using ::testing::Return;
using ::testing::Invoke;
using ::testing::_;
using ::testing::ReturnRef;
using ::testing::AtLeast;
using ::testing::DefaultValue;

class S3NewAccountRegisterNotifyActionTest : public testing::Test {
 protected:  // You should make the members protected s.t. they can be
             // accessed from sub-classes.
  S3NewAccountRegisterNotifyActionTest() {
    call_count_one = 0;
    evhtp_request_t *req = NULL;
    EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();
    ptr_mock_request =
        std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr);
    ptr_mock_request->set_account_id("account_test");
    clovis_kvs_writer_factory = std::make_shared<MockS3ClovisKVSWriterFactory>(
        ptr_mock_request, ptr_mock_s3_clovis_api);

    s3_account_user_idx_metadata_factory =
        std::make_shared<MockS3AccountUserIdxMetadataFactory>(ptr_mock_request);
    // ptr_mock_request->set_file_name("account_test");

    action_under_test.reset(new S3NewAccountRegisterNotifyAction(
        ptr_mock_request, ptr_mock_s3_clovis_api, clovis_kvs_writer_factory,
        s3_account_user_idx_metadata_factory));
  }

  std::shared_ptr<MockS3RequestObject> ptr_mock_request;
  std::shared_ptr<ClovisAPI> ptr_mock_s3_clovis_api;
  std::shared_ptr<MockS3ClovisKVSWriterFactory> clovis_kvs_writer_factory;
  std::shared_ptr<MockS3AccountUserIdxMetadataFactory>
      s3_account_user_idx_metadata_factory;
  std::shared_ptr<S3NewAccountRegisterNotifyAction> action_under_test;

  S3CallBack S3NewAccountRegisterNotifyAction_callbackobj;

  int call_count_one;

 public:
  void func_callback_one() { call_count_one += 1; }
};

TEST_F(S3NewAccountRegisterNotifyActionTest, Constructor) {
  struct m0_uint128 zero_oid = {0ULL, 0ULL};
  EXPECT_OID_EQ(zero_oid, action_under_test->bucket_list_index_oid);
  EXPECT_STREQ("index_salt_", action_under_test->collision_salt.c_str());
  EXPECT_EQ(0, action_under_test->collision_attempt_count);
}

TEST_F(S3NewAccountRegisterNotifyActionTest, ValidateRequestSuceess) {
  action_under_test->account_id_from_uri = "account_test";
  EXPECT_STREQ(action_under_test->account_id_from_uri.c_str(),
               ptr_mock_request->get_account_id().c_str());
  // Mock out the next calls on action.
  action_under_test->clear_tasks();
  action_under_test->add_task(std::bind(
      &S3NewAccountRegisterNotifyActionTest::func_callback_one, this));
  action_under_test->validate_request();
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3NewAccountRegisterNotifyActionTest, ValidateRequestFailed) {
  action_under_test->account_id_from_uri = "account_test123";
  EXPECT_STRNE(action_under_test->account_id_from_uri.c_str(),
               ptr_mock_request->get_account_id().c_str());

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(400, _)).Times(1);

  // Mock out the next calls on action.
  action_under_test->clear_tasks();
  action_under_test->add_task(std::bind(
      &S3NewAccountRegisterNotifyActionTest::func_callback_one, this));
  action_under_test->validate_request();

  EXPECT_EQ(0, call_count_one);
  EXPECT_STREQ("InvalidAccountForMgmtApi",
               action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3NewAccountRegisterNotifyActionTest, FetchBucketListIndexOid) {
  EXPECT_CALL(
      *(s3_account_user_idx_metadata_factory->mock_account_user_index_metadata),
      load(_, _)).Times(1);
  action_under_test->fetch_bucket_list_index_oid();
  EXPECT_TRUE(action_under_test->account_user_index_metadata != NULL);
}

TEST_F(S3NewAccountRegisterNotifyActionTest,
       ValidateFetchedBucketListIndexOidAlreadyPresent) {
  struct m0_uint128 oid = {0xffff, 0xffff};
  action_under_test->account_user_index_metadata =
      s3_account_user_idx_metadata_factory->mock_account_user_index_metadata;
  EXPECT_CALL(
      *(s3_account_user_idx_metadata_factory->mock_account_user_index_metadata),
      get_state())
      .Times(1)
      .WillOnce(Return(S3AccountUserIdxMetadataState::present));
  s3_account_user_idx_metadata_factory->mock_account_user_index_metadata
      ->set_bucket_list_index_oid(oid);
  EXPECT_CALL(*ptr_mock_request, send_response(201, _)).Times(AtLeast(1));

  action_under_test->validate_fetched_bucket_list_index_oid();
}

TEST_F(S3NewAccountRegisterNotifyActionTest,
       ValidateFetchedBucketListIndexOidAlreadyPresentZeroOid) {
  struct m0_uint128 zero_oid = {0ULL, 0ULL};
  action_under_test->account_user_index_metadata =
      s3_account_user_idx_metadata_factory->mock_account_user_index_metadata;

  EXPECT_CALL(
      *(s3_account_user_idx_metadata_factory->mock_account_user_index_metadata),
      get_state())
      .Times(1)
      .WillOnce(Return(S3AccountUserIdxMetadataState::present));

  // Mock out the next calls on action.
  action_under_test->clear_tasks();
  action_under_test->add_task(std::bind(
      &S3NewAccountRegisterNotifyActionTest::func_callback_one, this));
  // set the OID
  s3_account_user_idx_metadata_factory->mock_account_user_index_metadata
      ->set_bucket_list_index_oid(zero_oid);
  action_under_test->validate_fetched_bucket_list_index_oid();
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3NewAccountRegisterNotifyActionTest,
       ValidateFetchedBucketListIndexOidMissing) {
  action_under_test->account_user_index_metadata =
      s3_account_user_idx_metadata_factory->mock_account_user_index_metadata;
  EXPECT_CALL(
      *(s3_account_user_idx_metadata_factory->mock_account_user_index_metadata),
      get_state())
      .Times(2)
      .WillRepeatedly(Return(S3AccountUserIdxMetadataState::missing));

  // Mock out the next calls on action.
  action_under_test->clear_tasks();
  action_under_test->add_task(std::bind(
      &S3NewAccountRegisterNotifyActionTest::func_callback_one, this));
  action_under_test->collision_attempt_count = 0;
  action_under_test->validate_fetched_bucket_list_index_oid();
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3NewAccountRegisterNotifyActionTest,
       CreateBucketListIndexCollisionCount0) {
  action_under_test->collision_attempt_count = 0;
  EXPECT_CALL(*(clovis_kvs_writer_factory->mock_clovis_kvs_writer),
              create_index(_, _, _)).Times(1);
  action_under_test->create_bucket_list_index();
}

TEST_F(S3NewAccountRegisterNotifyActionTest, CreateBucketListIndexSuccessful) {
  struct m0_uint128 oid = {0xffff, 0xffff};
  action_under_test->clovis_kv_writer =
      clovis_kvs_writer_factory->mock_clovis_kvs_writer;
  action_under_test->clovis_kv_writer->oid_list.push_back(oid);

  // Mock out the next calls on action.
  action_under_test->clear_tasks();
  action_under_test->add_task(std::bind(
      &S3NewAccountRegisterNotifyActionTest::func_callback_one, this));

  action_under_test->create_bucket_list_index_successful();
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3NewAccountRegisterNotifyActionTest, CreateBucketListIndexFailed) {
  action_under_test->clovis_kv_writer =
      clovis_kvs_writer_factory->mock_clovis_kvs_writer;
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(500, _)).Times(AtLeast(1));
  EXPECT_CALL(*(clovis_kvs_writer_factory->mock_clovis_kvs_writer), get_state())
      .Times(1)
      .WillRepeatedly(Return(S3ClovisKVSWriterOpState::failed));
  action_under_test->create_bucket_list_index_failed();
}

TEST_F(S3NewAccountRegisterNotifyActionTest, HandleCollision) {
  action_under_test->collision_attempt_count = 1;
  std::string base_index_name = "ACCOUNTUSER/s3accountid";
  action_under_test->salted_bucket_list_index_name =
      "ACCOUNTUSER/s3accountid_salt_0";
  action_under_test->handle_collision(
      base_index_name, action_under_test->salted_bucket_list_index_name,
      std::bind(&S3CallBack::on_failed,
                &S3NewAccountRegisterNotifyAction_callbackobj));
  EXPECT_STREQ("ACCOUNTUSER/s3accountidindex_salt_1",
               action_under_test->salted_bucket_list_index_name.c_str());
  EXPECT_EQ(2, action_under_test->collision_attempt_count);
  EXPECT_TRUE(S3NewAccountRegisterNotifyAction_callbackobj.fail_called);
}

TEST_F(S3NewAccountRegisterNotifyActionTest,
       HandleCollisionMaxAttemptExceeded) {
  std::string base_index_name = "ACCOUNTUSER/s3account";
  action_under_test->salted_bucket_list_index_name =
      "ACCOUNTUSER/s3accountid_salt_0";
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(500, _)).Times(AtLeast(1));

  action_under_test->collision_attempt_count = MAX_COLLISION_RETRY_COUNT + 1;
  action_under_test->handle_collision(
      base_index_name, action_under_test->salted_bucket_list_index_name,
      std::bind(&S3CallBack::on_success,
                &S3NewAccountRegisterNotifyAction_callbackobj));
  EXPECT_EQ(MAX_COLLISION_RETRY_COUNT + 1,
            action_under_test->collision_attempt_count);
}

TEST_F(S3NewAccountRegisterNotifyActionTest, RegeneratedNewIndexName) {
  std::string base_index_name = "ACCOUNTUSER/1234";
  action_under_test->salted_bucket_list_index_name = "ACCOUNTUSER/s3accountid";
  action_under_test->collision_attempt_count = 2;
  action_under_test->regenerate_new_index_name(
      base_index_name, action_under_test->salted_bucket_list_index_name);
  EXPECT_STREQ("ACCOUNTUSER/1234index_salt_2",
               action_under_test->salted_bucket_list_index_name.c_str());
}

TEST_F(S3NewAccountRegisterNotifyActionTest, SaveBucketListIndexOid) {
  action_under_test->bucket_list_index_oid = {0xffff, 0xffff};
  action_under_test->account_user_index_metadata =
      s3_account_user_idx_metadata_factory->mock_account_user_index_metadata;

  EXPECT_CALL(
      *(s3_account_user_idx_metadata_factory->mock_account_user_index_metadata),
      save(_, _)).Times(1);
  action_under_test->save_bucket_list_index_oid();
  EXPECT_OID_EQ(action_under_test->bucket_list_index_oid,
                action_under_test->account_user_index_metadata
                    ->get_bucket_list_index_oid());
}

TEST_F(S3NewAccountRegisterNotifyActionTest, SendResponseToInternalError) {
  action_under_test->set_s3_error("InternalError");
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(500, _)).Times(AtLeast(1));
  action_under_test->send_response_to_s3_client();
}

TEST_F(S3NewAccountRegisterNotifyActionTest, SendSuccessResponse) {
  EXPECT_CALL(*ptr_mock_request, send_response(201, _)).Times(AtLeast(1));
  action_under_test->send_response_to_s3_client();
}