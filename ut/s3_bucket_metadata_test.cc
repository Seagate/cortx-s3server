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
 * Original creation date: 10-May-2017
 */

#include <json/json.h>
#include <memory>

#include "mock_s3_factory.h"
#include "mock_s3_request_object.h"
#include "s3_bucket_metadata.h"
#include "s3_callback_test_helpers.h"
#include "s3_common.h"
#include "s3_test_utils.h"

using ::testing::Eq;
using ::testing::Return;
using ::testing::Invoke;
using ::testing::_;
using ::testing::ReturnRef;
using ::testing::AtLeast;
using ::testing::DefaultValue;

#define DEFAULT_ACL_STR                                                 \
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<AccessControlPolicy "   \
  "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">\n  <Owner>\n    " \
  "<ID></ID>\n      <DisplayName></DisplayName>\n  </Owner>\n  "        \
  "<AccessControlList>\n    <Grant>\n      <Grantee "                   \
  "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "            \
  "xsi:type=\"CanonicalUser\">\n        <ID></ID>\n        "            \
  "<DisplayName></DisplayName>\n      </Grantee>\n      "               \
  "<Permission>FULL_CONTROL</Permission>\n    </Grant>\n  "             \
  "</AccessControlList>\n</AccessControlPolicy>\n"

#define DUMMY_POLICY_STR                                                \
  "{\n \"Id\": \"Policy1462526893193\",\n \"Statement\": [\n \"Sid\": " \
  "\"Stmt1462526862401\", \n }"

#define DUMMY_ACL_STR "<Owner>\n<ID></ID>\n</Owner>"

class S3BucketMetadataTest : public testing::Test {
 protected:
  S3BucketMetadataTest() {
    evhtp_request_t *req = NULL;
    EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();
    call_count_one = 0;
    ptr_mock_request =
        std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr);

    clovis_kvs_reader_factory = std::make_shared<MockS3ClovisKVSReaderFactory>(
        ptr_mock_request, ptr_mock_s3_clovis_api);

    clovis_kvs_writer_factory = std::make_shared<MockS3ClovisKVSWriterFactory>(
        ptr_mock_request, ptr_mock_s3_clovis_api);

    s3_account_user_idx_metadata_factory =
        std::make_shared<MockS3AccountUserIdxMetadataFactory>(ptr_mock_request);

    ptr_mock_request->set_account_name("s3account");
    ptr_mock_request->set_account_id("s3accountid");
    ptr_mock_request->set_user_name("s3user");
    ptr_mock_request->set_user_id("s3userid");

    action_under_test.reset(new S3BucketMetadata(
        ptr_mock_request, ptr_mock_s3_clovis_api, clovis_kvs_reader_factory,
        clovis_kvs_writer_factory, s3_account_user_idx_metadata_factory));
  }

  std::shared_ptr<MockS3RequestObject> ptr_mock_request;
  std::shared_ptr<ClovisAPI> ptr_mock_s3_clovis_api;
  std::shared_ptr<MockS3ClovisKVSReaderFactory> clovis_kvs_reader_factory;
  std::shared_ptr<MockS3ClovisKVSWriterFactory> clovis_kvs_writer_factory;
  std::shared_ptr<MockS3AccountUserIdxMetadataFactory>
      s3_account_user_idx_metadata_factory;
  S3CallBack s3bucketmetadata_callbackobj;
  std::shared_ptr<S3BucketMetadata> action_under_test;
  int call_count_one;

 public:
  void func_callback_one() { call_count_one += 1; }
};

TEST_F(S3BucketMetadataTest, ConstructorTest) {
  struct m0_uint128 zero_oid = {0ULL, 0ULL};
  EXPECT_STREQ("s3user", action_under_test->user_name.c_str());
  EXPECT_STREQ("s3account", action_under_test->account_name.c_str());
  EXPECT_STREQ("ACCOUNTUSER/s3accountid",
               action_under_test->salted_index_name.c_str());
  EXPECT_STREQ("", action_under_test->bucket_policy.c_str());
  EXPECT_STREQ("index_salt_", action_under_test->collision_salt.c_str());
  EXPECT_EQ(0, action_under_test->collision_attempt_count);
  EXPECT_OID_EQ(zero_oid, action_under_test->bucket_list_index_oid);
  EXPECT_OID_EQ(zero_oid, action_under_test->object_list_index_oid);
  EXPECT_OID_EQ(zero_oid, action_under_test->multipart_index_oid);
  EXPECT_STREQ(
      "", action_under_test->system_defined_attribute["Owner-User"].c_str());
  EXPECT_STREQ(
      "", action_under_test->system_defined_attribute["Owner-User-id"].c_str());
  EXPECT_STREQ(
      "", action_under_test->system_defined_attribute["Owner-Account"].c_str());
  EXPECT_STREQ(
      "",
      action_under_test->system_defined_attribute["Owner-Account-id"].c_str());
  std::string date = action_under_test->system_defined_attribute["Date"];
  EXPECT_STREQ("Z", date.substr(date.length() - 1).c_str());
  EXPECT_STREQ("us-west-2",
               action_under_test->system_defined_attribute["LocationConstraint"]
                   .c_str());
}

TEST_F(S3BucketMetadataTest, GetSystemAttributesTest) {
  // EXPECT_STREQ("mybucket", action_under_test->get_bucket_name().c_str());
  EXPECT_STRNE("", action_under_test->get_creation_time().c_str());
  action_under_test->system_defined_attribute["Owner-User-id"] = "s3owner";
  EXPECT_STREQ("s3owner", action_under_test->get_owner_id().c_str());
  EXPECT_STREQ("us-west-2",
               action_under_test->get_location_constraint().c_str());
  action_under_test->system_defined_attribute["Owner-User"] = "s3user";
  EXPECT_STREQ("s3user", action_under_test->get_owner_name().c_str());
}

TEST_F(S3BucketMetadataTest, GetSetOIDsPolicyAndLocation) {
  struct m0_uint128 oid = {0x1ffff, 0x1fff};
  action_under_test->bucket_list_index_oid = {0xfff, 0xfff};
  action_under_test->multipart_index_oid = {0x1fff, 0xff1};
  action_under_test->object_list_index_oid = {0xff, 0xff};
  EXPECT_OID_EQ(action_under_test->get_bucket_list_index_oid(),
                action_under_test->bucket_list_index_oid);
  EXPECT_OID_EQ(action_under_test->get_multipart_index_oid(),
                action_under_test->multipart_index_oid);
  EXPECT_OID_EQ(action_under_test->get_object_list_index_oid(),
                action_under_test->object_list_index_oid);
  action_under_test->object_list_index_oid_u_hi_str = "1234-1234-1234";
  EXPECT_STREQ("1234-1234-1234",
               action_under_test->get_object_list_index_oid_u_hi_str().c_str());
  action_under_test->object_list_index_oid_u_lo_str = "232323-232323";
  EXPECT_STREQ("232323-232323",
               action_under_test->get_object_list_index_oid_u_lo_str().c_str());

  action_under_test->set_bucket_list_index_oid(oid);
  EXPECT_OID_EQ(action_under_test->bucket_list_index_oid, oid);

  action_under_test->set_multipart_index_oid(oid);
  EXPECT_OID_EQ(action_under_test->multipart_index_oid, oid);

  action_under_test->set_object_list_index_oid(oid);
  EXPECT_OID_EQ(action_under_test->object_list_index_oid, oid);

  action_under_test->set_location_constraint("us-east");
  EXPECT_STREQ("us-east",
               action_under_test->system_defined_attribute["LocationConstraint"]
                   .c_str());
}

TEST_F(S3BucketMetadataTest, SetBucketPolicy) {
  std::string policy = DUMMY_POLICY_STR;
  action_under_test->setpolicy(policy);
  EXPECT_STREQ(DUMMY_POLICY_STR, action_under_test->bucket_policy.c_str());
}

TEST_F(S3BucketMetadataTest, DeletePolicy) {
  action_under_test->deletepolicy();
  EXPECT_STREQ("", action_under_test->bucket_policy.c_str());
}

TEST_F(S3BucketMetadataTest, SetAcl) {
  char expected_str[] =
      "<?xml version=\"1.0\" "
      "encoding=\"UTF-8\"?>\n<Owner>\n<ID></ID>\n\n<DisplayName></"
      "DisplayName>\n</Owner>";
  std::string acl = DUMMY_ACL_STR;
  action_under_test->setacl(acl);
  EXPECT_STREQ(expected_str,
               action_under_test->bucket_ACL.get_xml_str().c_str());
}

TEST_F(S3BucketMetadataTest, AddSystemAttribute) {
  action_under_test->add_system_attribute("LocationConstraint", "us-east");
  EXPECT_STREQ("us-east",
               action_under_test->system_defined_attribute["LocationConstraint"]
                   .c_str());
}

TEST_F(S3BucketMetadataTest, AddUserDefinedAttribute) {
  action_under_test->add_user_defined_attribute("Etag", "1234");
  EXPECT_STREQ("1234",
               action_under_test->user_defined_attribute["Etag"].c_str());
}

TEST_F(S3BucketMetadataTest, Load) {
  EXPECT_CALL(
      *(s3_account_user_idx_metadata_factory->mock_account_user_index_metadata),
      load(_, _))
      .Times(1);
  action_under_test->load(
      std::bind(&S3CallBack::on_success, &s3bucketmetadata_callbackobj),
      std::bind(&S3CallBack::on_failed, &s3bucketmetadata_callbackobj));
  EXPECT_EQ(S3BucketMetadataState::fetching, action_under_test->state);
}

TEST_F(S3BucketMetadataTest, FetchBucketListIndexOid) {
  EXPECT_CALL(
      *(s3_account_user_idx_metadata_factory->mock_account_user_index_metadata),
      load(_, _))
      .Times(1);
  action_under_test->fetch_bucket_list_index_oid();
  EXPECT_TRUE(action_under_test->account_user_index_metadata != NULL);
}

TEST_F(S3BucketMetadataTest, FetchBucketListIndexOIDSuccessStateIsSaving) {
  action_under_test->account_user_index_metadata =
      s3_account_user_idx_metadata_factory->mock_account_user_index_metadata;
  action_under_test->state = S3BucketMetadataState::saving;
  EXPECT_CALL(*(clovis_kvs_writer_factory->mock_clovis_kvs_writer),
              put_keyval(_, _, _, _, _))
      .Times(1);
  action_under_test->fetch_bucket_list_index_oid_success();
}

TEST_F(S3BucketMetadataTest, FetchBucketListIndexOIDSuccessStateIsFetching) {
  action_under_test->account_user_index_metadata =
      s3_account_user_idx_metadata_factory->mock_account_user_index_metadata;
  action_under_test->state = S3BucketMetadataState::fetching;
  EXPECT_CALL(*(clovis_kvs_reader_factory->mock_clovis_kvs_reader),
              get_keyval(_, "", _, _))
      .Times(1);
  action_under_test->fetch_bucket_list_index_oid_success();
}

TEST_F(S3BucketMetadataTest, FetchBucketListIndexOIDSuccessStateIsDeleting) {
  action_under_test->account_user_index_metadata =
      s3_account_user_idx_metadata_factory->mock_account_user_index_metadata;
  action_under_test->state = S3BucketMetadataState::deleting;
  EXPECT_CALL(*(clovis_kvs_writer_factory->mock_clovis_kvs_writer),
              delete_keyval(_, _, _, _))
      .Times(1);
  action_under_test->fetch_bucket_list_index_oid_success();
}

TEST_F(S3BucketMetadataTest, FetchBucketListIndexOIDFailedStateIsSaving) {
  action_under_test->account_user_index_metadata =
      s3_account_user_idx_metadata_factory->mock_account_user_index_metadata;
  EXPECT_CALL(
      *(s3_account_user_idx_metadata_factory->mock_account_user_index_metadata),
      get_state())
      .Times(1)
      .WillRepeatedly(Return(S3AccountUserIdxMetadataState::missing));
  action_under_test->state = S3BucketMetadataState::saving;
  action_under_test->collision_attempt_count = 0;
  EXPECT_CALL(*(clovis_kvs_writer_factory->mock_clovis_kvs_writer),
              create_index(_, _, _))
      .Times(1);
  action_under_test->fetch_bucket_list_index_oid_failed();
}

TEST_F(S3BucketMetadataTest,
       FetchBucketListIndexOIDFailedIndexMissingStateSaved) {
  action_under_test->handler_on_failed =
      std::bind(&S3CallBack::on_success, &s3bucketmetadata_callbackobj);
  action_under_test->account_user_index_metadata =
      s3_account_user_idx_metadata_factory->mock_account_user_index_metadata;
  EXPECT_CALL(
      *(s3_account_user_idx_metadata_factory->mock_account_user_index_metadata),
      get_state())
      .Times(1)
      .WillRepeatedly(Return(S3AccountUserIdxMetadataState::missing));
  action_under_test->state = S3BucketMetadataState::saved;
  action_under_test->fetch_bucket_list_index_oid_failed();
  EXPECT_EQ(S3BucketMetadataState::missing, action_under_test->state);
  EXPECT_TRUE(s3bucketmetadata_callbackobj.success_called);
}

TEST_F(S3BucketMetadataTest, FetchBucketListIndexOIDFailedStateIsFetching) {
  action_under_test->account_user_index_metadata =
      s3_account_user_idx_metadata_factory->mock_account_user_index_metadata;
  EXPECT_CALL(
      *(s3_account_user_idx_metadata_factory->mock_account_user_index_metadata),
      get_state())
      .Times(1)
      .WillRepeatedly(Return(S3AccountUserIdxMetadataState::missing));
  action_under_test->state = S3BucketMetadataState::fetching;
  action_under_test->collision_attempt_count = 0;
  EXPECT_CALL(*(clovis_kvs_writer_factory->mock_clovis_kvs_writer),
              create_index(_, _, _))
      .Times(1);
  action_under_test->fetch_bucket_list_index_oid_failed();
}

TEST_F(S3BucketMetadataTest, FetchBucketListIndexOIDFailedIndexFailed) {
  action_under_test->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &s3bucketmetadata_callbackobj);
  action_under_test->account_user_index_metadata =
      s3_account_user_idx_metadata_factory->mock_account_user_index_metadata;
  EXPECT_CALL(
      *(s3_account_user_idx_metadata_factory->mock_account_user_index_metadata),
      get_state())
      .Times(1)
      .WillRepeatedly(Return(S3AccountUserIdxMetadataState::failed));
  action_under_test->fetch_bucket_list_index_oid_failed();
  EXPECT_TRUE(s3bucketmetadata_callbackobj.fail_called);
  EXPECT_EQ(S3BucketMetadataState::failed, action_under_test->state);
}

TEST_F(S3BucketMetadataTest, LoadBucketInfo) {
  EXPECT_CALL(*(clovis_kvs_reader_factory->mock_clovis_kvs_reader),
              get_keyval(_, "", _, _))
      .Times(1);
  action_under_test->load_bucket_info();
}

TEST_F(S3BucketMetadataTest, LoadBucketInfoFailedJsonParsingFailed) {
  action_under_test->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &s3bucketmetadata_callbackobj);
  action_under_test->json_parsing_error = true;
  action_under_test->load_bucket_info_failed();
  EXPECT_TRUE(s3bucketmetadata_callbackobj.fail_called);
  EXPECT_EQ(S3BucketMetadataState::failed, action_under_test->state);
}

TEST_F(S3BucketMetadataTest, LoadBucketInfoFailedMetadataMissing) {
  action_under_test->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &s3bucketmetadata_callbackobj);
  action_under_test->clovis_kv_reader =
      clovis_kvs_reader_factory->mock_clovis_kvs_reader;
  EXPECT_CALL(*(clovis_kvs_reader_factory->mock_clovis_kvs_reader), get_state())
      .Times(1)
      .WillRepeatedly(Return(S3ClovisKVSReaderOpState::missing));
  action_under_test->load_bucket_info_failed();
  EXPECT_TRUE(s3bucketmetadata_callbackobj.fail_called);
  EXPECT_EQ(S3BucketMetadataState::missing, action_under_test->state);
}

TEST_F(S3BucketMetadataTest, LoadBucketInfoFailedMetadataFailed) {
  action_under_test->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &s3bucketmetadata_callbackobj);
  action_under_test->clovis_kv_reader =
      clovis_kvs_reader_factory->mock_clovis_kvs_reader;
  EXPECT_CALL(*(clovis_kvs_reader_factory->mock_clovis_kvs_reader), get_state())
      .Times(1)
      .WillRepeatedly(Return(S3ClovisKVSReaderOpState::failed));
  action_under_test->load_bucket_info_failed();
  EXPECT_TRUE(s3bucketmetadata_callbackobj.fail_called);
  EXPECT_EQ(S3BucketMetadataState::failed, action_under_test->state);
}

TEST_F(S3BucketMetadataTest, SaveMeatdataMissingIndexOID) {
  action_under_test->bucket_list_index_oid = {0ULL, 0ULL};
  EXPECT_CALL(
      *(s3_account_user_idx_metadata_factory->mock_account_user_index_metadata),
      load(_, _))
      .Times(1);
  action_under_test->save(
      std::bind(&S3CallBack::on_success, &s3bucketmetadata_callbackobj),
      std::bind(&S3CallBack::on_failed, &s3bucketmetadata_callbackobj));
  EXPECT_EQ(S3BucketMetadataState::saving, action_under_test->state);
}

TEST_F(S3BucketMetadataTest, SaveMeatdataIndexOIDPresent) {
  action_under_test->bucket_list_index_oid = {0x111f, 0xffff};
  EXPECT_CALL(*(clovis_kvs_writer_factory->mock_clovis_kvs_writer),
              put_keyval(_, _, _, _, _))
      .Times(1);
  action_under_test->save(
      std::bind(&S3CallBack::on_success, &s3bucketmetadata_callbackobj),
      std::bind(&S3CallBack::on_failed, &s3bucketmetadata_callbackobj));
  EXPECT_EQ(S3BucketMetadataState::saving, action_under_test->state);
}

TEST_F(S3BucketMetadataTest, CreateBucketListIndexCollisionCount0) {
  action_under_test->collision_attempt_count = 0;
  EXPECT_CALL(*(clovis_kvs_writer_factory->mock_clovis_kvs_writer),
              create_index(_, _, _))
      .Times(1);
  action_under_test->create_bucket_list_index();
  EXPECT_EQ(S3BucketMetadataState::missing, action_under_test->state);
}

TEST_F(S3BucketMetadataTest, CreateBucketListIndexCollisionCount1) {
  action_under_test->collision_attempt_count = 1;
  action_under_test->clovis_kv_writer =
      clovis_kvs_writer_factory->mock_clovis_kvs_writer;
  EXPECT_CALL(*(clovis_kvs_writer_factory->mock_clovis_kvs_writer),
              create_index(_, _, _))
      .Times(1);
  action_under_test->create_bucket_list_index();
  EXPECT_EQ(S3BucketMetadataState::missing, action_under_test->state);
}

TEST_F(S3BucketMetadataTest, CreateBucketListIndexSuccessful) {
  struct m0_uint128 oid = {0xffff, 0xffff};
  action_under_test->clovis_kv_writer =
      clovis_kvs_writer_factory->mock_clovis_kvs_writer;
  action_under_test->clovis_kv_writer->oid_list.push_back(oid);

  action_under_test->account_user_index_metadata =
      s3_account_user_idx_metadata_factory->mock_account_user_index_metadata;

  EXPECT_CALL(
      *(s3_account_user_idx_metadata_factory->mock_account_user_index_metadata),
      save(_, _))
      .Times(1);
  action_under_test->create_bucket_list_index_successful();
  EXPECT_OID_EQ(oid, action_under_test->account_user_index_metadata
                         ->get_bucket_list_index_oid());
}

TEST_F(S3BucketMetadataTest, CreateBucketListIndexFailedCollisionHappened) {
  action_under_test->clovis_kv_writer =
      clovis_kvs_writer_factory->mock_clovis_kvs_writer;
  action_under_test->collision_attempt_count = 1;
  EXPECT_CALL(*(clovis_kvs_writer_factory->mock_clovis_kvs_writer), get_state())
      .Times(1)
      .WillRepeatedly(Return(S3ClovisKVSWriterOpState::exists));

  EXPECT_CALL(*(clovis_kvs_writer_factory->mock_clovis_kvs_writer),
              create_index(_, _, _))
      .Times(1);

  action_under_test->create_bucket_list_index_failed();
  EXPECT_EQ(2, action_under_test->collision_attempt_count);
}

TEST_F(S3BucketMetadataTest, CreateBucketListIndexFailed) {
  action_under_test->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &s3bucketmetadata_callbackobj);
  action_under_test->clovis_kv_writer =
      clovis_kvs_writer_factory->mock_clovis_kvs_writer;
  EXPECT_CALL(*(clovis_kvs_writer_factory->mock_clovis_kvs_writer), get_state())
      .Times(1)
      .WillRepeatedly(Return(S3ClovisKVSWriterOpState::failed));
  action_under_test->create_bucket_list_index_failed();
  EXPECT_TRUE(s3bucketmetadata_callbackobj.fail_called);
}

TEST_F(S3BucketMetadataTest, HandleCollision) {
  action_under_test->collision_attempt_count = 1;
  action_under_test->clovis_kv_writer =
      clovis_kvs_writer_factory->mock_clovis_kvs_writer;
  EXPECT_CALL(*(clovis_kvs_writer_factory->mock_clovis_kvs_writer),
              create_index(_, _, _))
      .Times(1);
  action_under_test->handle_collision();
  EXPECT_STREQ("ACCOUNTUSER/s3accountidindex_salt_1",
               action_under_test->salted_index_name.c_str());
  EXPECT_EQ(2, action_under_test->collision_attempt_count);
}

TEST_F(S3BucketMetadataTest, HandleCollisionMaxAttemptExceeded) {
  action_under_test->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &s3bucketmetadata_callbackobj);
  action_under_test->collision_attempt_count = MAX_COLLISION_RETRY_COUNT + 1;
  action_under_test->handle_collision();
  EXPECT_EQ(S3BucketMetadataState::failed, action_under_test->state);
  EXPECT_EQ(MAX_COLLISION_RETRY_COUNT + 1,
            action_under_test->collision_attempt_count);
}

TEST_F(S3BucketMetadataTest, RegeneratedNewIndexName) {
  action_under_test->collision_attempt_count = 2;
  action_under_test->regenerate_new_index_name();
  EXPECT_STREQ("ACCOUNTUSER/s3accountidindex_salt_2",
               action_under_test->salted_index_name.c_str());
}

TEST_F(S3BucketMetadataTest, SaveBucketListIndexOid) {
  action_under_test->bucket_list_index_oid = {0xffff, 0xffff};
  action_under_test->account_user_index_metadata =
      s3_account_user_idx_metadata_factory->mock_account_user_index_metadata;

  EXPECT_CALL(
      *(s3_account_user_idx_metadata_factory->mock_account_user_index_metadata),
      save(_, _))
      .Times(1);
  action_under_test->save_bucket_list_index_oid();
  EXPECT_OID_EQ(action_under_test->bucket_list_index_oid,
                action_under_test->account_user_index_metadata
                    ->get_bucket_list_index_oid());
}

TEST_F(S3BucketMetadataTest, SaveBucketListIndexOidSucessfulStateSaving) {
  action_under_test->state = S3BucketMetadataState::saving;
  action_under_test->clovis_kv_writer =
      clovis_kvs_writer_factory->mock_clovis_kvs_writer;
  EXPECT_CALL(*(clovis_kvs_writer_factory->mock_clovis_kvs_writer),
              put_keyval(_, _, _, _, _))
      .Times(1);

  action_under_test->save_bucket_list_index_oid_successful();
}

TEST_F(S3BucketMetadataTest, SaveBucketListIndexOidSucessful) {
  action_under_test->handler_on_success =
      std::bind(&S3CallBack::on_success, &s3bucketmetadata_callbackobj);
  action_under_test->state = S3BucketMetadataState::saved;
  action_under_test->save_bucket_list_index_oid_successful();
  EXPECT_TRUE(s3bucketmetadata_callbackobj.success_called);
}

TEST_F(S3BucketMetadataTest, SaveBucketListIndexOIDFailed) {
  action_under_test->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &s3bucketmetadata_callbackobj);
  action_under_test->save_bucket_list_index_oid_failed();
  EXPECT_TRUE(s3bucketmetadata_callbackobj.fail_called);
}

TEST_F(S3BucketMetadataTest, SaveBucketInfo) {
  action_under_test->clovis_kv_writer =
      clovis_kvs_writer_factory->mock_clovis_kvs_writer;
  EXPECT_CALL(*(clovis_kvs_writer_factory->mock_clovis_kvs_writer),
              put_keyval(_, _, _, _, _))
      .Times(1);

  action_under_test->save_bucket_info();
  EXPECT_STREQ(
      "s3user",
      action_under_test->system_defined_attribute["Owner-User"].c_str());
  EXPECT_STREQ(
      "s3userid",
      action_under_test->system_defined_attribute["Owner-User-id"].c_str());
  EXPECT_STREQ(
      "s3account",
      action_under_test->system_defined_attribute["Owner-Account"].c_str());
  EXPECT_STREQ(
      "s3accountid",
      action_under_test->system_defined_attribute["Owner-Account-id"].c_str());
}

TEST_F(S3BucketMetadataTest, SaveBucketInfoSuccess) {
  action_under_test->handler_on_success =
      std::bind(&S3CallBack::on_success, &s3bucketmetadata_callbackobj);
  action_under_test->save_bucket_info_successful();
  EXPECT_TRUE(s3bucketmetadata_callbackobj.success_called);
  EXPECT_EQ(S3BucketMetadataState::saved, action_under_test->state);
}

TEST_F(S3BucketMetadataTest, SaveBucketInfoFailed) {
  action_under_test->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &s3bucketmetadata_callbackobj);
  action_under_test->save_bucket_info_failed();
  EXPECT_TRUE(s3bucketmetadata_callbackobj.fail_called);
  EXPECT_EQ(S3BucketMetadataState::failed, action_under_test->state);
}

TEST_F(S3BucketMetadataTest, RemovePresentMetadata) {
  action_under_test->state = S3BucketMetadataState::present;
  EXPECT_CALL(*(clovis_kvs_writer_factory->mock_clovis_kvs_writer),
              delete_keyval(_, _, _, _))
      .Times(1);

  action_under_test->remove(
      std::bind(&S3CallBack::on_success, &s3bucketmetadata_callbackobj),
      std::bind(&S3CallBack::on_failed, &s3bucketmetadata_callbackobj));
  EXPECT_TRUE(action_under_test->handler_on_success != NULL);
  EXPECT_TRUE(action_under_test->handler_on_failed != NULL);
}

TEST_F(S3BucketMetadataTest, RemoveAfterFetchingBucketListIndexOID) {
  action_under_test->state = S3BucketMetadataState::missing;
  action_under_test->account_user_index_metadata =
      s3_account_user_idx_metadata_factory->mock_account_user_index_metadata;

  EXPECT_CALL(
      *(s3_account_user_idx_metadata_factory->mock_account_user_index_metadata),
      load(_, _))
      .Times(1);
  action_under_test->remove(
      std::bind(&S3CallBack::on_success, &s3bucketmetadata_callbackobj),
      std::bind(&S3CallBack::on_failed, &s3bucketmetadata_callbackobj));

  EXPECT_EQ(S3BucketMetadataState::deleting, action_under_test->state);
}

TEST_F(S3BucketMetadataTest, RemoveBucketInfo) {
  action_under_test->clovis_kv_writer =
      clovis_kvs_writer_factory->mock_clovis_kvs_writer;
  EXPECT_CALL(*(clovis_kvs_writer_factory->mock_clovis_kvs_writer),
              delete_keyval(_, _, _, _))
      .Times(1);
  action_under_test->remove_bucket_info();
}

TEST_F(S3BucketMetadataTest, RemoveBucketInfoSuccessful) {
  action_under_test->handler_on_success =
      std::bind(&S3CallBack::on_success, &s3bucketmetadata_callbackobj);

  action_under_test->remove_bucket_info_successful();
  EXPECT_EQ(S3BucketMetadataState::deleted, action_under_test->state);
  EXPECT_TRUE(s3bucketmetadata_callbackobj.success_called);
}

TEST_F(S3BucketMetadataTest, RemoveBucketInfoFailed) {
  action_under_test->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &s3bucketmetadata_callbackobj);

  action_under_test->remove_bucket_info_failed();
  EXPECT_EQ(S3BucketMetadataState::failed, action_under_test->state);
  EXPECT_TRUE(s3bucketmetadata_callbackobj.fail_called);
}

TEST_F(S3BucketMetadataTest, CreateDefaultAcl) {
  EXPECT_STREQ(DEFAULT_ACL_STR,
               action_under_test->create_default_acl().c_str());
}

TEST_F(S3BucketMetadataTest, ToJson) {
  std::string json_str = action_under_test->to_json();
  Json::Value newroot;
  Json::Reader reader;
  bool parsingSuccessful = reader.parse(json_str, newroot);
  EXPECT_TRUE(parsingSuccessful);
}

TEST_F(S3BucketMetadataTest, FromJson) {
  struct m0_uint128 zero_oid = {0ULL, 0ULL};
  EXPECT_OID_EQ(zero_oid, action_under_test->multipart_index_oid);
  std::string json_str =
      "{\"ACL\":\"PD94+Cg==\",\"Bucket-Name\":\"seagatebucket\",\"System-"
      "Defined\":{\"Date\":\"2016-10-18T16:01:00.000Z\",\"Owner-Account\":\"s3_"
      "test\",\"Owner-Account-id\":\"s3accountid\",\"Owner-User\":\"tester\",\""
      "Owner-User-id\":\"s3userid\"},\"mero_multipart_index_oid_u_hi\":\""
      "g1qTetGfvWk=\",\"mero_multipart_index_oid_u_lo\":\"lvH6Q65xFAI=\","
      "\"mero_object_list_index_oid_u_hi\":\"AAAAAAAAAAA=\","
      "\"mero_object_list_index_oid_u_lo\":\"AAAAAAAAAAA=\"}";

  action_under_test->from_json(json_str);
  EXPECT_STREQ("seagatebucket", action_under_test->bucket_name.c_str());
  EXPECT_STREQ("tester", action_under_test->user_name.c_str());
  EXPECT_STREQ("s3_test", action_under_test->account_name.c_str());
  EXPECT_STREQ("s3accountid", action_under_test->account_id.c_str());
  EXPECT_OID_NE(zero_oid, action_under_test->multipart_index_oid);
}

TEST_F(S3BucketMetadataTest, GetEncodedBucketAcl) {
  std::string json_str = "{\"ACL\":\"PD94bg==\"}";

  action_under_test->from_json(json_str);
  EXPECT_STREQ("PD94bg==", action_under_test->get_encoded_bucket_acl().c_str());
}
