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
 * Original creation date: May-8-2017
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "mock_s3_bucket_metadata.h"
#include "mock_s3_clovis_wrapper.h"
#include "mock_s3_factory.h"
#include "s3_post_complete_action.h"

using ::testing::Invoke;
using ::testing::AtLeast;
using ::testing::ReturnRef;
using ::testing::Return;
using ::testing::_;

#define CREATE_BUCKET_METADATA_OBJ                      \
  do {                                                  \
    action_under_test_ptr->bucket_metadata =            \
        action_under_test_ptr->bucket_metadata_factory  \
            ->create_bucket_metadata_obj(request_mock); \
  } while (0)

#define CREATE_PART_METADATA_OBJ                                             \
  do {                                                                       \
    action_under_test_ptr->part_metadata =                                   \
        action_under_test_ptr->part_metadata_factory                         \
            ->create_part_metadata_obj(request_mock, mp_indx_oid, upload_id, \
                                       0);                                   \
  } while (0)

#define CREATE_MP_METADATA_OBJ                                               \
  do {                                                                       \
    action_under_test_ptr->multipart_metadata =                              \
        action_under_test_ptr->object_mp_metadata_factory                    \
            ->create_object_mp_metadata_obj(request_mock, mp_indx_oid, true, \
                                            upload_id);                      \
  } while (0)

#define CREATE_METADATA_OBJ                                                   \
  do {                                                                        \
    action_under_test_ptr->object_metadata =                                  \
        action_under_test_ptr->object_metadata_factory                        \
            ->create_object_metadata_obj(request_mock, object_list_indx_oid); \
  } while (0)

#define CREATE_KVS_READER_OBJ                                             \
  do {                                                                    \
    action_under_test_ptr->clovis_kv_reader =                             \
        action_under_test_ptr->s3_clovis_kvs_reader_factory               \
            ->create_clovis_kvs_reader(request_mock, s3_clovis_api_mock); \
  } while (0)

#define CREATE_WRITER_OBJ                                                   \
  do {                                                                      \
    action_under_test_ptr->clovis_writer =                                  \
        action_under_test_ptr->clovis_writer_factory->create_clovis_writer( \
            request_mock, oid);                                             \
  } while (0)

#define CREATE_ACTION_UNDER_TEST_OBJ                                      \
  do {                                                                    \
    EXPECT_CALL(*request_mock, get_query_string_value("uploadId"))        \
        .Times(AtLeast(1))                                                \
        .WillRepeatedly(Return("uploadId"));                              \
    action_under_test_ptr = std::make_shared<S3PostCompleteAction>(       \
        request_mock, s3_clovis_api_mock, clovis_kvs_reader_factory,      \
        bucket_meta_factory, object_meta_factory, object_mp_meta_factory, \
        part_meta_factory, clovis_writer_factory);                        \
  } while (0)

class S3PostCompleteActionTest : public testing::Test {
 protected:  // You should make the members protected s.t. they can be
             // accessed from sub-classes.
  S3PostCompleteActionTest() {
    evhtp_request_t *req = NULL;
    EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();
    upload_id = "upload_id";
    mp_indx_oid = {0xffff, 0xffff};
    oid = {0xfff, 0xfff};
    object_list_indx_oid = {0x11ffff, 0x1ffff};
    request_mock = std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr);
    s3_clovis_api_mock = std::make_shared<MockS3Clovis>();
    call_count_one = 0;
    layout_id =
        S3ClovisLayoutMap::get_instance()->get_best_layout_for_object_size();

    bucket_meta_factory =
        std::make_shared<MockS3BucketMetadataFactory>(request_mock);
    clovis_kvs_reader_factory = std::make_shared<MockS3ClovisKVSReaderFactory>(
        request_mock, s3_clovis_api_mock);
    object_meta_factory = std::make_shared<MockS3ObjectMetadataFactory>(
        request_mock, object_list_indx_oid);
    clovis_writer_factory =
        std::make_shared<MockS3ClovisWriterFactory>(request_mock);
    object_mp_meta_factory =
        std::make_shared<MockS3ObjectMultipartMetadataFactory>(
            request_mock, mp_indx_oid, true, upload_id);
    part_meta_factory = std::make_shared<MockS3PartMetadataFactory>(
        request_mock, oid, upload_id, 0);

    CREATE_ACTION_UNDER_TEST_OBJ;
  }

  std::shared_ptr<MockS3RequestObject> request_mock;
  std::shared_ptr<ClovisAPI> s3_clovis_api_mock;
  std::shared_ptr<MockS3ClovisKVSReaderFactory> clovis_kvs_reader_factory;
  std::shared_ptr<MockS3BucketMetadataFactory> bucket_meta_factory;
  std::shared_ptr<MockS3ObjectMetadataFactory> object_meta_factory;
  std::shared_ptr<MockS3ObjectMultipartMetadataFactory> object_mp_meta_factory;
  std::shared_ptr<MockS3PartMetadataFactory> part_meta_factory;
  std::shared_ptr<MockS3ClovisWriterFactory> clovis_writer_factory;
  std::shared_ptr<S3PostCompleteAction> action_under_test_ptr;

  std::string upload_id;
  struct m0_uint128 oid;
  struct m0_uint128 object_list_indx_oid;
  struct m0_uint128 mp_indx_oid;
  std::map<std::string, std::pair<int, std::string>> result_keys_values;
  std::map<std::string, std::string, S3NumStrComparator> mock_parts;
  int call_count_one;
  std::string mock_xml;
  int layout_id;

 public:
  void func_callback_one() { call_count_one += 1; }
};

TEST_F(S3PostCompleteActionTest, Constructor) {
  EXPECT_NE(0, action_under_test_ptr->number_of_tasks());
  EXPECT_TRUE(action_under_test_ptr->bucket_metadata_factory != nullptr);
  EXPECT_TRUE(action_under_test_ptr->s3_clovis_kvs_reader_factory != nullptr);
  EXPECT_TRUE(action_under_test_ptr->object_metadata_factory != nullptr);
  EXPECT_STREQ("uploadId", action_under_test_ptr->upload_id.c_str());
  EXPECT_EQ(0, action_under_test_ptr->object_size);
}

TEST_F(S3PostCompleteActionTest, FetchBucketInfo) {
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), load(_, _))
      .Times(AtLeast(1));
  action_under_test_ptr->fetch_bucket_info();
}

TEST_F(S3PostCompleteActionTest, LoadValidateRequestMoreContent) {
  EXPECT_CALL(*request_mock, get_data_length())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(1024));
  EXPECT_CALL(*request_mock, has_all_body_content())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(*request_mock, listen_for_incoming_data(_, _)).Times(1);
  action_under_test_ptr->load_and_validate_request();
}

TEST_F(S3PostCompleteActionTest, LoadValidateRequestMalformed) {
  mock_xml.assign("");
  EXPECT_CALL(*request_mock, get_data_length())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(1024));
  EXPECT_CALL(*request_mock, has_all_body_content())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*request_mock, get_full_body_content_as_string())
      .WillOnce(ReturnRef(mock_xml));

  EXPECT_CALL(*request_mock, resume()).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(400, _)).Times(AtLeast(1));
  action_under_test_ptr->load_and_validate_request();
  EXPECT_STREQ("MalformedXML",
               action_under_test_ptr->get_s3_error_code().c_str());
}

TEST_F(S3PostCompleteActionTest, LoadValidateRequest) {
  mock_xml.assign(
      "<CompleteMultipartUpload><Part><PartNumber>1</"
      "PartNumber><ETag>0d8de6aaeeb86de16c2062527741789f</ETag></"
      "Part><Part><PartNumber>2</"
      "PartNumber><ETag>1b28ffd7a838d8a9d8adb2d3f521b375</ETag></"
      "Part><Part><PartNumber>3</"
      "PartNumber><ETag>032b37805678903a8a16310a61c51d09</ETag></"
      "Part><Part><PartNumber>4</"
      "PartNumber><ETag>e3a80cd7a7286d28e74b1d5c358207a9</ETag></Part></"
      "CompleteMultipartUpload>");
  EXPECT_CALL(*request_mock, get_data_length())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(1024));
  EXPECT_CALL(*request_mock, has_all_body_content())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*request_mock, get_full_body_content_as_string())
      .WillOnce(ReturnRef(mock_xml));
  action_under_test_ptr->clear_tasks();
  action_under_test_ptr->add_task(
      std::bind(&S3PostCompleteActionTest::func_callback_one, this));

  action_under_test_ptr->load_and_validate_request();
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3PostCompleteActionTest, LoadValidateRequestNoContent) {
  EXPECT_CALL(*request_mock, get_data_length()).Times(1).WillOnce(Return(0));
  EXPECT_CALL(*request_mock, resume()).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(400, _)).Times(AtLeast(1));

  action_under_test_ptr->load_and_validate_request();
  EXPECT_STREQ("MalformedXML",
               action_under_test_ptr->get_s3_error_code().c_str());
}

TEST_F(S3PostCompleteActionTest, ConsumeIncomingContentMoreContent) {
  EXPECT_CALL(*request_mock, has_all_body_content())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(false));
  EXPECT_CALL(*request_mock, resume()).Times(1);

  action_under_test_ptr->consume_incoming_content();
}

TEST_F(S3PostCompleteActionTest, ValidateRequestBodyEmpty) {
  mock_xml.assign("");
  EXPECT_FALSE(action_under_test_ptr->validate_request_body(mock_xml));
}

TEST_F(S3PostCompleteActionTest, ValidateRequestBody) {
  mock_xml.assign(
      "<CompleteMultipartUpload><Part><PartNumber>1</"
      "PartNumber><ETag>0d8de6aaeeb86de16c2062527741789f</ETag></"
      "Part><Part><PartNumber>2</"
      "PartNumber><ETag>1b28ffd7a838d8a9d8adb2d3f521b375</ETag></"
      "Part><Part><PartNumber>3</"
      "PartNumber><ETag>032b37805678903a8a16310a61c51d09</ETag></"
      "Part><Part><PartNumber>4</"
      "PartNumber><ETag>e3a80cd7a7286d28e74b1d5c358207a9</ETag></Part></"
      "CompleteMultipartUpload>");

  EXPECT_TRUE(action_under_test_ptr->validate_request_body(mock_xml));
  EXPECT_STREQ("4", action_under_test_ptr->total_parts.c_str());
}

TEST_F(S3PostCompleteActionTest, ValidateRequestBodyOutOrderParts) {
  mock_xml.assign(
      "<CompleteMultipartUpload><Part><PartNumber>1</"
      "PartNumber><ETag>0d8de6aaeeb86de16c2062527741789f</ETag></"
      "Part><Part><PartNumber>2</"
      "PartNumber><ETag>1b28ffd7a838d8a9d8adb2d3f521b375</ETag></"
      "Part><Part><PartNumber>4</"
      "PartNumber><ETag>032b37805678903a8a16310a61c51d09</ETag></"
      "Part><Part><PartNumber>3</"
      "PartNumber><ETag>e3a80cd7a7286d28e74b1d5c358207a9</ETag></Part></"
      "CompleteMultipartUpload>");

  EXPECT_FALSE(action_under_test_ptr->validate_request_body(mock_xml));
  EXPECT_STREQ("", action_under_test_ptr->total_parts.c_str());
}

TEST_F(S3PostCompleteActionTest, ValidateRequestBodyMissingTag) {
  mock_xml.assign(
      "<CompleteMultipartUpload><Part><PartNumber>1</"
      "PartNumber><ETag>0d8de6aaeeb86de16c2062527741789f</ETag></"
      "Part><Part><PartNumber>2</PartNumber></Part><Part><PartNumber>3</"
      "PartNumber><ETag>032b37805678903a8a16310a61c51d09</ETag></"
      "Part><Part><PartNumber>4</"
      "PartNumber><ETag>e3a80cd7a7286d28e74b1d5c358207a9</ETag></Part></"
      "CompleteMultipartUpload>");

  EXPECT_FALSE(action_under_test_ptr->validate_request_body(mock_xml));
  EXPECT_STREQ("", action_under_test_ptr->total_parts.c_str());
}

TEST_F(S3PostCompleteActionTest, FetchBucketInfoFailedMissing) {
  CREATE_BUCKET_METADATA_OBJ;
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(S3BucketMetadataState::missing));
  EXPECT_CALL(*request_mock, resume()).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(404, _)).Times(AtLeast(1));
  action_under_test_ptr->fetch_bucket_info_failed();
  EXPECT_STREQ("NoSuchBucket",
               action_under_test_ptr->get_s3_error_code().c_str());
}

TEST_F(S3PostCompleteActionTest, FetchBucketInfoFailedInternalError) {
  CREATE_BUCKET_METADATA_OBJ;
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(S3BucketMetadataState::failed));
  EXPECT_CALL(*request_mock, resume()).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(500, _)).Times(AtLeast(1));
  action_under_test_ptr->fetch_bucket_info_failed();
  EXPECT_STREQ("InternalError",
               action_under_test_ptr->get_s3_error_code().c_str());
}

TEST_F(S3PostCompleteActionTest, FetchMultipartInfo) {
  CREATE_BUCKET_METADATA_OBJ;
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), load(_, _))
      .Times(AtLeast(1));
  action_under_test_ptr->fetch_multipart_info();
}

TEST_F(S3PostCompleteActionTest, FetchMultipartInfoFailedInvalidObject) {
  CREATE_MP_METADATA_OBJ;

  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(S3ObjectMetadataState::missing));
  EXPECT_CALL(*request_mock, resume()).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(403, _)).Times(AtLeast(1));
  action_under_test_ptr->fetch_multipart_info_failed();
  EXPECT_STREQ("InvalidObjectState",
               action_under_test_ptr->get_s3_error_code().c_str());
}

TEST_F(S3PostCompleteActionTest, FetchMultipartInfoFailedInternalError) {
  CREATE_MP_METADATA_OBJ;

  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(S3ObjectMetadataState::failed));
  EXPECT_CALL(*request_mock, resume()).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(500, _)).Times(AtLeast(1));
  action_under_test_ptr->fetch_multipart_info_failed();
  EXPECT_STREQ("InternalError",
               action_under_test_ptr->get_s3_error_code().c_str());
}

TEST_F(S3PostCompleteActionTest, FetchPartsInfo) {
  CREATE_KVS_READER_OBJ;

  EXPECT_CALL(*(clovis_kvs_reader_factory->mock_clovis_kvs_reader),
              next_keyval(_, _, _, _, _, _))
      .Times(1);
  action_under_test_ptr->fetch_parts_info();
}

TEST_F(S3PostCompleteActionTest, GetPartsSuccessfulInvalidPart) {
  CREATE_KVS_READER_OBJ;
  CREATE_MP_METADATA_OBJ;

  result_keys_values.insert(
      std::make_pair("testkey0", std::make_pair(10, "keyval")));

  EXPECT_CALL(*(clovis_kvs_reader_factory->mock_clovis_kvs_reader),
              get_key_values())
      .WillRepeatedly(ReturnRef(result_keys_values));
  EXPECT_CALL(*request_mock, resume()).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(400, _)).Times(AtLeast(1));

  action_under_test_ptr->get_parts_successful();
  EXPECT_STREQ("InvalidPart",
               action_under_test_ptr->get_s3_error_code().c_str());
}

TEST_F(S3PostCompleteActionTest, GetPartsSuccessful) {
  CREATE_KVS_READER_OBJ;
  CREATE_MP_METADATA_OBJ;

  result_keys_values.insert(std::make_pair("0", std::make_pair(0, "keyval1")));
  result_keys_values.insert(std::make_pair("1", std::make_pair(0, "keyval2")));
  result_keys_values.insert(std::make_pair("2", std::make_pair(0, "keyval3")));

  action_under_test_ptr->parts["0"] = "keyval1";
  action_under_test_ptr->parts["1"] = "keyval2";
  action_under_test_ptr->parts["2"] = "keyval3";

  action_under_test_ptr->total_parts = action_under_test_ptr->parts.size();
  EXPECT_CALL(*(clovis_kvs_reader_factory->mock_clovis_kvs_reader),
              get_key_values())
      .WillRepeatedly(ReturnRef(result_keys_values));
  EXPECT_CALL(*(part_meta_factory->mock_part_metadata), from_json(_))
      .WillRepeatedly(Return(0));
  EXPECT_CALL(*(part_meta_factory->mock_part_metadata), get_content_length())
      .WillRepeatedly(Return(MINIMUM_ALLOWED_PART_SIZE));
  EXPECT_CALL(*(part_meta_factory->mock_part_metadata), get_md5())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(action_under_test_ptr->parts["0"]));
  action_under_test_ptr->clear_tasks();
  action_under_test_ptr->add_task(
      std::bind(&S3PostCompleteActionTest::func_callback_one, this));

  action_under_test_ptr->get_parts_successful();
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3PostCompleteActionTest, GetPartsSuccessfulEntityTooSmall) {
  CREATE_KVS_READER_OBJ;
  CREATE_MP_METADATA_OBJ;

  result_keys_values.insert(std::make_pair("0", std::make_pair(0, "keyval1")));
  result_keys_values.insert(std::make_pair("1", std::make_pair(0, "keyval2")));
  result_keys_values.insert(std::make_pair("2", std::make_pair(0, "keyval3")));

  action_under_test_ptr->parts["0"] = "keyval1";
  action_under_test_ptr->parts["1"] = "keyval2";
  action_under_test_ptr->parts["2"] = "keyval3";

  action_under_test_ptr->total_parts = action_under_test_ptr->parts.size();
  EXPECT_CALL(*(clovis_kvs_reader_factory->mock_clovis_kvs_reader),
              get_key_values())
      .WillRepeatedly(ReturnRef(result_keys_values));
  EXPECT_CALL(*(part_meta_factory->mock_part_metadata), from_json(_))
      .WillRepeatedly(Return(0));
  EXPECT_CALL(*(part_meta_factory->mock_part_metadata), get_content_length())
      .WillRepeatedly(Return(action_under_test_ptr->parts["0"].length()));
  action_under_test_ptr->clear_tasks();
  action_under_test_ptr->add_task(
      std::bind(&S3PostCompleteActionTest::func_callback_one, this));

  action_under_test_ptr->get_parts_successful();
  EXPECT_EQ(1, call_count_one);
  EXPECT_STREQ("EntityTooSmall",
               action_under_test_ptr->get_s3_error_code().c_str());
}

TEST_F(S3PostCompleteActionTest, GetPartsSuccessfulJsonError) {
  CREATE_KVS_READER_OBJ;
  CREATE_MP_METADATA_OBJ;

  result_keys_values.insert(std::make_pair("0", std::make_pair(0, "keyval1")));
  action_under_test_ptr->parts["0"] = "keyval1";

  action_under_test_ptr->total_parts = action_under_test_ptr->parts.size();
  EXPECT_CALL(*(clovis_kvs_reader_factory->mock_clovis_kvs_reader),
              get_key_values())
      .WillRepeatedly(ReturnRef(result_keys_values));
  EXPECT_CALL(*(part_meta_factory->mock_part_metadata), from_json(_))
      .WillRepeatedly(Return(-1));
  EXPECT_CALL(*request_mock, resume()).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(400, _)).Times(AtLeast(1));
  action_under_test_ptr->get_parts_successful();
  EXPECT_STREQ("InvalidPart",
               action_under_test_ptr->get_s3_error_code().c_str());
}

TEST_F(S3PostCompleteActionTest, GetPartsInfoFailed) {
  EXPECT_CALL(*request_mock, resume()).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(500, _)).Times(AtLeast(1));

  action_under_test_ptr->get_parts_failed();
  EXPECT_STREQ("InternalError",
               action_under_test_ptr->get_s3_error_code().c_str());
}

TEST_F(S3PostCompleteActionTest, SaveMetadataAbortMultipart) {
  CREATE_BUCKET_METADATA_OBJ;
  CREATE_METADATA_OBJ;
  CREATE_MP_METADATA_OBJ;

  action_under_test_ptr->set_abort_multipart(true);
  action_under_test_ptr->clear_tasks();
  action_under_test_ptr->add_task(
      std::bind(&S3PostCompleteActionTest::func_callback_one, this));
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), set_oid(_))
      .Times(AtLeast(1));

  action_under_test_ptr->save_metadata();
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3PostCompleteActionTest, SaveMetadata) {
  CREATE_BUCKET_METADATA_OBJ;
  CREATE_METADATA_OBJ;
  CREATE_MP_METADATA_OBJ;

  std::map<std::string, std::string> user_attributes;
  user_attributes["user-attr-1"] = "name";
  user_attributes["user-attr-2"] = "email";

  action_under_test_ptr->set_abort_multipart(false);
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata),
              get_user_attributes())
      .Times(AtLeast(1))
      .WillRepeatedly(ReturnRef(user_attributes));
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata),
              add_user_defined_attribute(_, _))
      .Times(AtLeast(1));
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata),
              set_content_length(_))
      .Times(AtLeast(1));
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), set_md5(_))
      .Times(AtLeast(1));
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), set_oid(_))
      .Times(AtLeast(1));
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), save(_, _))
      .Times(AtLeast(1));

  action_under_test_ptr->save_metadata();
}

TEST_F(S3PostCompleteActionTest, DeletePartIndex) {
  CREATE_PART_METADATA_OBJ;
  EXPECT_CALL(*(part_meta_factory->mock_part_metadata), remove_index(_, _))
      .Times(1);
  action_under_test_ptr->delete_part_index();
}

TEST_F(S3PostCompleteActionTest, DeleteParts) {
  CREATE_WRITER_OBJ;
  CREATE_METADATA_OBJ;

  action_under_test_ptr->set_abort_multipart(true);
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_oid())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(oid));
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_layout_id())
      .WillOnce(Return(layout_id));

  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer),
              delete_object(_, _, _))
      .Times(1);
  action_under_test_ptr->delete_parts();
}

TEST_F(S3PostCompleteActionTest, DeletePartsNext) {
  CREATE_WRITER_OBJ;
  CREATE_METADATA_OBJ;

  action_under_test_ptr->set_abort_multipart(false);
  action_under_test_ptr->clear_tasks();
  action_under_test_ptr->add_task(
      std::bind(&S3PostCompleteActionTest::func_callback_one, this));
  action_under_test_ptr->delete_parts();
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3PostCompleteActionTest, DeletePartsFailed) {
  EXPECT_CALL(*request_mock, resume()).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(500, _)).Times(AtLeast(1));
  action_under_test_ptr->delete_parts_failed();
}

TEST_F(S3PostCompleteActionTest, DeleteMultipartMetadata) {
  CREATE_MP_METADATA_OBJ;
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), remove(_, _))
      .Times(AtLeast(1));

  action_under_test_ptr->delete_multipart_metadata();
}

TEST_F(S3PostCompleteActionTest, DeleteOldObjectIfPresentNULL) {
  CREATE_MP_METADATA_OBJ;
  mp_indx_oid = {0x0, 0x0};
  action_under_test_ptr->multipart_metadata->set_old_oid(mp_indx_oid);
  action_under_test_ptr->clear_tasks();
  action_under_test_ptr->add_task(
      std::bind(&S3PostCompleteActionTest::func_callback_one, this));

  action_under_test_ptr->delete_old_object_if_present();
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3PostCompleteActionTest, DeleteOldObjectIfPresent) {
  CREATE_MP_METADATA_OBJ;
  action_under_test_ptr->multipart_metadata->set_old_oid(mp_indx_oid);
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer),
              delete_object(_, _, _))
      .Times(AtLeast(1));

  action_under_test_ptr->delete_old_object_if_present();
}

TEST_F(S3PostCompleteActionTest, DeleteOldObjectIfPresentClovisWriter) {
  CREATE_MP_METADATA_OBJ;
  CREATE_WRITER_OBJ;

  action_under_test_ptr->multipart_metadata->set_old_oid(mp_indx_oid);
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer), set_oid(_))
      .Times(AtLeast(1));
  EXPECT_CALL(*(clovis_writer_factory->mock_clovis_writer),
              delete_object(_, _, _))
      .Times(AtLeast(1));

  action_under_test_ptr->delete_old_object_if_present();
}

TEST_F(S3PostCompleteActionTest, DeleteOldObjectFailed) {
  CREATE_MP_METADATA_OBJ;
  action_under_test_ptr->multipart_metadata->set_old_oid(mp_indx_oid);
  action_under_test_ptr->clear_tasks();
  action_under_test_ptr->add_task(
      std::bind(&S3PostCompleteActionTest::func_callback_one, this));

  action_under_test_ptr->delete_old_object_failed();
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3PostCompleteActionTest, SendResponseToClientInternalError) {
  action_under_test_ptr->post_successful = false;
  EXPECT_CALL(*request_mock, resume()).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(500, _)).Times(AtLeast(1));
  action_under_test_ptr->send_response_to_s3_client();
}

TEST_F(S3PostCompleteActionTest, SendResponseToClientErrorSet) {
  action_under_test_ptr->set_s3_error("MalformedXML");
  EXPECT_CALL(*request_mock, resume()).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(400, _)).Times(AtLeast(1));
  action_under_test_ptr->send_response_to_s3_client();
  EXPECT_STREQ("MalformedXML",
               action_under_test_ptr->get_s3_error_code().c_str());
}

TEST_F(S3PostCompleteActionTest, SendResponseToClientAbortMultipart) {
  action_under_test_ptr->set_abort_multipart(true);
  EXPECT_CALL(*request_mock, resume()).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(403, _)).Times(AtLeast(1));
  action_under_test_ptr->send_response_to_s3_client();
}

TEST_F(S3PostCompleteActionTest, SendResponseToClientSuccess) {
  action_under_test_ptr->post_successful = true;
  EXPECT_CALL(*request_mock, resume()).Times(AtLeast(1));
  EXPECT_CALL(*request_mock, send_response(200, _)).Times(AtLeast(1));
  action_under_test_ptr->send_response_to_s3_client();
}
