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
#include "s3_callback_test_helpers.h"
#include "s3_common.h"
#include "s3_object_metadata.h"
#include "s3_test_utils.h"
#include "s3_ut_common.h"

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

#define DUMMY_ACL_STR "<Owner>\n<ID>1</ID>\n</Owner>"

class S3ObjectMetadataTest : public testing::Test {
 protected:
  S3ObjectMetadataTest() {
    evhtp_request_t *req = NULL;
    EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();
    call_count_one = 0;
    ptr_mock_request =
        std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr);

    ptr_mock_s3_clovis_api = std::make_shared<MockS3Clovis>();
    EXPECT_CALL(*ptr_mock_s3_clovis_api, m0_h_ufid_next(_))
        .WillRepeatedly(Invoke(dummy_helpers_ufid_next));

    clovis_kvs_reader_factory = std::make_shared<MockS3ClovisKVSReaderFactory>(
        ptr_mock_request, ptr_mock_s3_clovis_api);

    clovis_kvs_writer_factory = std::make_shared<MockS3ClovisKVSWriterFactory>(
        ptr_mock_request, ptr_mock_s3_clovis_api);

    bucket_meta_factory = std::make_shared<MockS3BucketMetadataFactory>(
        ptr_mock_request, ptr_mock_s3_clovis_api);

    ptr_mock_request->set_account_name("s3account");
    ptr_mock_request->set_user_name("s3user");
    bucket_indx_oid = {0xffff, 0xffff};

    action_under_test.reset(new S3ObjectMetadata(
        ptr_mock_request, false, "", clovis_kvs_reader_factory,
        clovis_kvs_writer_factory, bucket_meta_factory,
        ptr_mock_s3_clovis_api));
    action_under_test_with_oid.reset(new S3ObjectMetadata(
        ptr_mock_request, bucket_indx_oid, false, "", clovis_kvs_reader_factory,
        clovis_kvs_writer_factory, bucket_meta_factory,
        ptr_mock_s3_clovis_api));
  }

  std::shared_ptr<MockS3RequestObject> ptr_mock_request;
  std::shared_ptr<MockS3Clovis> ptr_mock_s3_clovis_api;
  std::shared_ptr<MockS3BucketMetadataFactory> bucket_meta_factory;
  std::shared_ptr<MockS3ClovisKVSReaderFactory> clovis_kvs_reader_factory;
  std::shared_ptr<MockS3ClovisKVSWriterFactory> clovis_kvs_writer_factory;
  S3CallBack s3objectmetadata_callbackobj;
  std::shared_ptr<S3ObjectMetadata> action_under_test;
  std::shared_ptr<S3ObjectMetadata> action_under_test_with_oid;
  struct m0_uint128 bucket_indx_oid;
  int call_count_one;

 public:
  void func_callback_one() { call_count_one += 1; }
};

class S3MultipartObjectMetadataTest : public testing::Test {
 protected:
  S3MultipartObjectMetadataTest() {
    evhtp_request_t *req = NULL;
    EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();
    call_count_one = 0;
    ptr_mock_request =
        std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr);

    ptr_mock_s3_clovis_api = std::make_shared<MockS3Clovis>();

    clovis_kvs_reader_factory = std::make_shared<MockS3ClovisKVSReaderFactory>(
        ptr_mock_request, ptr_mock_s3_clovis_api);

    clovis_kvs_writer_factory = std::make_shared<MockS3ClovisKVSWriterFactory>(
        ptr_mock_request, ptr_mock_s3_clovis_api);

    bucket_meta_factory =
        std::make_shared<MockS3BucketMetadataFactory>(ptr_mock_request);

    ptr_mock_request->set_account_name("s3account");
    ptr_mock_request->set_user_name("s3user");
    bucket_indx_oid = {0xffff, 0xffff};
    action_under_test.reset(new S3ObjectMetadata(
        ptr_mock_request, true, "1234-1234", clovis_kvs_reader_factory,
        clovis_kvs_writer_factory, bucket_meta_factory,
        ptr_mock_s3_clovis_api));

    action_under_test_with_oid.reset(new S3ObjectMetadata(
        ptr_mock_request, bucket_indx_oid, true, "1234-1234",
        clovis_kvs_reader_factory, clovis_kvs_writer_factory,
        bucket_meta_factory, ptr_mock_s3_clovis_api));
  }

  std::shared_ptr<MockS3RequestObject> ptr_mock_request;
  std::shared_ptr<MockS3Clovis> ptr_mock_s3_clovis_api;
  std::shared_ptr<MockS3BucketMetadataFactory> bucket_meta_factory;
  std::shared_ptr<MockS3ClovisKVSReaderFactory> clovis_kvs_reader_factory;
  std::shared_ptr<MockS3ClovisKVSWriterFactory> clovis_kvs_writer_factory;
  S3CallBack s3objectmetadata_callbackobj;
  std::shared_ptr<S3ObjectMetadata> action_under_test;
  std::shared_ptr<S3ObjectMetadata> action_under_test_with_oid;
  struct m0_uint128 bucket_indx_oid;
  int call_count_one;

 public:
  void func_callback_one() { call_count_one += 1; }
};

TEST_F(S3ObjectMetadataTest, ConstructorTest) {
  struct m0_uint128 zero_oid = {0ULL, 0ULL};
  std::string index_name;
  EXPECT_STREQ("s3user", action_under_test->user_name.c_str());
  EXPECT_STREQ("s3account", action_under_test->account_name.c_str());
  EXPECT_OID_EQ(zero_oid, action_under_test->index_oid);
  EXPECT_OID_EQ(bucket_indx_oid, action_under_test_with_oid->index_oid);
  EXPECT_OID_EQ(zero_oid, action_under_test->old_oid);
  EXPECT_OID_EQ(M0_CLOVIS_ID_APP, action_under_test->oid);
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
}

TEST_F(S3MultipartObjectMetadataTest, ConstructorTest) {
  struct m0_uint128 zero_oid = {0ULL, 0ULL};
  std::string index_name;
  EXPECT_STREQ("s3user", action_under_test->user_name.c_str());
  EXPECT_STREQ("s3account", action_under_test->account_name.c_str());
  EXPECT_OID_EQ(zero_oid, action_under_test->index_oid);
  EXPECT_OID_EQ(bucket_indx_oid, action_under_test_with_oid->index_oid);
  EXPECT_OID_EQ(zero_oid, action_under_test->old_oid);
  EXPECT_OID_EQ(M0_CLOVIS_ID_APP, action_under_test->oid);
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
  index_name = action_under_test->index_name;
  EXPECT_STREQ("Multipart", index_name.substr(index_name.length() - 9).c_str());
  EXPECT_STREQ("1234-1234", action_under_test->upload_id.c_str());
  EXPECT_STREQ(
      "1234-1234",
      action_under_test->system_defined_attribute["Upload-ID"].c_str());
}

TEST_F(S3ObjectMetadataTest, GetSet) {
  struct m0_uint128 myoid = {0xfff, 0xfff};
  action_under_test->system_defined_attribute["Owner-User-id"] = "s3owner";
  EXPECT_STREQ("s3owner", action_under_test->get_owner_id().c_str());
  action_under_test->system_defined_attribute["Owner-User"] = "s3user";
  EXPECT_STREQ("s3user", action_under_test->get_owner_name().c_str());
  action_under_test->system_defined_attribute["Last-Modified"] =
      "2016-10-18T16:01:01.000Z";
  EXPECT_STREQ("2016-10-18T16:01:01.000Z",
               action_under_test->get_last_modified().c_str());
  std::string gmt_time = action_under_test->get_last_modified_gmt();
  EXPECT_STREQ("GMT", gmt_time.substr(gmt_time.length() - 3).c_str());
  std::string iso_time =
      action_under_test->system_defined_attribute["Last-Modified"];
  EXPECT_STREQ("00Z", iso_time.substr(iso_time.length() - 3).c_str());
  EXPECT_STREQ("STANDARD", action_under_test->get_storage_class().c_str());
  action_under_test->set_content_length("100");
  EXPECT_STREQ("100", action_under_test->get_content_length_str().c_str());
  EXPECT_EQ(100, action_under_test->get_content_length());
  action_under_test->set_md5("avbxy");
  EXPECT_STREQ("avbxy", action_under_test->get_md5().c_str());

  action_under_test->set_oid(myoid);
  EXPECT_OID_EQ(myoid, action_under_test->oid);
  EXPECT_TRUE(action_under_test->mero_oid_u_hi_str != "");
  EXPECT_TRUE(action_under_test->mero_oid_u_lo_str != "");

  action_under_test->index_oid = myoid;
  EXPECT_OID_EQ(myoid, action_under_test->get_index_oid());

  action_under_test->set_old_oid(myoid);
  EXPECT_OID_EQ(myoid, action_under_test->old_oid);
  EXPECT_TRUE(action_under_test->mero_old_oid_u_hi_str != "");
  EXPECT_TRUE(action_under_test->mero_old_oid_u_lo_str != "");

  action_under_test->set_part_index_oid(myoid);
  EXPECT_OID_EQ(myoid, action_under_test->part_index_oid);
  EXPECT_TRUE(action_under_test->mero_part_oid_u_hi_str != "");
  EXPECT_TRUE(action_under_test->mero_part_oid_u_lo_str != "");

  action_under_test->bucket_name = "seagatebucket";
  EXPECT_STREQ("BUCKET/seagatebucket",
               action_under_test->get_bucket_index_name().c_str());

  EXPECT_STREQ("BUCKET/seagatebucket/Multipart",
               action_under_test->get_multipart_index_name().c_str());
}

TEST_F(S3MultipartObjectMetadataTest, GetUserIdUplodIdName) {
  EXPECT_STREQ("123", action_under_test->user_id.c_str());
  EXPECT_STREQ("1234-1234", action_under_test->upload_id.c_str());
  EXPECT_STREQ("s3user", action_under_test->user_name.c_str());
}

TEST_F(S3ObjectMetadataTest, SetAcl) {
  action_under_test->system_defined_attribute["Owner-User"] = "s3user";
  char expected_str[] =
      "<Owner>\n<ID>1</ID>\n\n<DisplayName>s3user</"
      "DisplayName>\n</Owner>";
  std::string acl = DUMMY_ACL_STR;
  action_under_test->setacl(acl);
  EXPECT_STREQ(expected_str,
               action_under_test->object_ACL.get_xml_str().c_str());
}

TEST_F(S3ObjectMetadataTest, AddSystemAttribute) {
  action_under_test->add_system_attribute("LocationConstraint", "us-east");
  EXPECT_STREQ("us-east",
               action_under_test->system_defined_attribute["LocationConstraint"]
                   .c_str());
}

TEST_F(S3ObjectMetadataTest, AddUserDefinedAttribute) {
  action_under_test->add_user_defined_attribute("Etag", "1234");
  EXPECT_STREQ("1234",
               action_under_test->user_defined_attribute["Etag"].c_str());
}

TEST_F(S3ObjectMetadataTest, Load) {
  EXPECT_CALL(*(clovis_kvs_reader_factory->mock_clovis_kvs_reader),
              get_keyval(_, "", _, _))
      .Times(1);
  action_under_test->load(
      std::bind(&S3CallBack::on_success, &s3objectmetadata_callbackobj),
      std::bind(&S3CallBack::on_failed, &s3objectmetadata_callbackobj));

  EXPECT_TRUE(action_under_test->handler_on_success != nullptr);
  EXPECT_TRUE(action_under_test->handler_on_failed != nullptr);
}

TEST_F(S3ObjectMetadataTest, LoadSuccessful) {
  action_under_test->clovis_kv_reader =
      clovis_kvs_reader_factory->mock_clovis_kvs_reader;

  action_under_test->handler_on_success =
      std::bind(&S3CallBack::on_success, &s3objectmetadata_callbackobj);

  EXPECT_CALL(*(clovis_kvs_reader_factory->mock_clovis_kvs_reader), get_value())
      .WillRepeatedly(Return(
          "{\"Bucket-Name\":\"seagate_bucket\",\"Object-Name\":\"3kfile\"}"));
  action_under_test->load_successful();
  EXPECT_EQ(action_under_test->state, S3ObjectMetadataState::present);
  EXPECT_TRUE(s3objectmetadata_callbackobj.success_called);
}

TEST_F(S3ObjectMetadataTest, LoadSuccessInvalidJson) {
  action_under_test->clovis_kv_reader =
      clovis_kvs_reader_factory->mock_clovis_kvs_reader;

  action_under_test->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &s3objectmetadata_callbackobj);

  EXPECT_CALL(*(clovis_kvs_reader_factory->mock_clovis_kvs_reader), get_value())
      .WillRepeatedly(Return(""));
  action_under_test->load_successful();
  EXPECT_TRUE(action_under_test->json_parsing_error);
  EXPECT_EQ(action_under_test->state, S3ObjectMetadataState::failed);
  EXPECT_TRUE(s3objectmetadata_callbackobj.fail_called);
}

TEST_F(S3ObjectMetadataTest, LoadObjectInfoFailedJsonParsingFailed) {
  action_under_test->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &s3objectmetadata_callbackobj);
  action_under_test->json_parsing_error = true;
  action_under_test->load_failed();
  EXPECT_TRUE(s3objectmetadata_callbackobj.fail_called);
  EXPECT_EQ(S3ObjectMetadataState::failed, action_under_test->state);
}

TEST_F(S3ObjectMetadataTest, LoadObjectInfoFailedMetadataMissing) {
  action_under_test->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &s3objectmetadata_callbackobj);
  action_under_test->clovis_kv_reader =
      clovis_kvs_reader_factory->mock_clovis_kvs_reader;
  EXPECT_CALL(*(clovis_kvs_reader_factory->mock_clovis_kvs_reader), get_state())
      .Times(1)
      .WillRepeatedly(Return(S3ClovisKVSReaderOpState::missing));
  action_under_test->load_failed();
  EXPECT_TRUE(s3objectmetadata_callbackobj.fail_called);
  EXPECT_EQ(S3ObjectMetadataState::missing, action_under_test->state);
}

TEST_F(S3ObjectMetadataTest, LoadObjectInfoFailedMetadataFailed) {
  action_under_test->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &s3objectmetadata_callbackobj);
  action_under_test->clovis_kv_reader =
      clovis_kvs_reader_factory->mock_clovis_kvs_reader;
  EXPECT_CALL(*(clovis_kvs_reader_factory->mock_clovis_kvs_reader), get_state())
      .Times(1)
      .WillRepeatedly(Return(S3ClovisKVSReaderOpState::failed));
  action_under_test->load_failed();
  EXPECT_TRUE(s3objectmetadata_callbackobj.fail_called);
  EXPECT_EQ(S3ObjectMetadataState::failed, action_under_test->state);
}

TEST_F(S3ObjectMetadataTest, SaveMeatdataMissingIndexOID) {
  struct m0_uint128 zero_oid = {0ULL, 0ULL};
  action_under_test->index_oid = {0ULL, 0ULL};
  EXPECT_CALL(*(clovis_kvs_writer_factory->mock_clovis_kvs_writer),
              create_index_with_oid(_, _, _))
      .Times(1);

  action_under_test->save(
      std::bind(&S3CallBack::on_success, &s3objectmetadata_callbackobj),
      std::bind(&S3CallBack::on_failed, &s3objectmetadata_callbackobj));
  EXPECT_OID_NE(zero_oid, action_under_test->index_oid);
  EXPECT_EQ(S3ObjectMetadataState::missing, action_under_test->state);
}

TEST_F(S3ObjectMetadataTest, SaveMeatdataIndexOIDPresent) {
  action_under_test->index_oid = {0x111f, 0xffff};
  EXPECT_CALL(*(clovis_kvs_writer_factory->mock_clovis_kvs_writer),
              put_keyval(_, _, _, _, _))
      .Times(1);
  action_under_test->save(
      std::bind(&S3CallBack::on_success, &s3objectmetadata_callbackobj),
      std::bind(&S3CallBack::on_failed, &s3objectmetadata_callbackobj));
}

TEST_F(S3ObjectMetadataTest, CreateBucketIndex) {
  EXPECT_CALL(*(clovis_kvs_writer_factory->mock_clovis_kvs_writer),
              create_index_with_oid(_, _, _))
      .Times(1);
  action_under_test->create_bucket_index();
  EXPECT_EQ(S3ObjectMetadataState::missing, action_under_test->state);
}

TEST_F(S3MultipartObjectMetadataTest, CreateBucketIndexSuccessful) {
  action_under_test->index_oid = {0xffff, 0xffff};
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), save(_, _))
      .Times(AtLeast(1));
  action_under_test->create_bucket_index_successful();
  EXPECT_OID_EQ(action_under_test->index_oid,
                action_under_test->bucket_metadata->get_multipart_index_oid());
}

TEST_F(S3ObjectMetadataTest, CreateBucketIndexSuccessful) {
  action_under_test->index_oid = {0xffff, 0xffff};
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), save(_, _))
      .Times(AtLeast(1));
  action_under_test->create_bucket_index_successful();
  EXPECT_OID_EQ(
      action_under_test->index_oid,
      action_under_test->bucket_metadata->get_object_list_index_oid());
}

TEST_F(S3ObjectMetadataTest, SaveObjectListIndexSuccessful) {
  EXPECT_CALL(*(clovis_kvs_writer_factory->mock_clovis_kvs_writer),
              put_keyval(_, _, _, _, _))
      .Times(1);
  action_under_test->save_object_list_index_oid_successful();
}

TEST_F(S3ObjectMetadataTest, SaveObjectListIndexFailed) {
  action_under_test->bucket_metadata =
      action_under_test->bucket_metadata_factory->create_bucket_metadata_obj(
          ptr_mock_request);
  action_under_test->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &s3objectmetadata_callbackobj);
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(S3BucketMetadataState::failed));
  action_under_test->save_object_list_index_oid_failed();
  EXPECT_EQ(S3ObjectMetadataState::failed, action_under_test->state);
  EXPECT_TRUE(s3objectmetadata_callbackobj.fail_called);
}

TEST_F(S3ObjectMetadataTest, SaveObjectListIndexFailedToLaunch) {
  action_under_test->bucket_metadata =
      action_under_test->bucket_metadata_factory->create_bucket_metadata_obj(
          ptr_mock_request);
  action_under_test->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &s3objectmetadata_callbackobj);
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(S3BucketMetadataState::failed_to_launch));
  action_under_test->save_object_list_index_oid_failed();
  EXPECT_EQ(S3ObjectMetadataState::failed_to_launch, action_under_test->state);
  EXPECT_TRUE(s3objectmetadata_callbackobj.fail_called);
}

TEST_F(S3ObjectMetadataTest, CreateBucketListIndexFailedCollisionHappened) {
  struct m0_uint128 old_oid = action_under_test->index_oid;
  action_under_test->clovis_kv_writer =
      clovis_kvs_writer_factory->mock_clovis_kvs_writer;
  action_under_test->tried_count = 1;
  EXPECT_CALL(*(clovis_kvs_writer_factory->mock_clovis_kvs_writer), get_state())
      .Times(1)
      .WillRepeatedly(Return(S3ClovisKVSWriterOpState::exists));

  EXPECT_CALL(*(clovis_kvs_writer_factory->mock_clovis_kvs_writer),
              create_index_with_oid(_, _, _))
      .Times(1);

  action_under_test->create_bucket_index_failed();
  EXPECT_EQ(2, action_under_test->tried_count);
  EXPECT_OID_NE(old_oid, action_under_test->index_oid);
}

TEST_F(S3ObjectMetadataTest, CreateBucketListIndexFailed) {
  action_under_test->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &s3objectmetadata_callbackobj);
  action_under_test->clovis_kv_writer =
      clovis_kvs_writer_factory->mock_clovis_kvs_writer;
  EXPECT_CALL(*(clovis_kvs_writer_factory->mock_clovis_kvs_writer), get_state())
      .Times(2)
      .WillRepeatedly(Return(S3ClovisKVSWriterOpState::failed));
  action_under_test->create_bucket_index_failed();
  EXPECT_TRUE(s3objectmetadata_callbackobj.fail_called);
  EXPECT_EQ(action_under_test->state, S3ObjectMetadataState::failed);
}

TEST_F(S3ObjectMetadataTest, CreateBucketListIndexFailedToLaunch) {
  action_under_test->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &s3objectmetadata_callbackobj);
  action_under_test->clovis_kv_writer =
      clovis_kvs_writer_factory->mock_clovis_kvs_writer;
  EXPECT_CALL(*(clovis_kvs_writer_factory->mock_clovis_kvs_writer), get_state())
      .Times(2)
      .WillRepeatedly(Return(S3ClovisKVSWriterOpState::failed_to_launch));
  action_under_test->create_bucket_index_failed();
  EXPECT_TRUE(s3objectmetadata_callbackobj.fail_called);
  EXPECT_EQ(action_under_test->state, S3ObjectMetadataState::failed_to_launch);
}

TEST_F(S3ObjectMetadataTest, CollisionDetected) {
  struct m0_uint128 old_oid = {0xfff, 0xfff};
  action_under_test->index_oid = old_oid;
  action_under_test->tried_count = 1;
  action_under_test->clovis_kv_writer =
      clovis_kvs_writer_factory->mock_clovis_kvs_writer;
  EXPECT_CALL(*(clovis_kvs_writer_factory->mock_clovis_kvs_writer),
              create_index_with_oid(_, _, _))
      .Times(1);

  action_under_test->collision_detected();
  EXPECT_OID_NE(old_oid, action_under_test->index_oid);
  EXPECT_EQ(2, action_under_test->tried_count);
}

TEST_F(S3ObjectMetadataTest, CollisionDetectedMaxAttemptExceeded) {
  action_under_test->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &s3objectmetadata_callbackobj);
  action_under_test->tried_count = MAX_COLLISION_RETRY_COUNT + 1;
  action_under_test->collision_detected();
  EXPECT_EQ(S3ObjectMetadataState::failed, action_under_test->state);
  EXPECT_EQ(MAX_COLLISION_RETRY_COUNT + 1, action_under_test->tried_count);
}

TEST_F(S3ObjectMetadataTest, CreateNewOid) {
  struct m0_uint128 old_oid = {0xfff, 0xfff};
  action_under_test->index_oid = old_oid;
  action_under_test->create_new_oid();
  EXPECT_OID_NE(action_under_test->index_oid, old_oid);
}

TEST_F(S3ObjectMetadataTest, SaveMetadataWithoutParam) {
  action_under_test->clovis_kv_writer =
      clovis_kvs_writer_factory->mock_clovis_kvs_writer;
  EXPECT_CALL(*(clovis_kvs_writer_factory->mock_clovis_kvs_writer),
              put_keyval(_, _, _, _, _))
      .Times(1);

  action_under_test->save_metadata();
  EXPECT_STREQ(
      "s3user",
      action_under_test->system_defined_attribute["Owner-User"].c_str());
  EXPECT_STREQ(
      "123",
      action_under_test->system_defined_attribute["Owner-User-id"].c_str());
  EXPECT_STREQ(
      "s3account",
      action_under_test->system_defined_attribute["Owner-Account"].c_str());
  EXPECT_STREQ(
      "12345",
      action_under_test->system_defined_attribute["Owner-Account-id"].c_str());
}

TEST_F(S3ObjectMetadataTest, SaveMetadataWithParam) {
  action_under_test->clovis_kv_writer =
      clovis_kvs_writer_factory->mock_clovis_kvs_writer;
  EXPECT_CALL(*(clovis_kvs_writer_factory->mock_clovis_kvs_writer),
              put_keyval(_, _, _, _, _))
      .Times(1);

  action_under_test->save_metadata(
      std::bind(&S3CallBack::on_success, &s3objectmetadata_callbackobj),
      std::bind(&S3CallBack::on_failed, &s3objectmetadata_callbackobj));
  EXPECT_STREQ(
      "s3user",
      action_under_test->system_defined_attribute["Owner-User"].c_str());
  EXPECT_STREQ(
      "123",
      action_under_test->system_defined_attribute["Owner-User-id"].c_str());
  EXPECT_STREQ(
      "s3account",
      action_under_test->system_defined_attribute["Owner-Account"].c_str());
  EXPECT_STREQ(
      "12345",
      action_under_test->system_defined_attribute["Owner-Account-id"].c_str());
  EXPECT_TRUE(action_under_test->handler_on_success != nullptr);
  EXPECT_TRUE(action_under_test->handler_on_failed != nullptr);
}

TEST_F(S3ObjectMetadataTest, SaveMetadataSuccess) {
  action_under_test->handler_on_success =
      std::bind(&S3CallBack::on_success, &s3objectmetadata_callbackobj);
  action_under_test->save_metadata_successful();
  EXPECT_TRUE(s3objectmetadata_callbackobj.success_called);
  EXPECT_EQ(S3ObjectMetadataState::saved, action_under_test->state);
}

TEST_F(S3ObjectMetadataTest, SaveMetadataFailed) {
  action_under_test->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &s3objectmetadata_callbackobj);
  action_under_test->save_metadata_failed();
  EXPECT_TRUE(s3objectmetadata_callbackobj.fail_called);
  EXPECT_EQ(S3ObjectMetadataState::failed, action_under_test->state);
}

TEST_F(S3ObjectMetadataTest, Remove) {
  EXPECT_CALL(*(clovis_kvs_writer_factory->mock_clovis_kvs_writer),
              delete_keyval(_, _, _, _))
      .Times(1);

  action_under_test->remove(
      std::bind(&S3CallBack::on_success, &s3objectmetadata_callbackobj),
      std::bind(&S3CallBack::on_failed, &s3objectmetadata_callbackobj));
  EXPECT_TRUE(action_under_test->handler_on_success != NULL);
  EXPECT_TRUE(action_under_test->handler_on_failed != NULL);
}

TEST_F(S3ObjectMetadataTest, RemoveSuccessful) {
  action_under_test->handler_on_success =
      std::bind(&S3CallBack::on_success, &s3objectmetadata_callbackobj);

  action_under_test->remove_successful();
  EXPECT_EQ(S3ObjectMetadataState::deleted, action_under_test->state);
  EXPECT_TRUE(s3objectmetadata_callbackobj.success_called);
}

TEST_F(S3ObjectMetadataTest, RemoveFailed) {
  action_under_test->clovis_kv_writer =
      clovis_kvs_writer_factory->mock_clovis_kvs_writer;
  action_under_test->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &s3objectmetadata_callbackobj);
  EXPECT_CALL(*(clovis_kvs_writer_factory->mock_clovis_kvs_writer), get_state())
      .Times(1)
      .WillRepeatedly(Return(S3ClovisKVSWriterOpState::failed));
  action_under_test->remove_failed();
  EXPECT_EQ(S3ObjectMetadataState::failed, action_under_test->state);
  EXPECT_TRUE(s3objectmetadata_callbackobj.fail_called);
}

TEST_F(S3ObjectMetadataTest, RemoveFailedToLaunch) {
  action_under_test->clovis_kv_writer =
      clovis_kvs_writer_factory->mock_clovis_kvs_writer;
  action_under_test->handler_on_failed =
      std::bind(&S3CallBack::on_failed, &s3objectmetadata_callbackobj);
  EXPECT_CALL(*(clovis_kvs_writer_factory->mock_clovis_kvs_writer), get_state())
      .Times(1)
      .WillRepeatedly(Return(S3ClovisKVSWriterOpState::failed_to_launch));
  action_under_test->remove_failed();
  EXPECT_EQ(S3ObjectMetadataState::failed_to_launch, action_under_test->state);
  EXPECT_TRUE(s3objectmetadata_callbackobj.fail_called);
}

TEST_F(S3ObjectMetadataTest, CreateDefaultAcl) {
  EXPECT_STREQ(DEFAULT_ACL_STR,
               action_under_test->create_default_acl().c_str());
}

TEST_F(S3ObjectMetadataTest, ToJson) {
  std::string json_str = action_under_test->to_json();
  Json::Value newroot;
  Json::Reader reader;
  bool parsingSuccessful = reader.parse(json_str, newroot);
  EXPECT_TRUE(parsingSuccessful);
}

TEST_F(S3ObjectMetadataTest, FromJson) {
  int ret_status;
  std::string json_str =
      "{\"ACL\":\"PD94+Cg==\",\"Bucket-Name\":\"seagatebucket\"}";

  ret_status = action_under_test->from_json(json_str);
  EXPECT_TRUE(ret_status == 0);
}

TEST_F(S3MultipartObjectMetadataTest, FromJson) {
  int ret_status;
  std::string json_str =
      "{\"ACL\":\"PD94+Cg==\",\"Bucket-Name\":\"seagatebucket\",\"mero_old_oid_"
      "u_hi\":\"123\",\"mero_old_oid_u_lo\":\"456\"}";

  ret_status = action_under_test->from_json(json_str);
  EXPECT_TRUE(ret_status == 0);
  EXPECT_STREQ("seagatebucket", action_under_test->bucket_name.c_str());
  EXPECT_STREQ("123", action_under_test->mero_old_oid_u_hi_str.c_str());
  EXPECT_STREQ("456", action_under_test->mero_old_oid_u_lo_str.c_str());
}

TEST_F(S3ObjectMetadataTest, GetEncodedBucketAcl) {
  std::string json_str = "{\"ACL\":\"PD94bg==\"}";

  action_under_test->from_json(json_str);
  EXPECT_STREQ("PD94bg==", action_under_test->get_encoded_object_acl().c_str());
}
