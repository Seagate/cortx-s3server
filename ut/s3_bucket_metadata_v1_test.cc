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

#include <json/json.h>
#include <memory>

#include "mock_s3_factory.h"
#include "mock_s3_request_object.h"
#include "s3_bucket_metadata_proxy.h"
#include "s3_bucket_metadata_v1.h"
#include "s3_callback_test_helpers.h"
#include "s3_common.h"
#include "s3_test_utils.h"
#include "s3_ut_common.h"

using ::testing::Eq;
using ::testing::Return;
using ::testing::Invoke;
using ::testing::_;
using ::testing::ReturnRef;
using ::testing::AtLeast;
using ::testing::DefaultValue;
using ::testing::Matcher;

const char DEFAULT_ACL_STR[] =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<AccessControlPolicy "
    "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">\n  <Owner>\n    "
    "<ID></ID>\n      <DisplayName></DisplayName>\n  </Owner>\n  "
    "<AccessControlList>\n    <Grant>\n      <Grantee "
    "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
    "xsi:type=\"CanonicalUser\">\n        <ID></ID>\n        "
    "<DisplayName></DisplayName>\n      </Grantee>\n      "
    "<Permission>FULL_CONTROL</Permission>\n    </Grant>\n  "
    "</AccessControlList>\n</AccessControlPolicy>\n";

const char DUMMY_POLICY_STR[] =
    "{\n \"Id\": \"Policy1462526893193\",\n \"Statement\": [\n \"Sid\": "
    "\"Stmt1462526862401\", \n }";

const char DUMMY_ACL_STR[] = "<Owner>\n<ID></ID>\n</Owner>";

class S3BucketMetadataV1Test : public testing::Test {
 protected:
  S3BucketMetadataV1Test();
  void SetUp() override;

  std::shared_ptr<MockS3RequestObject> ptr_mock_request;
  std::shared_ptr<MockS3Motr> ptr_mock_s3_motr_api;
  std::shared_ptr<MockS3MotrKVSReaderFactory> motr_kvs_reader_factory;
  std::shared_ptr<MockS3MotrKVSWriterFactory> motr_kvs_writer_factory;
  std::shared_ptr<MockS3GlobalBucketIndexMetadataFactory>
      s3_global_bucket_index_metadata_factory;

  std::string bucket_name = "seagatebucket";
  bool success_called = false;
  bool fail_called = false;

  std::unique_ptr<S3BucketMetadataProxy> bucket_metadata_proxy;
  std::unique_ptr<S3BucketMetadataV1> action_under_test;

  void func_callback(S3BucketMetadataState);
  std::function<void(S3BucketMetadataState)> helper_functor;
};

S3BucketMetadataV1Test::S3BucketMetadataV1Test()
    : ptr_mock_request(
          std::make_shared<MockS3RequestObject>(nullptr, new EvhtpWrapper())) {

  ptr_mock_request->set_account_name("s3account");
  ptr_mock_request->set_account_id("s3accountid");
  ptr_mock_request->set_user_name("s3user");
  ptr_mock_request->set_user_id("s3userid");

  bucket_metadata_proxy.reset(
      new S3BucketMetadataProxy(ptr_mock_request, bucket_name));
  bucket_metadata_proxy->initialize();

  EXPECT_CALL(*ptr_mock_request, get_bucket_name())
      .WillRepeatedly(ReturnRef(bucket_name));

  ptr_mock_s3_motr_api = std::make_shared<MockS3Motr>();

  EXPECT_CALL(*ptr_mock_s3_motr_api, m0_h_ufid_next(_))
      .WillRepeatedly(Invoke(dummy_helpers_ufid_next));

  motr_kvs_reader_factory = std::make_shared<MockS3MotrKVSReaderFactory>(
      ptr_mock_request, ptr_mock_s3_motr_api);

  motr_kvs_writer_factory = std::make_shared<MockS3MotrKVSWriterFactory>(
      ptr_mock_request, ptr_mock_s3_motr_api);

  s3_global_bucket_index_metadata_factory =
      std::make_shared<MockS3GlobalBucketIndexMetadataFactory>(
          ptr_mock_request);

  helper_functor = std::bind(&S3BucketMetadataV1Test::func_callback, this,
                             std::placeholders::_1);
}

void S3BucketMetadataV1Test::func_callback(S3BucketMetadataState state) {
  switch (state) {
    case S3BucketMetadataState::failed:
    case S3BucketMetadataState::failed_to_launch:
    case S3BucketMetadataState::missing:
      fail_called = true;
      break;
    default:
      success_called = true;
  }
}

void S3BucketMetadataV1Test::SetUp() {

  action_under_test.reset(new S3BucketMetadataV1(
      *bucket_metadata_proxy, ptr_mock_s3_motr_api, motr_kvs_reader_factory,
      motr_kvs_writer_factory, s3_global_bucket_index_metadata_factory));

  action_under_test->callback = helper_functor;

  success_called = false;
  fail_called = false;
}

TEST_F(S3BucketMetadataV1Test, ConstructorTest) {
  struct m0_uint128 zero_oid = {0ULL, 0ULL};
  EXPECT_STREQ("s3user", action_under_test->user_name.c_str());
  EXPECT_STREQ("s3account", action_under_test->account_name.c_str());
  EXPECT_STREQ("", action_under_test->bucket_policy.c_str());
  EXPECT_STREQ("index_salt_", action_under_test->collision_salt.c_str());
  EXPECT_EQ(0, action_under_test->collision_attempt_count);
  EXPECT_OID_EQ(zero_oid,
                action_under_test->get_object_list_index_layout().oid);
  EXPECT_OID_EQ(zero_oid, action_under_test->object_list_index_layout.oid);
  EXPECT_OID_EQ(zero_oid, action_under_test->multipart_index_layout.oid);
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

TEST_F(S3BucketMetadataV1Test, GetSystemAttributesTest) {
  EXPECT_STRNE("", action_under_test->get_creation_time().c_str());
  action_under_test->system_defined_attribute["Owner-User-id"] = "s3owner";
  EXPECT_STREQ("s3owner", action_under_test->get_owner_id().c_str());
  EXPECT_STREQ("us-west-2",
               action_under_test->get_location_constraint().c_str());
  action_under_test->system_defined_attribute["Owner-User"] = "s3user";
  EXPECT_STREQ("s3user", action_under_test->get_owner_name().c_str());
}

TEST_F(S3BucketMetadataV1Test, GetSetOIDsPolicyAndLocation) {
  struct m0_uint128 oid = {0x1ffff, 0x1fff};
  action_under_test->multipart_index_layout = {{0x1fff, 0xff1}};
  action_under_test->object_list_index_layout = {{0xff, 0xff}};

  EXPECT_OID_EQ(action_under_test->get_multipart_index_layout().oid,
                action_under_test->multipart_index_layout.oid);
  EXPECT_OID_EQ(action_under_test->get_object_list_index_layout().oid,
                action_under_test->object_list_index_layout.oid);

  action_under_test->set_multipart_index_layout({oid});
  EXPECT_OID_EQ(action_under_test->multipart_index_layout.oid, oid);

  action_under_test->set_object_list_index_layout({oid});
  EXPECT_OID_EQ(action_under_test->object_list_index_layout.oid, oid);

  action_under_test->set_location_constraint("us-east");
  EXPECT_STREQ("us-east",
               action_under_test->system_defined_attribute["LocationConstraint"]
                   .c_str());
}

TEST_F(S3BucketMetadataV1Test, SetBucketPolicy) {
  std::string policy = DUMMY_POLICY_STR;
  action_under_test->setpolicy(policy);
  EXPECT_STREQ(DUMMY_POLICY_STR, action_under_test->bucket_policy.c_str());
}

TEST_F(S3BucketMetadataV1Test, DeletePolicy) {
  action_under_test->deletepolicy();
  EXPECT_STREQ("", action_under_test->bucket_policy.c_str());
}

TEST_F(S3BucketMetadataV1Test, SetAcl) {
  char expected_str[] = "<Owner>\n<ID></ID>\n</Owner>";
  std::string acl = DUMMY_ACL_STR;
  action_under_test->setacl(acl);
  EXPECT_STREQ(expected_str, action_under_test->encoded_acl.c_str());
}

TEST_F(S3BucketMetadataV1Test, GetTagsAsXml) {
  char expected_str[] =
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?><Tagging "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/"
      "\"><TagSet><Tag><Key>organization1234</Key><Value>marketing1234</"
      "Value></Tag><Tag><Key>organization124</Key><Value>marketing123</Value></"
      "Tag></TagSet></Tagging>";
  std::map<std::string, std::string> bucket_tags_map;
  bucket_tags_map["organization124"] = "marketing123";
  bucket_tags_map["organization1234"] = "marketing1234";

  action_under_test->set_tags(bucket_tags_map);

  EXPECT_STREQ(expected_str, action_under_test->get_tags_as_xml().c_str());
}

TEST_F(S3BucketMetadataV1Test, GetSpecialCharTagsAsXml) {
  char expected_str[] =
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?><Tagging "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/"
      "\"><TagSet><Tag><Key>organiza$tion</Key><Value>marketin*g</Value></"
      "Tag><Tag><Key>organizati#on124</Key><Value>marke@ting123</Value></Tag></"
      "TagSet></Tagging>";
  std::map<std::string, std::string> bucket_tags_map;
  bucket_tags_map["organiza$tion"] = "marketin*g";
  bucket_tags_map["organizati#on124"] = "marke@ting123";

  action_under_test->set_tags(bucket_tags_map);

  EXPECT_STREQ(expected_str, action_under_test->get_tags_as_xml().c_str());
}

TEST_F(S3BucketMetadataV1Test, AddSystemAttribute) {
  action_under_test->add_system_attribute("LocationConstraint", "us-east");
  EXPECT_STREQ("us-east",
               action_under_test->system_defined_attribute["LocationConstraint"]
                   .c_str());
}

TEST_F(S3BucketMetadataV1Test, AddUserDefinedAttribute) {
  action_under_test->add_user_defined_attribute("Etag", "1234");
  EXPECT_STREQ("1234",
               action_under_test->user_defined_attribute["Etag"].c_str());
}

TEST_F(S3BucketMetadataV1Test, Load) {
  EXPECT_CALL(*(s3_global_bucket_index_metadata_factory
                    ->mock_global_bucket_index_metadata),
              load(_, _)).Times(1);
  action_under_test->load(*bucket_metadata_proxy, helper_functor);
}

TEST_F(S3BucketMetadataV1Test, LoadBucketInfo) {
  EXPECT_CALL(*(motr_kvs_reader_factory->mock_motr_kvs_reader),
              get_keyval(_, "12345/seagate", _, _)).Times(1);
  action_under_test->account_id = "12345";
  action_under_test->bucket_name = "seagate";
  action_under_test->load_bucket_info();
}

TEST_F(S3BucketMetadataV1Test, LoadBucketInfoFailedJsonParsingFailed) {
  action_under_test->json_parsing_error = true;
  action_under_test->load_bucket_info_failed();
  EXPECT_TRUE(fail_called);
  EXPECT_EQ(S3BucketMetadataState::failed, action_under_test->state);
}

TEST_F(S3BucketMetadataV1Test, LoadBucketInfoFailedMetadataMissing) {
  action_under_test->motr_kv_reader =
      motr_kvs_reader_factory->mock_motr_kvs_reader;
  EXPECT_CALL(*(motr_kvs_reader_factory->mock_motr_kvs_reader), get_state())
      .Times(1)
      .WillRepeatedly(Return(S3MotrKVSReaderOpState::missing));
  action_under_test->load_bucket_info_failed();
  EXPECT_TRUE(fail_called);
  EXPECT_EQ(S3BucketMetadataState::missing, action_under_test->state);
}

TEST_F(S3BucketMetadataV1Test, LoadBucketInfoFailedMetadataFailed) {
  action_under_test->motr_kv_reader =
      motr_kvs_reader_factory->mock_motr_kvs_reader;
  EXPECT_CALL(*(motr_kvs_reader_factory->mock_motr_kvs_reader), get_state())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(S3MotrKVSReaderOpState::failed));
  action_under_test->load_bucket_info_failed();
  EXPECT_TRUE(fail_called);
  EXPECT_EQ(S3BucketMetadataState::failed, action_under_test->state);
}

TEST_F(S3BucketMetadataV1Test, LoadBucketInfoFailedMetadataFailedToLaunch) {
  action_under_test->motr_kv_reader =
      motr_kvs_reader_factory->mock_motr_kvs_reader;
  EXPECT_CALL(*(motr_kvs_reader_factory->mock_motr_kvs_reader), get_state())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(S3MotrKVSReaderOpState::failed_to_launch));
  action_under_test->load_bucket_info_failed();
  EXPECT_TRUE(fail_called);
  EXPECT_EQ(S3BucketMetadataState::failed_to_launch, action_under_test->state);
}

TEST_F(S3BucketMetadataV1Test, SaveMetadataIndexOIDMissing) {
  struct m0_uint128 oid = {0ULL, 0ULL};
  action_under_test->set_object_list_index_layout({oid});

  action_under_test->global_bucket_index_metadata =
      s3_global_bucket_index_metadata_factory
          ->mock_global_bucket_index_metadata;

  bucket_metadata_proxy->set_state(S3BucketMetadataState::missing);

  EXPECT_CALL(*(s3_global_bucket_index_metadata_factory
                    ->mock_global_bucket_index_metadata),
              save(_, _)).Times(1);
  action_under_test->save(*bucket_metadata_proxy, helper_functor);
}

TEST_F(S3BucketMetadataV1Test, SaveMetadataIndexOIDPresent) {
  bucket_metadata_proxy->set_state(S3BucketMetadataState::present);

  action_under_test->global_bucket_index_metadata =
      s3_global_bucket_index_metadata_factory
          ->mock_global_bucket_index_metadata;

  EXPECT_CALL(*(s3_global_bucket_index_metadata_factory
                    ->mock_global_bucket_index_metadata),
              save(_, _)).Times(0);

  ASSERT_DEATH(action_under_test->save(*bucket_metadata_proxy, helper_functor),
               ".*");
}

TEST_F(S3BucketMetadataV1Test, UpdateMetadataIndexOIDPresent) {
  struct m0_uint128 oid = {0x111f, 0xffff};
  action_under_test->set_object_list_index_layout({oid});
  action_under_test->object_list_index_layout = {{0x11ff, 0x1fff}};
  action_under_test->account_id = "12345";

  bucket_metadata_proxy->set_state(S3BucketMetadataState::present);
  EXPECT_CALL(*(motr_kvs_writer_factory->mock_motr_kvs_writer),
              put_keyval(_, Matcher<const std::string&>(testing::_), _, _, _,
                         _)).Times(1);

  action_under_test->update(*bucket_metadata_proxy, helper_functor);
}

TEST_F(S3BucketMetadataV1Test, UpdateMetadataIndexOIDMissing) {
  bucket_metadata_proxy->set_state(S3BucketMetadataState::missing);

  EXPECT_CALL(*(motr_kvs_writer_factory->mock_motr_kvs_writer),
              put_keyval(_, Matcher<const std::string&>(testing::_), _, _, _,
                         _)).Times(0);

  ASSERT_DEATH(
      action_under_test->update(*bucket_metadata_proxy, helper_functor), ".*");
}

TEST_F(S3BucketMetadataV1Test, CreateObjectListIndexCollisionCount0) {
  action_under_test->collision_attempt_count = 0;
  EXPECT_CALL(*(motr_kvs_writer_factory->mock_motr_kvs_writer),
              create_index_with_oid(_, _, _)).Times(1);
  action_under_test->create_object_list_index();
}

TEST_F(S3BucketMetadataV1Test, CreateMultipartListIndexCollisionCount0) {
  action_under_test->collision_attempt_count = 0;
  EXPECT_CALL(*(motr_kvs_writer_factory->mock_motr_kvs_writer),
              create_index_with_oid(_, _, _)).Times(1);
  action_under_test->create_multipart_list_index();
}

TEST_F(S3BucketMetadataV1Test, CreateObjectListIndexSuccessful) {
  action_under_test->collision_attempt_count = 0;
  action_under_test->motr_kv_writer =
      motr_kvs_writer_factory->mock_motr_kvs_writer;
  EXPECT_CALL(*(motr_kvs_writer_factory->mock_motr_kvs_writer),
              create_index_with_oid(_, _, _)).Times(1);
  action_under_test->create_object_list_index_successful();
}

TEST_F(S3BucketMetadataV1Test, CreateObjectListIndexFailedCollisionHappened) {
  action_under_test->motr_kv_writer =
      motr_kvs_writer_factory->mock_motr_kvs_writer;
  action_under_test->collision_attempt_count = 1;
  EXPECT_CALL(*(motr_kvs_writer_factory->mock_motr_kvs_writer), get_state())
      .Times(1)
      .WillRepeatedly(Return(S3MotrKVSWriterOpState::exists));

  EXPECT_CALL(*(motr_kvs_writer_factory->mock_motr_kvs_writer),
              create_index_with_oid(_, _, _)).Times(1);

  action_under_test->create_object_list_index_failed();
  EXPECT_EQ(2, action_under_test->collision_attempt_count);
}

TEST_F(S3BucketMetadataV1Test,
       CreateMultipartListIndexFailedCollisionHappened) {
  action_under_test->motr_kv_writer =
      motr_kvs_writer_factory->mock_motr_kvs_writer;
  action_under_test->collision_attempt_count = 1;
  EXPECT_CALL(*(motr_kvs_writer_factory->mock_motr_kvs_writer), get_state())
      .Times(1)
      .WillRepeatedly(Return(S3MotrKVSWriterOpState::exists));

  EXPECT_CALL(*(motr_kvs_writer_factory->mock_motr_kvs_writer),
              create_index_with_oid(_, _, _)).Times(1);

  action_under_test->create_multipart_list_index_failed();
  EXPECT_EQ(2, action_under_test->collision_attempt_count);
}

TEST_F(S3BucketMetadataV1Test, CreateObjectListIndexFailed) {
  action_under_test->motr_kv_writer =
      motr_kvs_writer_factory->mock_motr_kvs_writer;
  EXPECT_CALL(*(motr_kvs_writer_factory->mock_motr_kvs_writer), get_state())
      .Times(2)
      .WillRepeatedly(Return(S3MotrKVSWriterOpState::failed));
  action_under_test->global_bucket_index_metadata =
      s3_global_bucket_index_metadata_factory
          ->mock_global_bucket_index_metadata;
  EXPECT_CALL(*(s3_global_bucket_index_metadata_factory
                    ->mock_global_bucket_index_metadata),
              remove(_, _))
      .Times(1)
      .WillOnce(Invoke([&](std::function<void(void)>,
                           std::function<void(void)>) {
         action_under_test
             ->cleanup_on_create_err_global_bucket_account_id_info_fini_cb();
       }));
  action_under_test->create_object_list_index_failed();
  EXPECT_EQ(action_under_test->state, S3BucketMetadataState::failed);
  EXPECT_TRUE(fail_called);
}

TEST_F(S3BucketMetadataV1Test, CreateObjectListIndexFailedToLaunch) {
  action_under_test->motr_kv_writer =
      motr_kvs_writer_factory->mock_motr_kvs_writer;
  EXPECT_CALL(*(motr_kvs_writer_factory->mock_motr_kvs_writer), get_state())
      .Times(2)
      .WillRepeatedly(Return(S3MotrKVSWriterOpState::failed_to_launch));
  action_under_test->global_bucket_index_metadata =
      s3_global_bucket_index_metadata_factory
          ->mock_global_bucket_index_metadata;

  EXPECT_CALL(*(s3_global_bucket_index_metadata_factory
                    ->mock_global_bucket_index_metadata),
              remove(_, _))
      .Times(1)
      .WillOnce(Invoke([&](std::function<void(void)>,
                           std::function<void(void)>) {
         action_under_test
             ->cleanup_on_create_err_global_bucket_account_id_info_fini_cb();
       }));
  action_under_test->create_object_list_index_failed();
  EXPECT_EQ(action_under_test->state, S3BucketMetadataState::failed_to_launch);
  EXPECT_TRUE(fail_called);
}

TEST_F(S3BucketMetadataV1Test, CreateMultipartListIndexFailed) {
  action_under_test->motr_kv_writer =
      motr_kvs_writer_factory->mock_motr_kvs_writer;
  EXPECT_CALL(*(motr_kvs_writer_factory->mock_motr_kvs_writer), get_state())
      .Times(2)
      .WillRepeatedly(Return(S3MotrKVSWriterOpState::failed));
  action_under_test->global_bucket_index_metadata =
      s3_global_bucket_index_metadata_factory
          ->mock_global_bucket_index_metadata;
  EXPECT_CALL(*(s3_global_bucket_index_metadata_factory
                    ->mock_global_bucket_index_metadata),
              remove(_, _))
      .Times(1)
      .WillOnce(Invoke([&](std::function<void(void)>,
                           std::function<void(void)>) {
         action_under_test
             ->cleanup_on_create_err_global_bucket_account_id_info_fini_cb();
       }));
  action_under_test->create_multipart_list_index_failed();
  EXPECT_TRUE(fail_called);
  EXPECT_EQ(action_under_test->state, S3BucketMetadataState::failed);
}

TEST_F(S3BucketMetadataV1Test, CreateMultipartListIndexFailedToLaunch) {
  action_under_test->motr_kv_writer =
      motr_kvs_writer_factory->mock_motr_kvs_writer;
  EXPECT_CALL(*(motr_kvs_writer_factory->mock_motr_kvs_writer), get_state())
      .Times(2)
      .WillRepeatedly(Return(S3MotrKVSWriterOpState::failed_to_launch));
  action_under_test->global_bucket_index_metadata =
      s3_global_bucket_index_metadata_factory
          ->mock_global_bucket_index_metadata;
  EXPECT_CALL(*(s3_global_bucket_index_metadata_factory
                    ->mock_global_bucket_index_metadata),
              remove(_, _))
      .Times(1)
      .WillOnce(Invoke([&](std::function<void(void)>,
                           std::function<void(void)>) {
         action_under_test
             ->cleanup_on_create_err_global_bucket_account_id_info_fini_cb();
       }));
  action_under_test->create_multipart_list_index_failed();
  EXPECT_TRUE(fail_called);
  EXPECT_EQ(action_under_test->state, S3BucketMetadataState::failed_to_launch);
}

TEST_F(S3BucketMetadataV1Test, HandleCollision) {
  bool callback_called = false;

  action_under_test->collision_attempt_count = 1;
  action_under_test->salted_object_list_index_name = "BUCKET/seagate_salt_0";

  action_under_test->handle_collision(
      "BUCKET/seagate", action_under_test->salted_object_list_index_name,
      [&callback_called]() { callback_called = true; });

  EXPECT_STREQ("BUCKET/seagateindex_salt_1",
               action_under_test->salted_object_list_index_name.c_str());
  EXPECT_EQ(2, action_under_test->collision_attempt_count);
  EXPECT_TRUE(callback_called);
}

TEST_F(S3BucketMetadataV1Test, HandleCollisionMaxAttemptExceeded) {
  bool callback_called = false;

  action_under_test->salted_object_list_index_name = "BUCKET/seagate_salt_0";
  action_under_test->collision_attempt_count = MAX_COLLISION_RETRY_COUNT + 1;

  action_under_test->handle_collision(
      "BUCKET/seagate", action_under_test->salted_object_list_index_name,
      [&callback_called]() { callback_called = true; });

  EXPECT_FALSE(callback_called);
  EXPECT_EQ(S3BucketMetadataState::failed, action_under_test->state);
  EXPECT_EQ(MAX_COLLISION_RETRY_COUNT + 1,
            action_under_test->collision_attempt_count);
}

TEST_F(S3BucketMetadataV1Test, RegeneratedNewIndexName) {
  std::string base_index_name = "BUCKET/seagate";
  action_under_test->salted_object_list_index_name = "BUCKET/seagate";
  action_under_test->collision_attempt_count = 2;
  action_under_test->regenerate_new_index_name(
      base_index_name, action_under_test->salted_object_list_index_name);
  EXPECT_STREQ("BUCKET/seagateindex_salt_2",
               action_under_test->salted_object_list_index_name.c_str());
}

TEST_F(S3BucketMetadataV1Test, CreateExtendedMetadataIndexFailed) {
  action_under_test->motr_kv_writer =
      motr_kvs_writer_factory->mock_motr_kvs_writer;
  EXPECT_CALL(*(motr_kvs_writer_factory->mock_motr_kvs_writer), get_state())
      .Times(1)
      .WillRepeatedly(Return(S3MotrKVSWriterOpState::failed));
  action_under_test->global_bucket_index_metadata =
      s3_global_bucket_index_metadata_factory
          ->mock_global_bucket_index_metadata;
  EXPECT_CALL(*(s3_global_bucket_index_metadata_factory
                    ->mock_global_bucket_index_metadata),
              remove(_, _))
      .Times(1)
      .WillOnce(Invoke([&](std::function<void(void)>,
                           std::function<void(void)>) {
         action_under_test
             ->cleanup_on_create_err_global_bucket_account_id_info_fini_cb();
       }));
  action_under_test->create_extended_metadata_index_failed();
  EXPECT_EQ(action_under_test->state, S3BucketMetadataState::failed);
  EXPECT_TRUE(fail_called);
}

TEST_F(S3BucketMetadataV1Test, CreateExtendedMetadataIndexFailedToLaunch) {
  action_under_test->motr_kv_writer =
      motr_kvs_writer_factory->mock_motr_kvs_writer;
  EXPECT_CALL(*(motr_kvs_writer_factory->mock_motr_kvs_writer), get_state())
      .Times(1)
      .WillRepeatedly(Return(S3MotrKVSWriterOpState::failed_to_launch));
  action_under_test->global_bucket_index_metadata =
      s3_global_bucket_index_metadata_factory
          ->mock_global_bucket_index_metadata;

  EXPECT_CALL(*(s3_global_bucket_index_metadata_factory
                    ->mock_global_bucket_index_metadata),
              remove(_, _))
      .Times(1)
      .WillOnce(Invoke([&](std::function<void(void)>,
                           std::function<void(void)>) {
         action_under_test
             ->cleanup_on_create_err_global_bucket_account_id_info_fini_cb();
       }));
  action_under_test->create_extended_metadata_index_failed();
  EXPECT_EQ(action_under_test->state, S3BucketMetadataState::failed_to_launch);
  EXPECT_TRUE(fail_called);
}

TEST_F(S3BucketMetadataV1Test, SaveBucketInfo) {
  action_under_test->motr_kv_writer =
      motr_kvs_writer_factory->mock_motr_kvs_writer;
  EXPECT_CALL(*(motr_kvs_writer_factory->mock_motr_kvs_writer),
              put_keyval(_, Matcher<const std::string&>(testing::_), _, _, _,
                         _)).Times(1);

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

TEST_F(S3BucketMetadataV1Test, SaveBucketInfoSuccess) {
  action_under_test->save_bucket_info_successful();
  EXPECT_TRUE(success_called);
}

TEST_F(S3BucketMetadataV1Test, RemoveBucketInfoSuccessful) {
  action_under_test->global_bucket_index_metadata =
      s3_global_bucket_index_metadata_factory
          ->mock_global_bucket_index_metadata;

  EXPECT_CALL(*(s3_global_bucket_index_metadata_factory
                    ->mock_global_bucket_index_metadata),
              remove(_, _)).Times(AtLeast(1));

  action_under_test->remove_bucket_info_successful();
}

TEST_F(S3BucketMetadataV1Test, SaveBucketInfoFailed) {
  action_under_test->motr_kv_writer =
      motr_kvs_writer_factory->mock_motr_kvs_writer;
  EXPECT_CALL(*(motr_kvs_writer_factory->mock_motr_kvs_writer), get_state())
      .WillRepeatedly(Return(S3MotrKVSWriterOpState::failed));
  action_under_test->save_bucket_info_failed();
  EXPECT_TRUE(fail_called);
  EXPECT_EQ(S3BucketMetadataState::failed, action_under_test->state);
}

TEST_F(S3BucketMetadataV1Test, SaveBucketInfoFailedToLaunch) {
  action_under_test->motr_kv_writer =
      motr_kvs_writer_factory->mock_motr_kvs_writer;
  EXPECT_CALL(*(motr_kvs_writer_factory->mock_motr_kvs_writer), get_state())
      .WillRepeatedly(Return(S3MotrKVSWriterOpState::failed_to_launch));
  action_under_test->save_bucket_info_failed();
  EXPECT_TRUE(fail_called);
  EXPECT_EQ(S3BucketMetadataState::failed_to_launch, action_under_test->state);
}

TEST_F(S3BucketMetadataV1Test, SaveBucketInfoFailedWithGlobCleanup) {
  action_under_test->motr_kv_writer =
      motr_kvs_writer_factory->mock_motr_kvs_writer;
  EXPECT_CALL(*(motr_kvs_writer_factory->mock_motr_kvs_writer), get_state())
      .WillRepeatedly(Return(S3MotrKVSWriterOpState::failed));
  action_under_test->should_cleanup_global_idx = true;
  action_under_test->global_bucket_index_metadata =
      s3_global_bucket_index_metadata_factory
          ->mock_global_bucket_index_metadata;
  EXPECT_CALL(*(s3_global_bucket_index_metadata_factory
                    ->mock_global_bucket_index_metadata),
              remove(_, _))
      .Times(1)
      .WillOnce(Invoke([&](std::function<void(void)>,
                           std::function<void(void)>) {
         action_under_test
             ->cleanup_on_create_err_global_bucket_account_id_info_fini_cb();
       }));
  action_under_test->save_bucket_info_failed();
  EXPECT_TRUE(fail_called);
  EXPECT_EQ(S3BucketMetadataState::failed, action_under_test->state);
}

TEST_F(S3BucketMetadataV1Test, SaveBucketInfoFailedToLaunchWithGlobCleanup) {
  action_under_test->motr_kv_writer =
      motr_kvs_writer_factory->mock_motr_kvs_writer;
  EXPECT_CALL(*(motr_kvs_writer_factory->mock_motr_kvs_writer), get_state())
      .WillRepeatedly(Return(S3MotrKVSWriterOpState::failed_to_launch));
  action_under_test->should_cleanup_global_idx = true;
  action_under_test->global_bucket_index_metadata =
      s3_global_bucket_index_metadata_factory
          ->mock_global_bucket_index_metadata;
  EXPECT_CALL(*(s3_global_bucket_index_metadata_factory
                    ->mock_global_bucket_index_metadata),
              remove(_, _))
      .Times(1)
      .WillOnce(Invoke([&](std::function<void(void)>,
                           std::function<void(void)>) {
         action_under_test
             ->cleanup_on_create_err_global_bucket_account_id_info_fini_cb();
       }));
  action_under_test->save_bucket_info_failed();
  EXPECT_TRUE(fail_called);
  EXPECT_EQ(S3BucketMetadataState::failed_to_launch, action_under_test->state);
}

TEST_F(S3BucketMetadataV1Test, RemovePresentMetadata) {
  action_under_test->callback = nullptr;
  action_under_test->state = S3BucketMetadataState::present;

  EXPECT_CALL(*(motr_kvs_writer_factory->mock_motr_kvs_writer),
              delete_keyval(_, _, _, _)).Times(1);

  action_under_test->remove(helper_functor);
  EXPECT_TRUE(action_under_test->callback);
}

TEST_F(S3BucketMetadataV1Test, RemoveAbsentMetadata) {
  action_under_test->state = S3BucketMetadataState::missing;

  EXPECT_CALL(*(motr_kvs_writer_factory->mock_motr_kvs_writer),
              delete_keyval(_, _, _, _)).Times(0);

  ASSERT_DEATH(action_under_test->remove(helper_functor), ".*");
}

TEST_F(S3BucketMetadataV1Test, RemoveBucketInfo) {
  action_under_test->motr_kv_writer =
      motr_kvs_writer_factory->mock_motr_kvs_writer;

  EXPECT_CALL(*(motr_kvs_writer_factory->mock_motr_kvs_writer),
              delete_keyval(_, _, _, _)).Times(1);
  action_under_test->remove_bucket_info();
}

TEST_F(S3BucketMetadataV1Test, RemoveBucketAccountidInfoSuccessful) {
  action_under_test->remove_global_bucket_account_id_info_successful();
  EXPECT_EQ(action_under_test->state, S3BucketMetadataState::missing);
}

TEST_F(S3BucketMetadataV1Test, RemoveBucketAccountidInfoFailedToLaunch) {
  action_under_test->global_bucket_index_metadata =
      s3_global_bucket_index_metadata_factory
          ->mock_global_bucket_index_metadata;

  EXPECT_CALL(*(s3_global_bucket_index_metadata_factory
                    ->mock_global_bucket_index_metadata),
              get_state())
      .Times(1)
      .WillRepeatedly(
           Return(S3GlobalBucketIndexMetadataState::failed_to_launch));

  action_under_test->remove_global_bucket_account_id_info_failed();
  EXPECT_EQ(S3BucketMetadataState::failed_to_launch, action_under_test->state);
  EXPECT_TRUE(fail_called);
}

TEST_F(S3BucketMetadataV1Test, RemoveBucketAccountidInfoFailed) {
  action_under_test->global_bucket_index_metadata =
      s3_global_bucket_index_metadata_factory
          ->mock_global_bucket_index_metadata;

  EXPECT_CALL(*(s3_global_bucket_index_metadata_factory
                    ->mock_global_bucket_index_metadata),
              get_state())
      .Times(1)
      .WillRepeatedly(Return(S3GlobalBucketIndexMetadataState::failed));

  action_under_test->remove_global_bucket_account_id_info_failed();
  EXPECT_EQ(S3BucketMetadataState::failed, action_under_test->state);
  EXPECT_TRUE(fail_called);
}

TEST_F(S3BucketMetadataV1Test, RemoveBucketInfoFailed) {
  action_under_test->motr_kv_writer =
      motr_kvs_writer_factory->mock_motr_kvs_writer;
  EXPECT_CALL(*(motr_kvs_writer_factory->mock_motr_kvs_writer), get_state())
      .Times(1)
      .WillRepeatedly(Return(S3MotrKVSWriterOpState::failed));
  action_under_test->remove_bucket_info_failed();
  EXPECT_EQ(S3BucketMetadataState::failed, action_under_test->state);
  EXPECT_TRUE(fail_called);
}

TEST_F(S3BucketMetadataV1Test, RemoveBucketInfoFailedToLaunch) {
  action_under_test->motr_kv_writer =
      motr_kvs_writer_factory->mock_motr_kvs_writer;
  EXPECT_CALL(*(motr_kvs_writer_factory->mock_motr_kvs_writer), get_state())
      .Times(1)
      .WillRepeatedly(Return(S3MotrKVSWriterOpState::failed_to_launch));
  action_under_test->remove_bucket_info_failed();
  EXPECT_TRUE(fail_called);
  EXPECT_EQ(S3BucketMetadataState::failed_to_launch, action_under_test->state);
}

TEST_F(S3BucketMetadataV1Test, ToJson) {
  std::string json_str = action_under_test->to_json();
  Json::Value newglobal;
  Json::Reader reader;
  bool parsingSuccessful = reader.parse(json_str, newglobal);
  EXPECT_TRUE(parsingSuccessful);
}

TEST_F(S3BucketMetadataV1Test, FromJson) {
  struct m0_uint128 zero_oid = {0ULL, 0ULL};
  EXPECT_OID_EQ(zero_oid, action_under_test->multipart_index_layout.oid);
  std::string json_str =
      "{\"ACL\":\"PD94+Cg==\",\"Bucket-Name\":\"seagatebucket\",\"System-"
      "Defined\":{\"Date\":\"2016-10-18T16:01:00.000Z\",\"Owner-Account\":\"s3_"
      "test\",\"Owner-Account-id\":\"s3accountid\",\"Owner-User\":\"tester\",\""
      "Owner-User-id\":\"s3userid\"},\"motr_multipart_index_layout\":\""
      "g1qTetGfvWklvH6Q65xFAI==\","
      "\"motr_object_list_index_layout\":\"ABAAAAAAAAAAAAAAAAAAAA==\"}";

  action_under_test->from_json(json_str);
  EXPECT_STREQ("seagatebucket", action_under_test->bucket_name.c_str());
  EXPECT_STREQ("tester", action_under_test->user_name.c_str());
  EXPECT_STREQ("s3_test", action_under_test->account_name.c_str());
  EXPECT_STREQ("s3accountid", action_under_test->account_id.c_str());
  EXPECT_OID_NE(zero_oid, action_under_test->multipart_index_layout.oid);
}

TEST_F(S3BucketMetadataV1Test, GetEncodedBucketAcl) {
  std::string json_str = "{\"ACL\":\"PD94bg==\"}";

  action_under_test->from_json(json_str);
  EXPECT_STREQ("PD94bg==", action_under_test->get_encoded_bucket_acl().c_str());
}
