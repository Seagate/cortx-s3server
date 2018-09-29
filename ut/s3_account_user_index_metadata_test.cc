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
 * Original author:  Abrarahmed Momin   <abrar.habib@seagate.com>
 * Original creation date: 15-May-2017
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "mock_s3_bucket_metadata.h"
#include "mock_s3_clovis_wrapper.h"
#include "mock_s3_factory.h"
#include "mock_s3_request_object.h"
#include "s3_account_user_index_metadata.h"
#include "s3_callback_test_helpers.h"
#include "s3_test_utils.h"

using ::testing::Invoke;
using ::testing::AtLeast;
using ::testing::Return;
using ::testing::_;

#define CREATE_KVS_WRITER_OBJ                                             \
  do {                                                                    \
    idx_metadata_under_test_ptr->clovis_kv_writer =                       \
        idx_metadata_under_test_ptr->clovis_kvs_writer_factory            \
            ->create_clovis_kvs_writer(request_mock, s3_clovis_api_mock); \
  } while (0)

#define CREATE_KVS_READER_OBJ                                             \
  do {                                                                    \
    idx_metadata_under_test_ptr->clovis_kv_reader =                       \
        idx_metadata_under_test_ptr->clovis_kvs_reader_factory            \
            ->create_clovis_kvs_reader(request_mock, s3_clovis_api_mock); \
  } while (0)

#define CREATE_ACTION_UNDER_TEST_OBJ                                          \
  do {                                                                        \
    request_mock->set_user_id(user_id);                                       \
    request_mock->set_account_name(account_name);                             \
    request_mock->set_account_id(account_id);                                 \
    request_mock->set_user_name(user_name);                                   \
    idx_metadata_under_test_ptr = std::make_shared<S3AccountUserIdxMetadata>( \
        request_mock, s3_clovis_api_mock, clovis_kvs_reader_factory,          \
        clovis_kvs_writer_factory);                                           \
  } while (0)

class S3AccountUserIdxMetadataTest : public testing::Test {
 protected:  // You should make the members protected s.t. they can be
             // accessed from sub-classes.
  S3AccountUserIdxMetadataTest() {
    evhtp_request_t *req = NULL;
    EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();
    bucket_list_indx_oid = {0x11ffff, 0x1ffff};
    request_mock = std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr);
    s3_clovis_api_mock = std::make_shared<MockS3Clovis>();

    user_name.assign("tester");
    user_id.assign("123");
    account_id.assign("12345");
    account_name.assign("s3_test");
    call_count_one = 0;
    clovis_kvs_reader_factory = std::make_shared<MockS3ClovisKVSReaderFactory>(
        request_mock, s3_clovis_api_mock);
    clovis_kvs_writer_factory =
        std::make_shared<MockS3ClovisKVSWriterFactory>(request_mock);
    CREATE_ACTION_UNDER_TEST_OBJ;
  }

  std::shared_ptr<MockS3RequestObject> request_mock;
  std::shared_ptr<S3AccountUserIdxMetadata> idx_metadata_under_test_ptr;
  std::shared_ptr<MockS3ClovisKVSReaderFactory> clovis_kvs_reader_factory;
  std::shared_ptr<MockS3ClovisKVSWriterFactory> clovis_kvs_writer_factory;
  std::shared_ptr<MockS3Clovis> s3_clovis_api_mock;

  S3CallBack s3accountuserindex_callbackobj;
  std::string account_name;
  std::string account_id;
  std::string user_name;
  std::string user_id;
  struct m0_uint128 bucket_list_indx_oid;
  std::string mock_json_string;
  int call_count_one;

 public:
  void func_callback_one() { call_count_one += 1; }
};

TEST_F(S3AccountUserIdxMetadataTest, Constructor) {
  struct m0_uint128 zero_oid = {0ULL, 0ULL};
  EXPECT_TRUE(idx_metadata_under_test_ptr->clovis_kvs_reader_factory !=
              nullptr);
  EXPECT_TRUE(idx_metadata_under_test_ptr->clovis_kvs_writer_factory !=
              nullptr);
  EXPECT_OID_EQ(zero_oid, idx_metadata_under_test_ptr->bucket_list_index_oid);
  EXPECT_STREQ(account_name.c_str(),
               idx_metadata_under_test_ptr->account_name.c_str());
  EXPECT_STREQ(account_id.c_str(),
               idx_metadata_under_test_ptr->account_id.c_str());
  EXPECT_STREQ(user_name.c_str(),
               idx_metadata_under_test_ptr->user_name.c_str());
  EXPECT_STREQ(user_id.c_str(), idx_metadata_under_test_ptr->user_id.c_str());
}

TEST_F(S3AccountUserIdxMetadataTest, Load) {
  EXPECT_CALL(*(clovis_kvs_reader_factory->mock_clovis_kvs_reader),
              get_keyval(_, "ACCOUNTUSER/" + account_id, _, _))
      .Times(AtLeast(1));
  idx_metadata_under_test_ptr->load(
      std::bind(&S3CallBack::on_success, &s3accountuserindex_callbackobj),
      std::bind(&S3CallBack::on_failed, &s3accountuserindex_callbackobj));
  EXPECT_EQ(S3AccountUserIdxMetadataState::missing,
            idx_metadata_under_test_ptr->state);
}

TEST_F(S3AccountUserIdxMetadataTest, FromJson) {
  mock_json_string.assign(
      "{\"account_id\":\"12345\",\"account_name\":\"s3_test\",\"bucket_list_"
      "index_oid_u_hi\":\"VQSwBgAAAHg=\",\"bucket_list_index_oid_u_lo\":"
      "\"sIWaUHl/59A=\",\"user_id\":\"123\",\"user_name\":\"tester\"}");

  EXPECT_EQ(0, idx_metadata_under_test_ptr->from_json(mock_json_string));
}

TEST_F(S3AccountUserIdxMetadataTest, FromJsonError) {
  mock_json_string.assign(
      "{\"account_id\":\"12345\",\"account_name\":\"s3_test\",\"bucket_list_"
      "index_oid_u_hi\":\"VQSwBgAAAHg=\",\"bucket_list_index_oid_u_lo\":"
      "\"sIWaUHl/59A=\",\"user_id\":\"123\",\"user_name\"tester\"}");

  EXPECT_EQ(-1, idx_metadata_under_test_ptr->from_json(mock_json_string));
}

TEST_F(S3AccountUserIdxMetadataTest, LoadSuccessful) {
  CREATE_KVS_READER_OBJ;
  mock_json_string.assign(
      "{\"account_id\":\"12345\",\"account_name\":\"s3_test\",\"bucket_list_"
      "index_oid_u_hi\":\"VQSwBgAAAHg=\",\"bucket_list_index_oid_u_lo\":"
      "\"sIWaUHl/59A=\",\"user_id\":\"123\",\"user_name\":\"tester\"}");

  EXPECT_CALL(*(clovis_kvs_reader_factory->mock_clovis_kvs_reader), get_value())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(mock_json_string));

  idx_metadata_under_test_ptr->handler_on_success =
      std::bind(&S3AccountUserIdxMetadataTest::func_callback_one, this);
  idx_metadata_under_test_ptr->load_successful();
  EXPECT_EQ(S3AccountUserIdxMetadataState::present,
            idx_metadata_under_test_ptr->state);
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3AccountUserIdxMetadataTest, LoadSuccessfulJsonError) {
  CREATE_KVS_READER_OBJ;
  mock_json_string.assign(
      "{\"account_id\":\"12345\",\"account_name\":\"s3_test\",\"bucket_list_"
      "index_oid_u_hi\":\"VQSwBgAAAHg=\",\"bucket_list_index_oid_u_lo\":"
      "\"sIWaUHl/59A=\",\"user_id\":\"123\",\"user_name\"tester\"}");

  EXPECT_CALL(*(clovis_kvs_reader_factory->mock_clovis_kvs_reader), get_value())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(mock_json_string));

  idx_metadata_under_test_ptr->handler_on_failed =
      std::bind(&S3AccountUserIdxMetadataTest::func_callback_one, this);
  idx_metadata_under_test_ptr->load_successful();
  EXPECT_EQ(S3AccountUserIdxMetadataState::failed,
            idx_metadata_under_test_ptr->state);
  EXPECT_TRUE(idx_metadata_under_test_ptr->json_parsing_error);
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3AccountUserIdxMetadataTest, LoadFailed) {
  CREATE_KVS_READER_OBJ;
  EXPECT_CALL(*(clovis_kvs_reader_factory->mock_clovis_kvs_reader), get_state())
      .WillRepeatedly(Return(S3ClovisKVSReaderOpState::failed));

  idx_metadata_under_test_ptr->handler_on_failed =
      std::bind(&S3AccountUserIdxMetadataTest::func_callback_one, this);
  idx_metadata_under_test_ptr->load_failed();
  EXPECT_EQ(S3AccountUserIdxMetadataState::failed,
            idx_metadata_under_test_ptr->state);
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3AccountUserIdxMetadataTest, LoadFailedMissing) {
  CREATE_KVS_READER_OBJ;
  EXPECT_CALL(*(clovis_kvs_reader_factory->mock_clovis_kvs_reader), get_state())
      .WillRepeatedly(Return(S3ClovisKVSReaderOpState::missing));

  idx_metadata_under_test_ptr->handler_on_failed =
      std::bind(&S3AccountUserIdxMetadataTest::func_callback_one, this);
  idx_metadata_under_test_ptr->load_failed();
  EXPECT_EQ(S3AccountUserIdxMetadataState::missing,
            idx_metadata_under_test_ptr->state);
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3AccountUserIdxMetadataTest, SaveSuccessful) {
  idx_metadata_under_test_ptr->handler_on_success =
      std::bind(&S3AccountUserIdxMetadataTest::func_callback_one, this);
  idx_metadata_under_test_ptr->save_successful();
  EXPECT_EQ(S3AccountUserIdxMetadataState::saved,
            idx_metadata_under_test_ptr->state);
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3AccountUserIdxMetadataTest, SaveFailed) {
  CREATE_KVS_WRITER_OBJ;
  EXPECT_CALL(*(clovis_kvs_writer_factory->mock_clovis_kvs_writer), get_state())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(S3ClovisKVSWriterOpState::failed));
  idx_metadata_under_test_ptr->handler_on_failed =
      std::bind(&S3AccountUserIdxMetadataTest::func_callback_one, this);
  idx_metadata_under_test_ptr->save_failed();
  EXPECT_EQ(S3AccountUserIdxMetadataState::failed,
            idx_metadata_under_test_ptr->state);
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3AccountUserIdxMetadataTest, SaveFailedToLaunch) {
  CREATE_KVS_WRITER_OBJ;
  EXPECT_CALL(*(clovis_kvs_writer_factory->mock_clovis_kvs_writer), get_state())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(S3ClovisKVSWriterOpState::failed_to_launch));
  idx_metadata_under_test_ptr->handler_on_failed =
      std::bind(&S3AccountUserIdxMetadataTest::func_callback_one, this);
  idx_metadata_under_test_ptr->save_failed();
  EXPECT_EQ(S3AccountUserIdxMetadataState::failed_to_launch,
            idx_metadata_under_test_ptr->state);
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3AccountUserIdxMetadataTest, RemoveSuccessful) {
  idx_metadata_under_test_ptr->handler_on_success =
      std::bind(&S3AccountUserIdxMetadataTest::func_callback_one, this);
  idx_metadata_under_test_ptr->remove_successful();
  EXPECT_EQ(S3AccountUserIdxMetadataState::deleted,
            idx_metadata_under_test_ptr->state);
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3AccountUserIdxMetadataTest, RemoveFailed) {
  CREATE_KVS_WRITER_OBJ;
  EXPECT_CALL(*(clovis_kvs_writer_factory->mock_clovis_kvs_writer), get_state())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(S3ClovisKVSWriterOpState::failed));
  idx_metadata_under_test_ptr->handler_on_failed =
      std::bind(&S3AccountUserIdxMetadataTest::func_callback_one, this);
  idx_metadata_under_test_ptr->remove_failed();
  EXPECT_EQ(S3AccountUserIdxMetadataState::failed,
            idx_metadata_under_test_ptr->state);
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3AccountUserIdxMetadataTest, RemoveFailedToLaunch) {
  CREATE_KVS_WRITER_OBJ;
  EXPECT_CALL(*(clovis_kvs_writer_factory->mock_clovis_kvs_writer), get_state())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(S3ClovisKVSWriterOpState::failed_to_launch));
  idx_metadata_under_test_ptr->handler_on_failed =
      std::bind(&S3AccountUserIdxMetadataTest::func_callback_one, this);
  idx_metadata_under_test_ptr->remove_failed();
  EXPECT_EQ(S3AccountUserIdxMetadataState::failed_to_launch,
            idx_metadata_under_test_ptr->state);
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3AccountUserIdxMetadataTest, ToJson) {
  mock_json_string.assign(
      "{\"account_id\":\"12345\",\"account_name\":\"s3_test\",\"bucket_list_"
      "index_oid_u_hi\":\"AAAAAAAAAAA=\",\"bucket_list_index_oid_u_lo\":"
      "\"AAAAAAAAAAA=\",\"user_id\":\"123\",\"user_name\":\"tester\"}\n");

  EXPECT_STREQ(mock_json_string.c_str(),
               idx_metadata_under_test_ptr->to_json().c_str());
}

TEST_F(S3AccountUserIdxMetadataTest, Save) {
  CREATE_KVS_WRITER_OBJ;
  EXPECT_CALL(*(clovis_kvs_writer_factory->mock_clovis_kvs_writer),
              put_keyval(_, _, _, _, _))
      .Times(1);

  idx_metadata_under_test_ptr->save(
      std::bind(&S3CallBack::on_success, &s3accountuserindex_callbackobj),
      std::bind(&S3CallBack::on_failed, &s3accountuserindex_callbackobj));
  EXPECT_EQ(S3AccountUserIdxMetadataState::missing,
            idx_metadata_under_test_ptr->state);
}

TEST_F(S3AccountUserIdxMetadataTest, Remove) {
  CREATE_KVS_WRITER_OBJ;
  EXPECT_CALL(*(clovis_kvs_writer_factory->mock_clovis_kvs_writer),
              delete_keyval(_, _, _, _))
      .Times(1);

  idx_metadata_under_test_ptr->remove(
      std::bind(&S3CallBack::on_success, &s3accountuserindex_callbackobj),
      std::bind(&S3CallBack::on_failed, &s3accountuserindex_callbackobj));
}
