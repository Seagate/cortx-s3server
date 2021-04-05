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

#include <memory>
#include <string>
#include "mock_s3_motr_wrapper.h"
#include "mock_s3_factory.h"
#include "mock_s3_probable_delete_record.h"
#include "s3_motr_layout.h"
#include "s3_put_object_action.h"
#include "s3_test_utils.h"
#include "s3_ut_common.h"
#include "s3_m0_uint128_helper.h"
#include "s3_md5_hash.h"

using ::testing::Eq;
using ::testing::Return;
using ::testing::StrEq;
using ::testing::Invoke;
using ::testing::_;
using ::testing::ReturnRef;
using ::testing::AtLeast;
using ::testing::DefaultValue;

#define CREATE_BUCKET_METADATA                                            \
  do {                                                                    \
    EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), load(_, _)) \
        .Times(AtLeast(1));                                               \
    action_under_test->fetch_bucket_info();                               \
  } while (0)

#define CREATE_OBJECT_METADATA                                            \
  do {                                                                    \
    CREATE_BUCKET_METADATA;                                               \
    EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata),             \
                get_object_list_index_oid())                              \
        .Times(AtLeast(1))                                                \
        .WillRepeatedly(ReturnRef(object_list_indx_oid));                 \
    EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata),             \
                get_objects_version_list_index_oid())                     \
        .Times(AtLeast(1))                                                \
        .WillRepeatedly(ReturnRef(objects_version_list_idx_oid));         \
    EXPECT_CALL(*(object_meta_factory->mock_object_metadata), load(_, _)) \
        .Times(AtLeast(1));                                               \
    EXPECT_CALL(*(ptr_mock_request), http_verb())                         \
        .WillOnce(Return(S3HttpVerb::PUT));                               \
    action_under_test->fetch_object_info();                               \
  } while (0)

class S3PutObjectActionTest : public testing::Test {
 protected:
  S3PutObjectActionTest() {
    S3Option::get_instance()->disable_auth();

    evhtp_request_t *req = NULL;
    EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();

    bucket_name = "seagatebucket";
    object_name = "objname";
    oid = {0x1ffff, 0x1ffff};
    old_object_oid = {0x1ffff, 0x1fff0};
    old_layout_id = 2;
    object_list_indx_oid = {0x11ffff, 0x1ffff};
    objects_version_list_idx_oid = {0x1ffff, 0x11fff};
    zero_oid_idx = {0ULL, 0ULL};

    layout_id =
        S3MotrLayoutMap::get_instance()->get_best_layout_for_object_size();

    call_count_one = 0;

    ptr_mock_s3_motr_api = std::make_shared<MockS3Motr>();
    EXPECT_CALL(*ptr_mock_s3_motr_api, m0_h_ufid_next(_))
        .WillRepeatedly(Invoke(dummy_helpers_ufid_next));

    async_buffer_factory =
        std::make_shared<MockS3AsyncBufferOptContainerFactory>(
            S3Option::get_instance()->get_libevent_pool_buffer_size());

    ptr_mock_request = std::make_shared<MockS3RequestObject>(
        req, evhtp_obj_ptr, async_buffer_factory);
    EXPECT_CALL(*ptr_mock_request, get_bucket_name())
        .WillRepeatedly(ReturnRef(bucket_name));
    EXPECT_CALL(*ptr_mock_request, get_object_name())
        .Times(AtLeast(1))
        .WillRepeatedly(ReturnRef(object_name));
    EXPECT_CALL(*(ptr_mock_request), get_header_value(StrEq("x-amz-tagging")))
        .WillOnce(Return(""));

    // Owned and deleted by shared_ptr in S3PutObjectAction
    bucket_meta_factory =
        std::make_shared<MockS3BucketMetadataFactory>(ptr_mock_request);

    object_meta_factory = std::make_shared<MockS3ObjectMetadataFactory>(
        ptr_mock_request, ptr_mock_s3_motr_api);
    object_meta_factory->set_object_list_index_oid(object_list_indx_oid);

    motr_writer_factory = std::make_shared<MockS3MotrWriterFactory>(
        ptr_mock_request, oid, ptr_mock_s3_motr_api);

    bucket_tag_body_factory_mock = std::make_shared<MockS3PutTagBodyFactory>(
        MockObjectTagsStr, MockRequestId);

    motr_kvs_writer_factory = std::make_shared<MockS3MotrKVSWriterFactory>(
        ptr_mock_request, ptr_mock_s3_motr_api);
    std::map<std::string, std::string> input_headers;
    input_headers["Authorization"] = "1";
    EXPECT_CALL(*ptr_mock_request, get_in_headers_copy()).Times(1).WillOnce(
        ReturnRef(input_headers));
    // EXPECT_CALL(*ptr_mock_request, get_header_value(_));
    action_under_test.reset(new S3PutObjectAction(
        ptr_mock_request, ptr_mock_s3_motr_api, bucket_meta_factory,
        object_meta_factory, motr_writer_factory, bucket_tag_body_factory_mock,
        motr_kvs_writer_factory));
  }

  std::shared_ptr<MockS3RequestObject> ptr_mock_request;
  std::shared_ptr<MockS3Motr> ptr_mock_s3_motr_api;
  std::shared_ptr<MockS3BucketMetadataFactory> bucket_meta_factory;
  std::shared_ptr<MockS3ObjectMetadataFactory> object_meta_factory;
  std::shared_ptr<MockS3MotrWriterFactory> motr_writer_factory;
  std::shared_ptr<MockS3MotrKVSWriterFactory> motr_kvs_writer_factory;
  std::shared_ptr<MockS3PutTagBodyFactory> bucket_tag_body_factory_mock;
  std::shared_ptr<MockS3AsyncBufferOptContainerFactory> async_buffer_factory;

  std::shared_ptr<S3PutObjectAction> action_under_test;

  struct m0_uint128 object_list_indx_oid;
  struct m0_uint128 objects_version_list_idx_oid;
  struct m0_uint128 oid, old_object_oid;
  struct m0_uint128 zero_oid_idx;
  int layout_id, old_layout_id;
  std::map<std::string, std::string> request_header_map;

  std::string MockObjectTagsStr;
  std::string MockRequestId;
  int call_count_one;
  std::string bucket_name, object_name;

 public:
  void func_callback_one() { call_count_one += 1; }
};

TEST_F(S3PutObjectActionTest, ConstructorTest) {
  EXPECT_EQ(0, action_under_test->tried_count);
  EXPECT_EQ("uri_salt_", action_under_test->salt);
  EXPECT_NE(0, action_under_test->number_of_tasks());
}

TEST_F(S3PutObjectActionTest, FetchBucketInfo) {
  CREATE_BUCKET_METADATA;
  EXPECT_TRUE(action_under_test->bucket_metadata != NULL);
}

TEST_F(S3PutObjectActionTest, ValidateObjectKeyLengthPositiveCase) {
  EXPECT_CALL(*ptr_mock_request, get_object_name())
      .WillOnce(ReturnRef(object_name));

  action_under_test->validate_put_request();

  EXPECT_STRNE("KeyTooLongError",
               action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3PutObjectActionTest, ValidateObjectKeyLengthNegativeCase) {
  std::string too_long_obj_name(
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

  EXPECT_CALL(*ptr_mock_request, get_object_name())
      .WillOnce(ReturnRef(too_long_obj_name));

  action_under_test->validate_put_request();

  EXPECT_STREQ("KeyTooLongError",
               action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3PutObjectActionTest, ValidateMetadataLengthNegativeCase) {
  EXPECT_CALL(*ptr_mock_request, get_object_name()).Times(AtLeast(1)).WillOnce(
      ReturnRef(object_name));
  EXPECT_CALL(*ptr_mock_request, get_header_size()).WillOnce(Return(9000));
  action_under_test->validate_put_request();
  EXPECT_STREQ("BadRequest", action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3PutObjectActionTest, ValidateUserMetadataLengthNegativeCase) {
  EXPECT_CALL(*ptr_mock_request, get_object_name()).Times(AtLeast(1)).WillOnce(
      ReturnRef(object_name));
  EXPECT_CALL(*ptr_mock_request, get_user_metadata_size())
      .WillOnce(Return(3000));
  action_under_test->validate_put_request();
  EXPECT_STREQ("BadRequest", action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3PutObjectActionTest, ValidateRequestTags) {
  call_count_one = 0;
  request_header_map.clear();
  request_header_map["x-amz-tagging"] = "key1=value1&key2=value2";
  EXPECT_CALL(*ptr_mock_request, get_header_value(_))
      .WillOnce(Return("key1=value1&key2=value2"));
  action_under_test->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(action_under_test,
                         S3PutObjectActionTest::func_callback_one, this);

  action_under_test->validate_x_amz_tagging_if_present();

  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3PutObjectActionTest, VaidateEmptyTags) {
  request_header_map.clear();
  request_header_map["x-amz-tagging"] = "";
  EXPECT_CALL(*ptr_mock_request, get_header_value(_)).WillOnce(Return(""));
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(_, _)).Times(1);
  EXPECT_CALL(*ptr_mock_request, resume(_)).Times(1);
  action_under_test->clear_tasks();

  action_under_test->validate_x_amz_tagging_if_present();
  EXPECT_STREQ("InvalidTagError",
               action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3PutObjectActionTest, VaidateInvalidTagsCase1) {
  request_header_map.clear();
  request_header_map["x-amz-tagging"] = "key1=";
  EXPECT_CALL(*ptr_mock_request, get_header_value(_)).WillOnce(Return("key1="));
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(_, _)).Times(1);
  EXPECT_CALL(*ptr_mock_request, resume(_)).Times(1);
  action_under_test->clear_tasks();

  action_under_test->validate_x_amz_tagging_if_present();
  EXPECT_STREQ("InvalidTagError",
               action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3PutObjectActionTest, VaidateInvalidTagsCase2) {
  request_header_map.clear();
  request_header_map["x-amz-tagging"] = "key1=value1&=value2";
  EXPECT_CALL(*ptr_mock_request, get_header_value(_))
      .WillOnce(Return("key1=value1&=value2"));
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(_, _)).Times(1);
  EXPECT_CALL(*ptr_mock_request, resume(_)).Times(1);
  action_under_test->clear_tasks();

  action_under_test->validate_x_amz_tagging_if_present();
  EXPECT_STREQ("InvalidTagError",
               action_under_test->get_s3_error_code().c_str());
}

// Count of tags exceding limit.

TEST_F(S3PutObjectActionTest, VaidateInvalidTagsCase3) {
  request_header_map.clear();
  request_header_map["x-amz-tagging"] =
      "key1=value1&key2=value2&key3=value3&key4=value4&key5=value5&key6=value6&"
      "key7=value7&key8=value8&key9=value9&key10=value10&key11=value11";
  EXPECT_CALL(*ptr_mock_request, get_header_value(_)).WillOnce(Return(
      "key1=value1&key2=value2&key3=value3&key4=value4&key5=value5&key6=value6&"
      "key7=value7&key8=value8&key9=value9&key10=value10&key11=value11"));
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(_, _)).Times(1);
  EXPECT_CALL(*ptr_mock_request, resume(_)).Times(1);
  action_under_test->clear_tasks();

  action_under_test->validate_x_amz_tagging_if_present();
  EXPECT_STREQ("InvalidTagError",
               action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3PutObjectActionTest, VaidateInvalidTagsCase4) {
  request_header_map.clear();
  request_header_map["x-amz-tagging"] = "Key=seag`ate&Value=marketing";
  EXPECT_CALL(*ptr_mock_request, get_header_value(_))
      .WillOnce(Return("Key=seag`ate&Value=marketing"));
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(_, _)).Times(1);
  EXPECT_CALL(*ptr_mock_request, resume(_)).Times(1);

  action_under_test->clear_tasks();
  action_under_test->validate_x_amz_tagging_if_present();

  EXPECT_STREQ("InvalidTagError",
               action_under_test->get_s3_error_code().c_str());
}

// Case 1 : URL encoded
TEST_F(S3PutObjectActionTest, VaidateSpecialCharTagsCase1) {
  call_count_one = 0;
  request_header_map.clear();
  const char *x_amz_tagging = "ke%2by=valu%2be&ke%2dy=valu%2de";  // '+' & '-'
  request_header_map["x-amz-tagging"] = x_amz_tagging;
  EXPECT_CALL(*ptr_mock_request, get_header_value(_))
      .WillOnce(Return(x_amz_tagging));
  action_under_test->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(action_under_test,
                         S3PutObjectActionTest::func_callback_one, this);

  action_under_test->validate_x_amz_tagging_if_present();

  EXPECT_EQ(1, call_count_one);
}

// Case 2 : Non URL encoded
TEST_F(S3PutObjectActionTest, VaidateSpecialCharTagsCase2) {
  call_count_one = 0;
  request_header_map.clear();
  request_header_map["x-amz-tagging"] = "ke+y=valu+e&ke-y=valu-e";
  EXPECT_CALL(*ptr_mock_request, get_header_value(_))
      .WillOnce(Return("ke+y=valu+e&ke-y=valu-e"));
  action_under_test->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(action_under_test,
                         S3PutObjectActionTest::func_callback_one, this);

  action_under_test->validate_x_amz_tagging_if_present();

  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3PutObjectActionTest, FetchObjectInfoWhenBucketNotPresent) {
  CREATE_BUCKET_METADATA;

  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillRepeatedly(Return(S3BucketMetadataState::missing));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(_, _)).Times(1);
  EXPECT_CALL(*ptr_mock_request, resume(_)).Times(1);

  action_under_test->fetch_bucket_info_failed();

  EXPECT_STREQ("NoSuchBucket", action_under_test->get_s3_error_code().c_str());
  EXPECT_TRUE(action_under_test->bucket_metadata != NULL);
  EXPECT_TRUE(action_under_test->object_metadata == NULL);
}

TEST_F(S3PutObjectActionTest, FetchObjectInfoWhenBucketFailed) {
  CREATE_BUCKET_METADATA;

  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillRepeatedly(Return(S3BucketMetadataState::failed));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(_, _)).Times(1);
  EXPECT_CALL(*ptr_mock_request, resume(_)).Times(1);

  action_under_test->fetch_bucket_info_failed();

  EXPECT_STREQ("InternalError", action_under_test->get_s3_error_code().c_str());
  EXPECT_TRUE(action_under_test->bucket_metadata != NULL);
  EXPECT_TRUE(action_under_test->object_metadata == NULL);
}

TEST_F(S3PutObjectActionTest, FetchObjectInfoWhenBucketFailedTolaunch) {
  CREATE_BUCKET_METADATA;

  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .WillRepeatedly(Return(S3BucketMetadataState::failed_to_launch));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(_, _)).Times(1);
  EXPECT_CALL(*ptr_mock_request, resume(_)).Times(1);

  action_under_test->fetch_bucket_info_failed();

  EXPECT_STREQ("ServiceUnavailable",
               action_under_test->get_s3_error_code().c_str());
  EXPECT_TRUE(action_under_test->bucket_metadata != NULL);
  EXPECT_TRUE(action_under_test->object_metadata == NULL);
}

/*   TODO metadata fetch moved to s3_object_action class,
//     so these test will be moved there
TEST_F(S3PutObjectActionTest,
       FetchObjectInfoWhenBucketPresentAndObjIndexAbsent) {
  CREATE_BUCKET_METADATA;

  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata), get_state())
      .Times(AtLeast(1))
      .WillOnce(Return(S3BucketMetadataState::present));
  bucket_meta_factory->mock_bucket_metadata->set_object_list_index_oid(
      zero_oid_idx);

  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), load(_, _))
      .Times(0);

  // Mock out the next calls on action.
  action_under_test->clear_tasks();
ACTION_TASK_ADD_OBJPTR(action_under_test,
S3PutObjectActionTest::func_callback_one, this);

  action_under_test->fetch_bucket_info_successful();

  EXPECT_EQ(1, call_count_one);
  EXPECT_TRUE(action_under_test->bucket_metadata != nullptr);
  EXPECT_TRUE(action_under_test->object_metadata == nullptr);
}
*/
TEST_F(S3PutObjectActionTest, FetchObjectInfoReturnedFoundShouldHaveNewOID) {
  CREATE_OBJECT_METADATA;

  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_state())
      .WillOnce(Return(S3ObjectMetadataState::present));

  struct m0_uint128 old_oid = {0x1ffff, 0x1ffff};
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_oid())
      .Times(1)
      .WillOnce(Return(old_oid));
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_layout_id())
      .Times(1)
      .WillOnce(Return(layout_id));

  // Mock out the next calls on action.
  action_under_test->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(action_under_test,
                         S3PutObjectActionTest::func_callback_one, this);

  // Remember default generated OID
  struct m0_uint128 oid_before_regen = action_under_test->new_object_oid;
  action_under_test->fetch_object_info_success();

  EXPECT_EQ(1, call_count_one);
  EXPECT_OID_NE(zero_oid_idx, action_under_test->old_object_oid);
  EXPECT_OID_NE(oid_before_regen, action_under_test->new_object_oid);
}

TEST_F(S3PutObjectActionTest, FetchObjectInfoReturnedNotFoundShouldUseURL2OID) {
  CREATE_OBJECT_METADATA;

  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_state())
      .WillRepeatedly(Return(S3ObjectMetadataState::missing));

  // Mock out the next calls on action.
  action_under_test->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(action_under_test,
                         S3PutObjectActionTest::func_callback_one, this);

  // Remember default generated OID
  struct m0_uint128 oid_before_regen = action_under_test->new_object_oid;
  action_under_test->fetch_object_info_success();

  EXPECT_EQ(1, call_count_one);
  EXPECT_OID_EQ(zero_oid_idx, action_under_test->old_object_oid);
  EXPECT_OID_EQ(oid_before_regen, action_under_test->new_object_oid);
}

TEST_F(S3PutObjectActionTest, FetchObjectInfoReturnedInvalidStateReportsError) {
  CREATE_OBJECT_METADATA;

  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_state())
      .WillRepeatedly(Return(S3ObjectMetadataState::failed));

  // Mock out the next calls on action.
  action_under_test->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(action_under_test,
                         S3PutObjectActionTest::func_callback_one, this);

  // Remember default generated OID
  struct m0_uint128 oid_before_regen = action_under_test->new_object_oid;

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(_, _)).Times(1);
  EXPECT_CALL(*ptr_mock_request, resume(_)).Times(1);

  action_under_test->fetch_object_info_success();

  EXPECT_STREQ("InternalError", action_under_test->get_s3_error_code().c_str());
  EXPECT_EQ(0, call_count_one);
  EXPECT_OID_EQ(zero_oid_idx, action_under_test->old_object_oid);
  EXPECT_OID_EQ(oid_before_regen, action_under_test->new_object_oid);
}

TEST_F(S3PutObjectActionTest, CreateObjectFirstAttempt) {
  EXPECT_CALL(*ptr_mock_request, get_content_length()).Times(1).WillOnce(
      Return(1024));
  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), create_object(_, _, _))
      .Times(1);
  action_under_test->create_object();
  EXPECT_TRUE(action_under_test->motr_writer != nullptr);
}

TEST_F(S3PutObjectActionTest, CreateObjectSecondAttempt) {
  EXPECT_CALL(*ptr_mock_request, get_content_length()).Times(2).WillRepeatedly(
      Return(1024));

  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), create_object(_, _, _))
      .Times(2);
  action_under_test->create_object();
  action_under_test->tried_count = 1;
  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), set_oid(_)).Times(1);
  action_under_test->create_object();
  EXPECT_TRUE(action_under_test->motr_writer != nullptr);
}

TEST_F(S3PutObjectActionTest, CreateObjectFailedTestWhileShutdown) {
  S3Option::get_instance()->set_is_s3_shutting_down(true);
  EXPECT_CALL(*ptr_mock_request, pause()).Times(1);
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(503, _)).Times(1);
  EXPECT_CALL(*ptr_mock_request, resume(_)).Times(1);

  action_under_test->create_object_failed();
  EXPECT_TRUE(action_under_test->motr_writer == NULL);
  S3Option::get_instance()->set_is_s3_shutting_down(false);
}

TEST_F(S3PutObjectActionTest, CreateObjectFailedWithCollisionExceededRetry) {
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;
  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), get_state())
      .Times(1)
      .WillOnce(Return(S3MotrWiterOpState::exists));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(500, _)).Times(1);
  EXPECT_CALL(*ptr_mock_request, resume(_)).Times(1);

  action_under_test->tried_count = MAX_COLLISION_RETRY_COUNT + 1;
  action_under_test->create_object_failed();

  EXPECT_STREQ("InternalError", action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3PutObjectActionTest, CreateObjectFailedWithCollisionRetry) {
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;
  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), get_state())
      .Times(1)
      .WillOnce(Return(S3MotrWiterOpState::exists));
  EXPECT_CALL(*ptr_mock_request, get_content_length()).Times(1).WillOnce(
      Return(1024));

  action_under_test->tried_count = MAX_COLLISION_RETRY_COUNT - 1;
  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), set_oid(_)).Times(1);
  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), create_object(_, _, _))
      .Times(1);

  action_under_test->create_object_failed();
}

TEST_F(S3PutObjectActionTest, CreateObjectFailedTest) {
  EXPECT_CALL(*ptr_mock_request, get_content_length()).Times(1).WillOnce(
      Return(1024));
  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), create_object(_, _, _))
      .Times(1);
  action_under_test->create_object();

  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), get_state())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(S3MotrWiterOpState::failed));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(_, _)).Times(1);
  EXPECT_CALL(*ptr_mock_request, resume(_)).Times(1);

  action_under_test->create_object_failed();
  EXPECT_STREQ("InternalError", action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3PutObjectActionTest, CreateObjectFailedToLaunchTest) {
  EXPECT_CALL(*ptr_mock_request, get_content_length()).Times(1).WillOnce(
      Return(1024));
  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), create_object(_, _, _))
      .Times(1);
  action_under_test->create_object();

  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), get_state())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(S3MotrWiterOpState::failed_to_launch));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(503, _)).Times(1);
  EXPECT_CALL(*ptr_mock_request, resume(_)).Times(1);

  action_under_test->create_object_failed();
  EXPECT_STREQ("ServiceUnavailable",
               action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3PutObjectActionTest, CreateNewOidTest) {
  struct m0_uint128 old_oid = action_under_test->new_object_oid;

  action_under_test->create_new_oid(old_oid);

  EXPECT_OID_NE(old_oid, action_under_test->new_object_oid);
}

TEST_F(S3PutObjectActionTest, InitiateDataStreamingForZeroSizeObject) {
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;
  EXPECT_CALL(*ptr_mock_request, get_content_length()).Times(1).WillOnce(
      Return(0));

  // Mock out the next calls on action.
  action_under_test->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(action_under_test,
                         S3PutObjectActionTest::func_callback_one, this);

  action_under_test->initiate_data_streaming();
  EXPECT_FALSE(action_under_test->write_in_progress);
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3PutObjectActionTest, InitiateDataStreamingExpectingMoreData) {
  action_under_test->_set_layout_id(layout_id);

  EXPECT_CALL(*ptr_mock_request, get_content_length()).Times(1).WillOnce(
      Return(1024));
  EXPECT_CALL(*ptr_mock_request, has_all_body_content()).Times(1).WillOnce(
      Return(false));
  EXPECT_CALL(*ptr_mock_request, listen_for_incoming_data(_, _)).Times(1);

  action_under_test->initiate_data_streaming();

  EXPECT_FALSE(action_under_test->write_in_progress);
}

TEST_F(S3PutObjectActionTest, InitiateDataStreamingWeHaveAllData) {
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;
  action_under_test->_set_layout_id(layout_id);

  EXPECT_CALL(*ptr_mock_request, get_content_length())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(1024));
  EXPECT_CALL(*ptr_mock_request, has_all_body_content()).Times(1).WillOnce(
      Return(true));
  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), get_content_length())
      .WillRepeatedly(Return(1024));

  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer),
              write_content(_, _, _, _)).Times(1);

  action_under_test->initiate_data_streaming();

  EXPECT_TRUE(action_under_test->write_in_progress);
}

// Write not in progress and we have all the data
TEST_F(S3PutObjectActionTest, ConsumeIncomingShouldWriteIfWeAllData) {
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;
  action_under_test->_set_layout_id(layout_id);

  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), is_freezed())
      .WillRepeatedly(Return(true));
  // S3Option::get_instance()->get_motr_write_payload_size() = 1048576 * 1
  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), get_content_length())
      .WillRepeatedly(Return(1024));

  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer),
              write_content(_, _, _, _)).Times(1);

  action_under_test->consume_incoming_content();

  EXPECT_TRUE(action_under_test->write_in_progress);
}

// Write not in progress, expecting more, we have exact what we can write
TEST_F(S3PutObjectActionTest, ConsumeIncomingShouldWriteIfWeExactData) {
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;
  action_under_test->_set_layout_id(layout_id);

  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), is_freezed())
      .WillRepeatedly(Return(false));
  // S3Option::get_instance()->get_motr_write_payload_size() = 1048576 * 1
  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), get_content_length())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(
           S3Option::get_instance()->get_motr_write_payload_size(layout_id)));

  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer),
              write_content(_, _, _, _)).Times(1);

  EXPECT_CALL(*ptr_mock_request, pause()).Times(1);

  action_under_test->consume_incoming_content();

  EXPECT_TRUE(action_under_test->write_in_progress);
}

// Write not in progress, expecting more, we have more than we can write
TEST_F(S3PutObjectActionTest, ConsumeIncomingShouldWriteIfWeHaveMoreData) {
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;
  action_under_test->_set_layout_id(layout_id);

  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), is_freezed())
      .WillRepeatedly(Return(false));
  // S3Option::get_instance()->get_motr_write_payload_size() = 1048576 * 1
  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), get_content_length())
      .WillRepeatedly(Return(
           S3Option::get_instance()->get_motr_write_payload_size(layout_id) +
           1024));

  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer),
              write_content(_, _, _, _)).Times(1);

  EXPECT_CALL(*ptr_mock_request, pause()).Times(1);

  action_under_test->consume_incoming_content();

  EXPECT_TRUE(action_under_test->write_in_progress);
}

// we are expecting more data
TEST_F(S3PutObjectActionTest, ConsumeIncomingShouldPauseWhenWeHaveTooMuch) {
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;
  action_under_test->_set_layout_id(layout_id);

  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), is_freezed())
      .WillRepeatedly(Return(false));
  // S3Option::get_instance()->get_motr_write_payload_size() = 1048576 * 1
  // S3_READ_AHEAD_MULTIPLE: 1
  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), get_content_length())
      .WillRepeatedly(Return(
           S3Option::get_instance()->get_motr_write_payload_size(layout_id) *
           S3Option::get_instance()->get_read_ahead_multiple() * 2));

  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer),
              write_content(_, _, _, _)).Times(1);

  EXPECT_CALL(*ptr_mock_request, pause()).Times(1);
  action_under_test->consume_incoming_content();

  EXPECT_TRUE(action_under_test->write_in_progress);
}

TEST_F(S3PutObjectActionTest,
       ConsumeIncomingShouldNotWriteWhenWriteInprogress) {
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;
  action_under_test->write_in_progress = true;

  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), is_freezed())
      .WillRepeatedly(Return(true));
  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), get_content_length())
      .Times(1)
      .WillOnce(Return(
           S3Option::get_instance()->get_motr_write_payload_size(layout_id) *
           2));

  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer),
              write_content(_, _, _, _)).Times(0);

  action_under_test->consume_incoming_content();
}

TEST_F(S3PutObjectActionTest, DelayedDeleteOldObject) {
  CREATE_OBJECT_METADATA;

  S3Option::get_instance()->set_s3server_obj_delayed_del_enabled(true);

  m0_uint128 old_object_oid = {0x1ffff, 0x1ffff};
  int old_layout_id = 2;
  action_under_test->old_object_oid = old_object_oid;
  action_under_test->old_layout_id = old_layout_id;

  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), set_oid(_)).Times(0);
  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer),
              delete_object(_, _, old_layout_id)).Times(0);

  action_under_test->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(action_under_test,
                         S3PutObjectActionTest::func_callback_one, this);

  action_under_test->delete_old_object();
  EXPECT_EQ(1, call_count_one);
}

TEST_F(S3PutObjectActionTest, ConsumeIncomingContentRequestTimeout) {
  ptr_mock_request->s3_client_read_error = "RequestTimeout";
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(_, _)).Times(AtLeast(1));

  action_under_test->consume_incoming_content();
  EXPECT_STREQ("RequestTimeout",
               action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3PutObjectActionTest, WriteObjectShouldWriteContentAndMarkProgress) {
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;
  action_under_test->_set_layout_id(layout_id);

  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer),
              write_content(_, _, _, _)).Times(1);
  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), get_content_length())
      .WillRepeatedly(Return(1024));

  action_under_test->write_object(async_buffer_factory->get_mock_buffer());

  EXPECT_TRUE(action_under_test->write_in_progress);
}

TEST_F(S3PutObjectActionTest, WriteObjectFailedShouldUndoMarkProgress) {
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;
  action_under_test->_set_layout_id(layout_id);

  // mock mark progress
  action_under_test->write_in_progress = true;
  action_under_test->new_oid_str = S3M0Uint128Helper::to_string(oid);
  MockS3ProbableDeleteRecord *prob_rec = new MockS3ProbableDeleteRecord(
      action_under_test->new_oid_str, {0ULL, 0ULL}, "abc_obj", oid, layout_id,
      object_list_indx_oid, objects_version_list_idx_oid,
      "" /* Version does not exists yet */, false /* force_delete */,
      false /* is_multipart */, {0ULL, 0ULL});
  action_under_test->new_probable_del_rec.reset(prob_rec);
  // expectations for mark_new_oid_for_deletion()
  // EXPECT_CALL(*prob_rec, set_force_delete(true)).Times(1);
  // EXPECT_CALL(*prob_rec, to_json()).Times(1);
  // EXPECT_CALL(*(motr_kvs_writer_factory->mock_motr_kvs_writer),
  //            put_keyval(_, _, _, _, _)).Times(1);

  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), get_state())
      .Times(1)
      .WillOnce(Return(S3MotrWiterOpState::failed));
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(500, _)).Times(1);
  EXPECT_CALL(*ptr_mock_request, resume(_)).Times(1);

  action_under_test->write_object_failed();

  EXPECT_STREQ("InternalError", action_under_test->get_s3_error_code().c_str());
  EXPECT_FALSE(action_under_test->write_in_progress);
}

TEST_F(S3PutObjectActionTest, WriteObjectFailedDuetoEntityOpenFailure) {
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;
  action_under_test->_set_layout_id(layout_id);

  // mock mark progress
  action_under_test->write_in_progress = true;
  action_under_test->new_oid_str = S3M0Uint128Helper::to_string(oid);
  MockS3ProbableDeleteRecord *prob_rec = new MockS3ProbableDeleteRecord(
      action_under_test->new_oid_str, {0ULL, 0ULL}, "abc_obj", oid, layout_id,
      object_list_indx_oid, objects_version_list_idx_oid,
      "" /* Version does not exists yet */, false /* force_delete */,
      false /* is_multipart */, {0ULL, 0ULL});
  action_under_test->new_probable_del_rec.reset(prob_rec);
  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), get_state())
      .Times(1)
      .WillOnce(Return(S3MotrWiterOpState::failed_to_launch));
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(503, _)).Times(1);
  EXPECT_CALL(*ptr_mock_request, resume(_)).Times(1);

  action_under_test->write_object_failed();

  EXPECT_FALSE(action_under_test->write_in_progress);
  EXPECT_STREQ("ServiceUnavailable",
               action_under_test->get_s3_error_code().c_str());
}

// Test S3PutObjectAction::write_object_failed() S3 fault mode code
TEST_F(S3PutObjectActionTest, WriteObjectFailedVerifyS3FaultMode) {
  MD5hash in_md5crypt, out_md5;
  std::string data("S3 fault mode");
  std::string bucket("seagate_bucket"), object("object1");
  in_md5crypt.Update((const char *)data.c_str(), data.size());
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;
  action_under_test->motr_writer->set_MD5Hash_instance(in_md5crypt);

  action_under_test->_set_layout_id(9);
  size_t object_size = 1048 * 10;
  // Enable S3 fault mode
  action_under_test->max_objects_in_s3_fault_mode = 1;
  action_under_test->tried_count = 1;
  action_under_test->total_object_size_consumed = object_size / 2;
  CREATE_BUCKET_METADATA;
  action_under_test->new_object_metadata =
      object_meta_factory->mock_object_metadata;
  action_under_test->new_object_metadata->set_object_list_index_oid(
      object_list_indx_oid);
  action_under_test->new_object_metadata->set_objects_version_list_index_oid(
      objects_version_list_idx_oid);

  action_under_test->new_oid_str = S3M0Uint128Helper::to_string(oid);
  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), get_state())
      .Times(AtLeast(1))
      .WillOnce(Return(S3MotrWiterOpState::failed));
  ptr_mock_request->set_bucket_name(bucket);
  ptr_mock_request->set_object_name(object);
  EXPECT_CALL(*ptr_mock_request, get_content_length())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(object_size));
  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), create_object(_, _, _))
      .Times(1);

  action_under_test->last_object_size = object_size / 2;

  action_under_test->write_object_failed();

  in_md5crypt.Finalize();

  // Verify MD5 hash instance is saved correctly
  out_md5 = action_under_test->last_MD5Hash_state;
  out_md5.Finalize();
  EXPECT_STREQ(in_md5crypt.get_md5_string().c_str(),
               out_md5.get_md5_string().c_str());

  EXPECT_TRUE(action_under_test->fault_mode_active);
  EXPECT_EQ((object_size / 2), action_under_test->primary_object_size);
  EXPECT_EQ(action_under_test->new_object_metadata->get_object_type(),
            S3ObjectMetadataType::only_frgments);
  EXPECT_TRUE(action_under_test->new_object_metadata->is_object_extended());
}

TEST_F(S3PutObjectActionTest, WriteObjectSuccessfulWhileShuttingDown) {
  S3Option::get_instance()->set_is_s3_shutting_down(true);
  EXPECT_CALL(*ptr_mock_request, pause()).Times(1);
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(503, _)).Times(1);
  EXPECT_CALL(*ptr_mock_request, resume(_)).Times(1);

  action_under_test->_set_layout_id(layout_id);

  // mock mark progress
  action_under_test->write_in_progress = true;

  action_under_test->write_object_successful();

  S3Option::get_instance()->set_is_s3_shutting_down(false);

  EXPECT_FALSE(action_under_test->write_in_progress);
}

// We have all the data: Freezed
TEST_F(S3PutObjectActionTest, WriteObjectSuccessfulShouldWriteStateAllData) {
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;
  action_under_test->_set_layout_id(layout_id);

  // mock mark progress
  action_under_test->write_in_progress = true;

  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), is_freezed())
      .WillRepeatedly(Return(true));
  // S3Option::get_instance()->get_motr_write_payload_size() = 1048576 * 1
  // S3_READ_AHEAD_MULTIPLE: 1
  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), get_content_length())
      .WillRepeatedly(Return(1024));
  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer),
              write_content(_, _, _, _)).Times(1);

  action_under_test->write_object_successful();

  EXPECT_TRUE(action_under_test->write_in_progress);
}

// We have some data but not all and exact to write
TEST_F(S3PutObjectActionTest,
       WriteObjectSuccessfulShouldWriteWhenExactWritableSize) {
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;
  action_under_test->_set_layout_id(layout_id);

  // mock mark progress
  action_under_test->write_in_progress = true;

  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), is_freezed())
      .WillRepeatedly(Return(false));
  // S3Option::get_instance()->get_motr_write_payload_size() = 1048576 * 1
  // S3_READ_AHEAD_MULTIPLE: 1
  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), get_content_length())
      .WillRepeatedly(Return(
           S3Option::get_instance()->get_motr_write_payload_size(layout_id)));

  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer),
              write_content(_, _, _, _)).Times(1);
  EXPECT_CALL(*ptr_mock_request, resume(_)).Times(1);

  action_under_test->write_object_successful();

  EXPECT_TRUE(action_under_test->write_in_progress);
}

// We have some data but not all and but have more to write
TEST_F(S3PutObjectActionTest,
       WriteObjectSuccessfulShouldWriteSomeDataWhenMoreData) {
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;
  action_under_test->_set_layout_id(layout_id);

  // mock mark progress
  action_under_test->write_in_progress = true;

  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), is_freezed())
      .WillRepeatedly(Return(false));
  // S3Option::get_instance()->get_motr_write_payload_size() = 1048576 * 1
  // S3_READ_AHEAD_MULTIPLE: 1
  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), get_content_length())
      .WillRepeatedly(Return(
           S3Option::get_instance()->get_motr_write_payload_size(layout_id) +
           1024));

  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer),
              write_content(_, _, _, _)).Times(1);
  EXPECT_CALL(*ptr_mock_request, resume(_)).Times(1);

  action_under_test->write_object_successful();

  EXPECT_TRUE(action_under_test->write_in_progress);
}

// We have some data but not all and but have more to write
TEST_F(S3PutObjectActionTest, WriteObjectSuccessfulDoNextStepWhenAllIsWritten) {
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;
  action_under_test->_set_layout_id(layout_id);

  // mock mark progress
  action_under_test->write_in_progress = true;

  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), is_freezed())
      .WillRepeatedly(Return(true));
  // S3Option::get_instance()->get_motr_write_payload_size() = 1048576 * 1
  // S3_READ_AHEAD_MULTIPLE: 1
  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), get_content_length())
      .WillRepeatedly(Return(0));

  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer),
              write_content(_, _, _, _)).Times(0);

  // Mock out the next calls on action.
  action_under_test->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(action_under_test,
                         S3PutObjectActionTest::func_callback_one, this);

  action_under_test->write_object_successful();

  EXPECT_EQ(1, call_count_one);
  EXPECT_FALSE(action_under_test->write_in_progress);
}

// We expecting more and not enough to write
TEST_F(S3PutObjectActionTest, WriteObjectSuccessfulShouldRestartReadingData) {
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;
  action_under_test->_set_layout_id(layout_id);

  // mock mark progress
  action_under_test->write_in_progress = true;

  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), is_freezed())
      .WillRepeatedly(Return(false));
  // S3Option::get_instance()->get_motr_write_payload_size() = 1048576 * 1
  // S3_READ_AHEAD_MULTIPLE: 1
  EXPECT_CALL(*async_buffer_factory->get_mock_buffer(), get_content_length())
      .WillRepeatedly(Return(
           S3Option::get_instance()->get_motr_write_payload_size(layout_id) -
           1024));

  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer),
              write_content(_, _, _, _)).Times(0);

  action_under_test->write_object_successful();

  EXPECT_FALSE(action_under_test->write_in_progress);
}

// Test S3PutObjectAction::write_object_successful() in s3 fault mode
TEST_F(S3PutObjectActionTest, WriteObjectSuccessfulVerifyS3FaultMode) {
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;
  action_under_test->_set_layout_id(layout_id);
  std::string bucket("seagate_bucket"), object("object1");

  CREATE_BUCKET_METADATA;
  action_under_test->new_object_metadata =
      object_meta_factory->mock_object_metadata;
  action_under_test->new_object_metadata->set_object_list_index_oid(
      object_list_indx_oid);
  action_under_test->new_object_metadata->set_objects_version_list_index_oid(
      objects_version_list_idx_oid);
  action_under_test->new_oid_str = S3M0Uint128Helper::to_string(oid);
  action_under_test->current_fault_iteration = 1;
  action_under_test->new_object_metadata->regenerate_version_id();

  ptr_mock_request->set_bucket_name(bucket);
  ptr_mock_request->set_object_name(object);
  ptr_mock_request->get_buffered_input()->freeze();

  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), get_oid())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(oid));

  // mock mark progress
  action_under_test->write_in_progress = true;
  action_under_test->fault_mode_active = true;
  action_under_test->create_fragment_when_write_success = true;
  std::shared_ptr<S3ObjectExtendedMetadata> new_ext_object_metadata =
      object_meta_factory->create_object_ext_metadata_obj(
          ptr_mock_request, ptr_mock_request->get_bucket_name(),
          ptr_mock_request->get_object_name(),
          action_under_test->new_object_metadata->get_obj_version_key(),
          object_list_indx_oid);
  action_under_test->new_object_metadata->set_extended_object_metadata(
      new_ext_object_metadata);
  action_under_test->write_object_successful();

  EXPECT_FALSE(action_under_test->motr_writer->get_buffer_rewrite_flag());
}

TEST_F(S3PutObjectActionTest, SaveMetadata) {
  CREATE_BUCKET_METADATA;
  bucket_meta_factory->mock_bucket_metadata->set_object_list_index_oid(
      object_list_indx_oid);

  action_under_test->new_object_metadata =
      object_meta_factory->mock_object_metadata;
  action_under_test->new_oid_str = S3M0Uint128Helper::to_string(oid);

  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;

  EXPECT_CALL(*ptr_mock_request, get_data_length_str()).Times(1).WillOnce(
      Return("1024"));
  EXPECT_CALL(*ptr_mock_request, get_header_value("content-md5"))
      .Times(1)
      .WillOnce(Return(""));
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata),
              reset_date_time_to_current()).Times(AtLeast(1));
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata),
              set_content_length(Eq("1024"))).Times(AtLeast(1));
  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), get_content_md5())
      .Times(AtLeast(1))
      .WillOnce(Return("abcd1234abcd"));
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata),
              set_md5(Eq("abcd1234abcd"))).Times(AtLeast(1));
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), set_tags(_))
      .Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, get_header_value("Content-Type"))
      .Times(1)
      .WillOnce(Return(""));
  std::map<std::string, std::string> input_headers;
  input_headers["x-amz-meta-item-1"] = "1024";
  input_headers["x-amz-meta-item-2"] = "s3.seagate.com";

  EXPECT_CALL(*ptr_mock_request, get_in_headers_copy()).Times(1).WillOnce(
      ReturnRef(input_headers));

  EXPECT_CALL(*(object_meta_factory->mock_object_metadata),
              add_user_defined_attribute(Eq("x-amz-meta-item-1"), Eq("1024")))
      .Times(AtLeast(1));
  EXPECT_CALL(
      *(object_meta_factory->mock_object_metadata),
      add_user_defined_attribute(Eq("x-amz-meta-item-2"), Eq("s3.seagate.com")))
      .Times(AtLeast(1));

  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), save(_, _))
      .Times(AtLeast(1));

  action_under_test->save_metadata();
}

TEST_F(S3PutObjectActionTest, SaveObjectMetadataFailed) {
  CREATE_OBJECT_METADATA;
  bucket_meta_factory->mock_bucket_metadata->set_object_list_index_oid(
      object_list_indx_oid);
  action_under_test->new_object_metadata =
      object_meta_factory->mock_object_metadata;

  action_under_test->new_oid_str = S3M0Uint128Helper::to_string(oid);

  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;

  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_state())
      .WillRepeatedly(Return(S3ObjectMetadataState::failed));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(500, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, resume(_)).Times(1);

  MockS3ProbableDeleteRecord *prob_rec = new MockS3ProbableDeleteRecord(
      action_under_test->new_oid_str, {0ULL, 0ULL}, "abc_obj", oid, layout_id,
      object_list_indx_oid, objects_version_list_idx_oid,
      "" /* Version does not exists yet */, false /* force_delete */,
      false /* is_multipart */, {0ULL, 0ULL});
  action_under_test->new_probable_del_rec.reset(prob_rec);
  action_under_test->new_ext_probable_del_rec_list.push_back(
      std::move(action_under_test->new_probable_del_rec));
  // expectations for mark_new_oid_for_deletion()
  EXPECT_CALL(*prob_rec, set_force_delete(true)).Times(1);
  EXPECT_CALL(*prob_rec, to_json()).Times(1);
  EXPECT_CALL(*(motr_kvs_writer_factory->mock_motr_kvs_writer),
              put_keyval(_, _, _, _)).Times(1);

  action_under_test->clear_tasks();
  action_under_test->save_object_metadata_failed();

  EXPECT_STREQ("InternalError", action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3PutObjectActionTest, SendResponseWhenShuttingDown) {
  S3Option::get_instance()->set_is_s3_shutting_down(true);

  EXPECT_CALL(*ptr_mock_request, pause()).Times(1);
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request,
              set_out_header_value(Eq("Retry-After"), Eq("1"))).Times(1);
  EXPECT_CALL(*ptr_mock_request, send_response(503, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, resume(_)).Times(1);

  // send_response_to_s3_client is called in check_shutdown_and_rollback
  action_under_test->check_shutdown_and_rollback();

  S3Option::get_instance()->set_is_s3_shutting_down(false);
}

TEST_F(S3PutObjectActionTest, SendErrorResponse) {
  action_under_test->set_s3_error("InternalError");

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(500, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, resume(false)).Times(1);

  action_under_test->send_response_to_s3_client();
}

TEST_F(S3PutObjectActionTest, SendSuccessResponse) {
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;

  // Simulate success
  action_under_test->s3_put_action_state =
      S3PutObjectActionState::metadataSaved;
  std::unique_ptr<S3ProbableDeleteRecord> new_probable_del_rec;
  new_probable_del_rec.reset(new S3ProbableDeleteRecord(
      "new_object", old_object_oid, "test", oid, 9, oid, oid, "test/2021",
      false /* force_delete */));

  action_under_test->new_ext_probable_del_rec_list.push_back(
      std::move(new_probable_del_rec));

  // expectations for remove_new_oid_probable_record()
  action_under_test->new_oid_str = S3M0Uint128Helper::to_string(oid);
  EXPECT_CALL(*(motr_kvs_writer_factory->mock_motr_kvs_writer),
              delete_keyval(_, _, _, _)).Times(1);

  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer), get_content_md5())
      .Times(AtLeast(1))
      .WillOnce(Return("abcd1234abcd"));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(200, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, resume(_)).Times(1);

  action_under_test->send_response_to_s3_client();
}

TEST_F(S3PutObjectActionTest, SendFailedResponse) {
  action_under_test->set_s3_error("InternalError");
  action_under_test->s3_put_action_state =
      S3PutObjectActionState::validationFailed;

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(500, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, resume(_)).Times(1);

  action_under_test->send_response_to_s3_client();
}

TEST_F(S3PutObjectActionTest, ValidateMissingContentLength) {
  EXPECT_CALL(*ptr_mock_request, get_object_name())
      .WillOnce(ReturnRef(object_name));
  EXPECT_CALL(*ptr_mock_request, is_header_present("Content-Length"))
      .WillOnce(Return(false));

  action_under_test->validate_put_request();

  EXPECT_STREQ("MissingContentLength",
               action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3PutObjectActionTest, AddExtendedObjectToProbableList) {
  struct m0_uint128 extended_oid = oid;
  unsigned int layout_id = 9;
  size_t object_size = 1024 * 1024;
  std::vector<std::unique_ptr<S3ProbableDeleteRecord>>
      new_ext_probable_del_rec_list;
  CREATE_BUCKET_METADATA;

  action_under_test->new_object_metadata =
      object_meta_factory->mock_object_metadata;
  action_under_test->new_object_metadata->set_object_list_index_oid(
      object_list_indx_oid);
  action_under_test->new_object_metadata->set_objects_version_list_index_oid(
      objects_version_list_idx_oid);

  action_under_test->new_oid_str = S3M0Uint128Helper::to_string(oid);

  EXPECT_CALL(*ptr_mock_request, get_bucket_name())
      .WillRepeatedly(ReturnRef(bucket_name));

  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata),
              get_object_list_index_oid())
      .WillRepeatedly(Return(object_list_indx_oid));
  EXPECT_CALL(*(bucket_meta_factory->mock_bucket_metadata),
              get_objects_version_list_index_oid())
      .WillRepeatedly(Return(objects_version_list_idx_oid));

  EXPECT_CALL(*(object_meta_factory->mock_object_metadata), get_object_name())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(object_name));
  EXPECT_CALL(*(object_meta_factory->mock_object_metadata),
              get_version_key_in_index())
      .Times(AtLeast(1))
      .WillRepeatedly(Return(object_name));
  EXPECT_CALL(*(motr_kvs_writer_factory->mock_motr_kvs_writer),
              put_keyval(_, _, _, _, _)).Times(1);

  action_under_test->add_extended_object_oid_to_probable_dead_oid_list(
      extended_oid, layout_id, object_size,
      action_under_test->new_object_metadata, new_ext_probable_del_rec_list);
}

TEST_F(S3PutObjectActionTest, AddExtendedObjectToProbableListFailed) {
  action_under_test->motr_kv_writer =
      motr_kvs_writer_factory->mock_motr_kvs_writer;
  EXPECT_CALL(*(motr_kvs_writer_factory->mock_motr_kvs_writer), get_state())
      .Times(1)
      .WillOnce(Return(S3MotrKVSWriterOpState::failed));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(500, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, resume(_)).Times(1);

  action_under_test->add_extended_object_oid_to_probable_dead_oid_list_failed();
}

TEST_F(S3PutObjectActionTest, ContinueObjectWrite) {
  MD5hash in_md5crypt, out_md5;
  std::string data("Sample object data");
  in_md5crypt.Update((const char *)data.c_str(), data.size());
  action_under_test->last_MD5Hash_state = in_md5crypt;
  action_under_test->motr_writer = motr_writer_factory->mock_motr_writer;

  EXPECT_CALL(*(motr_writer_factory->mock_motr_writer),
              write_content(_, _, _, _)).Times(1);

  action_under_test->continue_object_write();

  in_md5crypt.Finalize();

  // Verify MD5 hash instance is saved correctly
  out_md5 = motr_writer_factory->mock_motr_writer->get_MD5Hash_instance();
  out_md5.Finalize();
  EXPECT_STREQ(in_md5crypt.get_md5_string().c_str(),
               out_md5.get_md5_string().c_str());

  // Verify 'create_fragment_when_write_success' is set true
  EXPECT_TRUE(action_under_test->create_fragment_when_write_success);
}