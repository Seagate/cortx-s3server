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

#include "mock_s3_motr_wrapper.h"
#include "s3_put_multiobject_copy_action.h"
#include "mock_s3_motr_wrapper.h"
#include "mock_s3_factory.h"
#include "mock_s3_probable_delete_record.h"
#include "mock_s3_request_object.h"
#include "s3_motr_layout.h"
#include "s3_ut_common.h"
#include "s3_m0_uint128_helper.h"

using ::testing::Eq;
using ::testing::Return;
using ::testing::Invoke;
using ::testing::_;
using ::testing::ReturnRef;
using ::testing::AtLeast;
using ::testing::DefaultValue;
using ::testing::HasSubstr;


class S3PutMultipartCopyActionTest : public testing::Test {
 protected:
  S3PutMultipartCopyActionTest() {
    mp_indx_oid = {0xffff, 0xffff};
    oid = {0x1ffff, 0x1ffff};
    object_list_indx_oid = {0x11ffff, 0x1ffff};
    object_list_index_layout = {{0x11ffff, 0x1ffff}};
    objects_version_list_idx_oid = {0x1ffff, 0x11fff};
    objects_version_list_index_layout = {{0x1ffff, 0x11fff}};
    part_list_idx_oid = {0x1fff, 0x1fff};
    upload_id = "upload_id";
    call_count_one = 0;
    destination_bucket_name = "destbkt";
    destination_object_name = "destobj";
    input_headers["Authorization"] = "1";

    layout_id =
        S3MotrLayoutMap::get_instance()->get_best_layout_for_object_size();

    async_buffer_factory =
        std::make_shared<MockS3AsyncBufferOptContainerFactory>(
            S3Option::get_instance()->get_libevent_pool_buffer_size());

    evhtp_request_t *req = nullptr;
    EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();
    ptr_mock_request = std::make_shared<MockS3RequestObject>(
        req, evhtp_obj_ptr, async_buffer_factory);

    ptr_mock_s3_motr_api = std::make_shared<MockS3Motr>();

    EXPECT_CALL(*ptr_mock_request, get_bucket_name())
        .WillRepeatedly(ReturnRef(destination_bucket_name));

    EXPECT_CALL(*ptr_mock_request, get_object_name())
        .WillRepeatedly(ReturnRef(destination_object_name));

    EXPECT_CALL(*ptr_mock_s3_motr_api, m0_h_ufid_next(_))
        .WillRepeatedly(Invoke(dummy_helpers_ufid_next));

    EXPECT_CALL(*ptr_mock_request, get_query_string_value("uploadId"))
        .WillRepeatedly(Return("upload_id"));

    EXPECT_CALL(*ptr_mock_request, get_query_string_value("partNumber"))
        .WillRepeatedly(Return("1"));

    bucket_meta_factory =
        std::make_shared<MockS3BucketMetadataFactory>(ptr_mock_request);

    object_meta_factory = std::make_shared<MockS3ObjectMetadataFactory>(
      ptr_mock_request, ptr_mock_s3_motr_api);

    object_mp_meta_factory = std::make_shared<MockS3ObjectMultipartMetadataFactory>(
      ptr_mock_request, ptr_mock_s3_motr_api, upload_id);
    object_mp_meta_factory->set_object_list_index_oid(mp_indx_oid);

    part_meta_factory = std::make_shared<MockS3PartMetadataFactory>(
      ptr_mock_request, oid, upload_id, 0);

    motr_writer_factory = std::make_shared<MockS3MotrWriterFactory>(
      ptr_mock_request, oid, ptr_mock_s3_motr_api);

    motr_reader_factory = std::make_shared<MockS3MotrReaderFactory>(
      ptr_mock_request, oid, layout_id, ptr_mock_s3_motr_api);

    motr_kvs_writer_factory = std::make_shared<MockS3MotrKVSWriterFactory>(
      ptr_mock_request, ptr_mock_s3_motr_api);
  }

  std::shared_ptr<MockS3RequestObject> ptr_mock_request;
  std::shared_ptr<MockS3Motr> ptr_mock_s3_motr_api;
  std::shared_ptr<MockS3BucketMetadataFactory> bucket_meta_factory;
  std::shared_ptr<MockS3ObjectMetadataFactory> object_meta_factory;
  std::shared_ptr<MockS3PartMetadataFactory> part_meta_factory;
  std::shared_ptr<MockS3ObjectMultipartMetadataFactory> object_mp_meta_factory;
  std::shared_ptr<MockS3MotrWriterFactory> motr_writer_factory;
  std::shared_ptr<MockS3MotrReaderFactory> motr_reader_factory;
  std::shared_ptr<MockS3MotrKVSWriterFactory> motr_kvs_writer_factory;
  std::shared_ptr<MockS3AsyncBufferOptContainerFactory> async_buffer_factory;
  std::shared_ptr<S3PutMultipartCopyAction> action_under_test;
  struct m0_uint128 mp_indx_oid;
  struct m0_uint128 object_list_indx_oid;
  struct s3_motr_idx_layout object_list_index_layout;
  struct m0_uint128 objects_version_list_idx_oid;
  struct s3_motr_idx_layout objects_version_list_index_layout;
  struct m0_uint128 part_list_idx_oid;
  struct m0_uint128 oid;
  int layout_id;
  std::string upload_id;
  std::string destination_object_name;
  std::string destination_bucket_name;
  int call_count_one;
  std::map<std::string, std::string> input_headers;

  void create_src_bucket_metadata();
  void create_src_object_metadata();

  void create_dst_bucket_metadata();
  void create_dst_object_metadata();

 public:
  void func_callback_one() { call_count_one++; } 
};

void S3PutMultipartCopyActionTest::create_src_bucket_metadata() {

  EXPECT_CALL(*bucket_meta_factory->mock_bucket_metadata, load(_, _))
      .Times(AtLeast(1));
  action_under_test->fetch_additional_bucket_info();
}

void S3PutMultipartCopyActionTest::create_src_object_metadata() {

  create_src_bucket_metadata();

  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata),
              get_object_list_index_layout())
      .Times(1)
      .WillOnce(ReturnRef(object_list_index_layout));
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata),
              get_objects_version_list_index_layout())
      .Times(1)
      .WillOnce(ReturnRef(objects_version_list_index_layout));
  EXPECT_CALL(*object_meta_factory->mock_object_metadata, load(_, _))
      .Times(AtLeast(1));

  action_under_test->fetch_additional_object_info();
}

void S3PutMultipartCopyActionTest::create_dst_bucket_metadata() {

  EXPECT_CALL(*bucket_meta_factory->mock_bucket_metadata, load(_, _))
      .Times(AtLeast(1));
  action_under_test->fetch_bucket_info();
}

void S3PutMultipartCopyActionTest::create_dst_object_metadata() {
  create_dst_bucket_metadata();

  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata),
              get_object_list_index_layout())
      .Times(AtLeast(1))
      .WillRepeatedly(ReturnRef(object_list_index_layout));
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata),
              get_objects_version_list_index_layout())
      .Times(AtLeast(1))
      .WillRepeatedly(ReturnRef(objects_version_list_index_layout));
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), load(_, _))
      .Times(AtLeast(1));
  EXPECT_CALL(*(ptr_mock_request), http_verb())
      .WillOnce(Return(S3HttpVerb::PUT));

  action_under_test->fetch_object_info();
}


class S3PutMultipartCopyActionTestNoMockAuth
    : public S3PutMultipartCopyActionTest {
 protected:
  S3PutMultipartCopyActionTestNoMockAuth() : S3PutMultipartCopyActionTest() {
    S3Option::get_instance()->disable_auth();

    EXPECT_CALL(*ptr_mock_s3_motr_api, m0_h_ufid_next(_))
        .WillRepeatedly(Invoke(dummy_helpers_ufid_next));

    EXPECT_CALL(*ptr_mock_request, is_chunked())
        .WillRepeatedly(Return(false));

    EXPECT_CALL(*ptr_mock_request, get_in_headers_copy()).Times(1).WillOnce(
        ReturnRef(input_headers));

    action_under_test.reset(new S3PutMultipartCopyAction(
        ptr_mock_request, ptr_mock_s3_motr_api, object_mp_meta_factory,
        part_meta_factory, bucket_meta_factory, object_meta_factory,
        motr_writer_factory, motr_reader_factory, motr_kvs_writer_factory));
  }
};

// class S3PutMultipartCopyActionTestWithMockAuth
//     : public S3PutMultipartCopyActionTest {
//  protected:
//   S3PutMultipartCopyActionTestWithMockAuth()
//       : S3PutMultipartCopyActionTest() {
//     S3Option::get_instance()->enable_auth();
//     EXPECT_CALL(*ptr_mock_request,
// is_chunked()).WillRepeatedly(Return(true));
//     mock_auth_factory =
//         std::make_shared<MockS3AuthClientFactory>(ptr_mock_request);
//     EXPECT_CALL(*ptr_mock_request, get_in_headers_copy()).Times(1).WillOnce(
//         ReturnRef(input_headers));
//     action_under_test.reset(new S3PutMultipartCopyAction(
//         ptr_mock_request, ptr_mock_s3_motr_api, object_mp_meta_factory,
//         part_meta_factory, bucket_meta_factory, object_meta_factory,
//         motr_writer_factory, motr_reader_factory, motr_kvs_writer_factory,
//         mock_auth_factory));
//   }
//   std::shared_ptr<MockS3AuthClientFactory> mock_auth_factory;
// };

TEST_F(S3PutMultipartCopyActionTestNoMockAuth, 
       ConstructorTest) {
  EXPECT_EQ(1, action_under_test->part_number);
  EXPECT_STREQ("upload_id", action_under_test->upload_id.c_str());
  EXPECT_EQ(action_under_test->s3_copy_part_action_state,
            S3PutObjectActionState::empty);
  EXPECT_NE(0, action_under_test->number_of_tasks());
  EXPECT_EQ(0, action_under_test->total_data_to_copy);
  EXPECT_TRUE(action_under_test->auth_completed);
  EXPECT_FALSE(action_under_test->auth_failed);
  EXPECT_FALSE(action_under_test->auth_in_progress);
  EXPECT_FALSE(action_under_test->write_failed);
  EXPECT_FALSE(action_under_test->motr_write_in_progress);
  EXPECT_FALSE(action_under_test->motr_write_completed);
}

TEST_F(S3PutMultipartCopyActionTestNoMockAuth,
       ValidateSourceBucketOrObjectEmpty) {
  
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(2));
  EXPECT_CALL(*ptr_mock_request, send_response(400, _)).Times(2);

  action_under_test->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(action_under_test,
                        S3PutMultipartCopyActionTest::func_callback_one,
                        this);

  action_under_test->additional_bucket_name = "";
  action_under_test->additional_object_name = "src_obj";
  action_under_test->validate_multipart_partcopy_request();
  EXPECT_STREQ("InvalidArgument",
               action_under_test->get_s3_error_code().c_str());
  
  action_under_test->additional_bucket_name = "src_bkt";
  action_under_test->additional_object_name = "";
  action_under_test->validate_multipart_partcopy_request();
  EXPECT_STREQ("InvalidArgument",
               action_under_test->get_s3_error_code().c_str());
  EXPECT_EQ(0, call_count_one);
}

TEST_F(S3PutMultipartCopyActionTestNoMockAuth,
       ValidateSourceAndDestinationAreSame) {
  
  std::string destination_bucket = "src-bkt";
  std::string destination_object = "src-obj";
  action_under_test->additional_bucket_name = "src-bkt";
  action_under_test->additional_object_name = "src-obj";

  EXPECT_CALL(*ptr_mock_request, get_bucket_name())
      .Times(AtLeast(1))
      .WillRepeatedly(ReturnRef(destination_bucket));

  EXPECT_CALL(*ptr_mock_request, get_object_name())
      .Times(AtLeast(1))
      .WillRepeatedly(ReturnRef(destination_object));

  action_under_test->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(action_under_test,
                        S3PutMultipartCopyActionTest::func_callback_one,
                        this);

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(400, _)).Times(1);

  action_under_test->validate_multipart_partcopy_request();

  EXPECT_EQ(action_under_test->s3_copy_part_action_state,
            S3PutObjectActionState::validationFailed);
  EXPECT_STREQ("InvalidRequest",
               action_under_test->get_s3_error_code().c_str());
  EXPECT_EQ(0, call_count_one);
}

TEST_F(S3PutMultipartCopyActionTestNoMockAuth,
       SourcePartSizeGreaterThanMaxLimit) {
  
  std::string destination_bucket = "dest-bkt";
  action_under_test->additional_bucket_name = "src-bkt";
  action_under_test->additional_object_name = "src-obj";

  EXPECT_CALL(*ptr_mock_request, get_bucket_name())
      .Times(AtLeast(1))
      .WillRepeatedly(ReturnRef(destination_bucket));

  EXPECT_CALL(*ptr_mock_request, get_object_name()).Times(0);

  action_under_test->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(action_under_test,
                        S3PutMultipartCopyActionTest::func_callback_one,
                        this);

  action_under_test->additional_object_metadata =
      action_under_test->object_metadata_factory->create_object_metadata_obj(
          ptr_mock_request);

  EXPECT_CALL(*object_meta_factory->mock_object_metadata,
              get_content_length())
      .Times(2)
      .WillRepeatedly(Return(6000000000));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(400, _)).Times(1);

  action_under_test->validate_multipart_partcopy_request();

  EXPECT_EQ(action_under_test->s3_copy_part_action_state,
            S3PutObjectActionState::validationFailed);
  EXPECT_STREQ("InvalidRequest",
               action_under_test->get_s3_error_code().c_str()); 
  EXPECT_EQ(0, call_count_one);
}

TEST_F(S3PutMultipartCopyActionTestNoMockAuth,
       ValidatePartCopyRequestSuccess) {

  std::string destination_bucket = "dest-bkt";
  action_under_test->additional_bucket_name = "src-bkt";
  action_under_test->additional_object_name = "src-obj";

  action_under_test->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(action_under_test,
    S3PutMultipartCopyActionTest::func_callback_one, this);

  EXPECT_CALL(*ptr_mock_request, get_bucket_name())
      .Times(AtLeast(1))
      .WillRepeatedly(ReturnRef(destination_bucket));

  EXPECT_CALL(*ptr_mock_request, get_object_name()).Times(0);

  action_under_test->additional_object_metadata =
      action_under_test->object_metadata_factory->create_object_metadata_obj(
          ptr_mock_request);

  EXPECT_CALL(*object_meta_factory->mock_object_metadata,
              get_content_length())
      .Times(2)
      .WillRepeatedly(Return(2048));

  action_under_test->validate_multipart_partcopy_request();
  EXPECT_EQ(2048, action_under_test->total_data_to_copy);
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3PutMultipartCopyActionTestNoMockAuth,
       CheckPartNumberFailedInvalidPartTest) {

  action_under_test->part_number = MINIMUM_PART_NUMBER - 1;
  action_under_test->clear_tasks();
  EXPECT_CALL(*ptr_mock_request, get_content_length())
      .WillRepeatedly(Return(1));
  ACTION_TASK_ADD_OBJPTR(action_under_test,
                         S3PutMultipartCopyActionTest::func_callback_one, this);
  action_under_test->check_part_details();
  EXPECT_STREQ("InvalidPart", action_under_test->get_s3_error_code().c_str());
  EXPECT_EQ(0, call_count_one);

    action_under_test->part_number = MAXIMUM_PART_NUMBER + 1;
    action_under_test->clear_tasks();
    ACTION_TASK_ADD_OBJPTR(action_under_test,
                           S3PutMultipartCopyActionTest::func_callback_one,
                           this);
    action_under_test->check_part_details();
    EXPECT_STREQ("InvalidPart",
  action_under_test->get_s3_error_code().c_str());
    EXPECT_EQ(0, call_count_one);

    action_under_test->part_number = MINIMUM_PART_NUMBER;
    action_under_test->clear_tasks();
    ACTION_TASK_ADD_OBJPTR(action_under_test,
                           S3PutMultipartCopyActionTest::func_callback_one,
                           this);
    action_under_test->check_part_details();
    EXPECT_EQ(1, call_count_one);

    action_under_test->part_number = MAXIMUM_PART_NUMBER;
    action_under_test->clear_tasks();
    ACTION_TASK_ADD_OBJPTR(action_under_test,
                           S3PutMultipartCopyActionTest::func_callback_one,
                           this);
    action_under_test->check_part_details();
    EXPECT_EQ(2, call_count_one);
}

TEST_F(S3PutMultipartCopyActionTestNoMockAuth,
       ValidateObjectKeyLengthNegativeCase) {
  std::string too_long_object_name(
      "vaxsfhwmbuegarlllxjyppqbzewahzdgnykqcbmjnezicblmveddlnvuejvxtjkogpqmnexv"
      "piaqufqsxozqzsxxtmmlnukpfnpvtepdxvxqmnwnsceaujybilbqwwhhofxhlbvqeqbcbbag"
      "ijtgemhhfudggqdwpowidypjvxwwjayhghicnwupyritzpoobtwsbhihvzxnqlycpdwomlsw"
      "vsvvvvpuhlfvhckzyazsvqvrubobhlrajnytsvhnboykzzdjtvzxsacdjawgztgqgesyxgyu"
      "gmfwwoxiaksrdtbiudlppssyoylbtazbsfvaxcysnetayhkpbtegvdxyowxfofnrkigayqta"
      "teseujcngrrpfkqypqehvezuoxaqxonlxagmvbbaujjgvnhzvcgasuetslydhvxgttepjmxs"
      "zczjcvsgrgjkhedysupjtrcvtwhhgudpjgtmtrsmusucjtmzqejpfvmzsvjshkzzhtmdowgo"
      "wvzwiqdhthsdbsnxyhapevigrtvhbzpylibfxpfoxiwoyqfyzxskefjththojqgglhmhbzhl"
      "uyoycxjuwbnkdhmsstycomxqzvvpvvkzoxhwvmpbwldqcrpsbpwrozymppbnnewmmmrxdxjq"
      "thetmfvjpeldndmomdudinwjiixsidcxpbacrtlwmgaljzaglsjcbfnsfqyiawieycdvdhat"
      "wzcbypcyfsnpeuxmiugsdesnxhwywgtopqfbkxbpewohuecyneojfeksgukhsxalqxwzitsz"
      "ilqchkdokgaakogpswctdsuybydalwzznotdvmynxlkomxfeplorgzkvveuslhmmnyeufsjq"
      "kzoomzdfvaaaxzykmgcmqdqxitjtmpkriwtihthlewlebaiekhzjctlnlwqrgwwhjulqkjfd"
      "sxhkxjyrahmmnqvyslxcbcuzobmbwxopritmxzjtvnqbszdhfftmfedpxrkiktorpvibtcoa"
      "tvkvpqvevyhsscoxshpbwjhzfwmvccvbjrnjfkchbrvgctwxhfaqoqhm");

  action_under_test->part_number = MINIMUM_PART_NUMBER;
  EXPECT_CALL(*ptr_mock_request, get_object_name())
      .WillOnce(ReturnRef(too_long_object_name));

  action_under_test->check_part_details();

  EXPECT_STREQ("KeyTooLongError",
               action_under_test->get_s3_error_code().c_str());
  EXPECT_EQ(action_under_test->s3_copy_part_action_state,
            S3PutObjectActionState::validationFailed);
}

TEST_F(S3PutMultipartCopyActionTestNoMockAuth,
       ValidateMetadataLengthNegativeCase) {
  EXPECT_CALL(*ptr_mock_request, get_header_size()).WillOnce(Return(9000));
  action_under_test->check_part_details();
  EXPECT_STREQ("MetadataTooLarge",
               action_under_test->get_s3_error_code().c_str());
  EXPECT_EQ(action_under_test->s3_copy_part_action_state,
            S3PutObjectActionState::validationFailed);
}

TEST_F(S3PutMultipartCopyActionTestNoMockAuth,
       ValidateUserMetadataLengthNegativeCase) {
  EXPECT_CALL(*ptr_mock_request, get_user_metadata_size())
      .WillOnce(Return(3000));
  action_under_test->check_part_details();
  EXPECT_STREQ("MetadataTooLarge",
               action_under_test->get_s3_error_code().c_str());
  EXPECT_EQ(action_under_test->s3_copy_part_action_state,
            S3PutObjectActionState::validationFailed);
}

TEST_F(S3PutMultipartCopyActionTestNoMockAuth, FetchMultipartMetadata) {
  action_under_test->bucket_metadata =
      bucket_meta_factory->mock_bucket_metadata;
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;

  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), load(_, _))
      .Times(1);
  action_under_test->fetch_multipart_metadata();
}

TEST_F(S3PutMultipartCopyActionTestNoMockAuth,
       FetchMultiPartMetadataNoSuchUploadFailed) {
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .WillOnce(Return(S3ObjectMetadataState::missing));
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(_, _)).Times(1);

  action_under_test->fetch_multipart_failed();

  EXPECT_STREQ("NoSuchUpload", action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3PutMultipartCopyActionTestNoMockAuth,
       FetchMultiPartMetadataInternalErrorFailed) {
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;
  EXPECT_CALL(*(object_mp_meta_factory->mock_object_mp_metadata), get_state())
      .WillOnce(Return(S3ObjectMetadataState::failed));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(_, _)).Times(1);

  action_under_test->fetch_multipart_failed();

  EXPECT_STREQ("InternalError", action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3PutMultipartCopyActionTestNoMockAuth,
       CreatePart) {

  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;
  object_mp_meta_factory->mock_object_mp_metadata->set_pvid(
      (struct m0_fid *)&oid);
  EXPECT_CALL(*ptr_mock_request, get_content_length()).Times(1)
    .WillOnce(Return(1024));
  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer),
              create_object(_, _, _, _)).Times(AtLeast(1));
  action_under_test->create_part();
  EXPECT_TRUE(action_under_test->motr_writer != nullptr);
}

TEST_F(S3PutMultipartCopyActionTestNoMockAuth,
       CreatePartFailedTestWhileShutdown) {
  S3Option::get_instance()->set_is_s3_shutting_down(true);
  EXPECT_CALL(*ptr_mock_request, pause()).Times(1);
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(503, _)).Times(1);

  action_under_test->create_part_object_failed();
  EXPECT_TRUE(action_under_test->motr_writer == NULL);
  S3Option::get_instance()->set_is_s3_shutting_down(false);
}

TEST_F(S3PutMultipartCopyActionTestNoMockAuth, 
    CreatePartFailedTest) {

  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;
  object_mp_meta_factory->mock_object_mp_metadata->set_pvid(
      (struct m0_fid *)&oid);
  EXPECT_CALL(*ptr_mock_request, get_content_length()).Times(1)
    .WillOnce(Return(1024));
  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer),
              create_object(_, _, _, _)).Times(AtLeast(1));
  action_under_test->create_part();

  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), get_state())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(S3MotrWiterOpState::failed));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(_, _)).Times(1);

  action_under_test->create_part_object_failed();
  EXPECT_STREQ("InternalError", action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3PutMultipartCopyActionTestNoMockAuth, 
    CreatePartFailedToLaunchTest) {

  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;
  object_mp_meta_factory->mock_object_mp_metadata->set_pvid(
      (struct m0_fid *)&oid);
  EXPECT_CALL(*ptr_mock_request, get_content_length()).Times(1)
    .WillOnce(Return(1024));
  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer),
              create_object(_, _, _, _)).Times(AtLeast(1));
  action_under_test->create_part();

  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), get_state())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(S3MotrWiterOpState::failed_to_launch));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(_, _)).Times(1);

  action_under_test->create_part_object_failed();
  EXPECT_STREQ("ServiceUnavailable",
    action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3PutMultipartCopyActionTestNoMockAuth, SendErrorResponse) {
  action_under_test->set_s3_error("InternalError");

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(500, _)).Times(AtLeast(1));

  action_under_test->send_response_to_s3_client();
}

TEST_F(S3PutMultipartCopyActionTestNoMockAuth, SaveMetadata) {
  action_under_test->part_metadata = part_meta_factory->mock_part_metadata;
  action_under_test->object_multipart_metadata =
      object_mp_meta_factory->mock_object_mp_metadata;
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;
  action_under_test->total_data_to_copy = 1024;

  EXPECT_CALL(*(part_meta_factory->mock_part_metadata),
              reset_date_time_to_current()).Times(AtLeast(1));

  EXPECT_CALL(*(part_meta_factory->mock_part_metadata),
              set_content_length(Eq("1024"))).Times(AtLeast(1));

  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), get_content_md5())
      .Times(AtLeast(1))
      .WillOnce(Return("abcd1234abcd"));

  EXPECT_CALL(*(part_meta_factory->mock_part_metadata),
              set_md5(Eq("abcd1234abcd"))).Times(AtLeast(1));

  std::map<std::string, std::string> input_headers;
  input_headers["x-amz-meta-item-1"] = "1024";
  input_headers["x-amz-meta-item-2"] = "s3.seagate.com";

  EXPECT_CALL(*ptr_mock_request, get_in_headers_copy()).Times(1).WillOnce(
      ReturnRef(input_headers));

  EXPECT_CALL(*(part_meta_factory->mock_part_metadata),
              add_user_defined_attribute(Eq("x-amz-meta-item-1"), Eq("1024")))
      .Times(AtLeast(1));
  EXPECT_CALL(
      *(part_meta_factory->mock_part_metadata),
      add_user_defined_attribute(Eq("x-amz-meta-item-2"), Eq("s3.seagate.com")))
      .Times(AtLeast(1));

  EXPECT_CALL(*(part_meta_factory->mock_part_metadata), save(_, _))
      .Times(AtLeast(1));

  action_under_test->save_metadata();
}
