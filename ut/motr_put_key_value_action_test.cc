/*
 * COPYRIGHT 2020 SEAGATE LLC
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
 * Original author:  Amit Kumar   <amit.kumar@seagate.com>
 * Original creation date: 24-June-2020
 */

#include "mock_s3_clovis_wrapper.h"
#include "mock_motr_request_object.h"
#include "mock_s3_factory.h"
#include "s3_m0_uint128_helper.h"

#include "motr_put_key_value_action.h"

using ::testing::ReturnRef;
using ::testing::Return;
using ::testing::AtLeast;

class MotrPutKeyValueActionTest : public testing::Test {
 protected:
  MotrPutKeyValueActionTest() {
    evhtp_request_t *req = NULL;
    EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();
    index_id = {0x1ffff, 0x1ffff};
    call_count = 0;

    auto index_id_str_pair = S3M0Uint128Helper::to_string_pair(index_id);
    index_id_str_hi = index_id_str_pair.first;
    index_id_str_lo = index_id_str_pair.second;

    ptr_mock_request =
        std::make_shared<MockMotrRequestObject>(req, evhtp_obj_ptr);

    std::map<std::string, std::string> input_headers;
    input_headers["Authorization"] = "1";
    EXPECT_CALL(*ptr_mock_request, get_in_headers_copy()).Times(1).WillOnce(
        ReturnRef(input_headers));

    ptr_mock_s3_clovis_api = std::make_shared<MockS3Clovis>();

    mock_clovis_kvs_writer_factory =
        std::make_shared<MockS3ClovisKVSWriterFactory>(ptr_mock_request,
                                                       ptr_mock_s3_clovis_api);

    action_under_test.reset(
        new MotrPutKeyValueAction(ptr_mock_request, ptr_mock_s3_clovis_api,
                                  mock_clovis_kvs_writer_factory));
  }

  int call_count;
  struct m0_uint128 index_id;
  std::string index_id_str_lo;
  std::string index_id_str_hi;
  std::shared_ptr<MockMotrRequestObject> ptr_mock_request;
  std::shared_ptr<MockS3Clovis> ptr_mock_s3_clovis_api;
  std::shared_ptr<MockS3ClovisKVSWriterFactory> mock_clovis_kvs_writer_factory;
  std::shared_ptr<MotrPutKeyValueAction> action_under_test;

 public:
  void func_callback() { call_count += 1; }
};

TEST_F(MotrPutKeyValueActionTest, ValidateKeyValueInvalidIndex) {
  struct m0_uint128 zero_index_id = {0ULL, 0ULL};

  auto zero_index_id_str_pair =
      S3M0Uint128Helper::to_string_pair(zero_index_id);
  std::string zero_index_id_str_hi = zero_index_id_str_pair.first;
  std::string zero_index_id_str_lo = zero_index_id_str_pair.second;

  EXPECT_CALL(*ptr_mock_request, get_index_id_hi()).Times(1).WillOnce(
      ReturnRef(zero_index_id_str_hi));
  EXPECT_CALL(*ptr_mock_request, get_index_id_lo()).Times(1).WillOnce(
      ReturnRef(zero_index_id_str_lo));

  EXPECT_CALL(*ptr_mock_request, c_get_full_path())
      .WillOnce(Return("/indexes/123-456"));
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(400, _)).Times(AtLeast(1));

  action_under_test->read_and_validate_key_value();
}

TEST_F(MotrPutKeyValueActionTest, ValidateKeyValueValidIndexInvalidValue) {
  EXPECT_CALL(*ptr_mock_request, get_index_id_hi()).Times(1).WillOnce(
      ReturnRef(index_id_str_hi));
  EXPECT_CALL(*ptr_mock_request, get_index_id_lo()).Times(1).WillOnce(
      ReturnRef(index_id_str_lo));

  EXPECT_CALL(*ptr_mock_request, has_all_body_content()).Times(1).WillOnce(
      Return(true));

  std::string invalid_value = "Invalid-Json-String";
  EXPECT_CALL(*ptr_mock_request, get_full_body_content_as_string())
      .Times(1)
      .WillOnce(ReturnRef(invalid_value));

  EXPECT_CALL(*ptr_mock_request, c_get_full_path())
      .WillOnce(Return("/indexes/123-456"));
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(400, _)).Times(AtLeast(1));

  action_under_test->read_and_validate_key_value();
}

TEST_F(MotrPutKeyValueActionTest, ValidateKeyValueValidIndexValidValue) {
  EXPECT_CALL(*ptr_mock_request, get_index_id_hi()).Times(1).WillOnce(
      ReturnRef(index_id_str_hi));
  EXPECT_CALL(*ptr_mock_request, get_index_id_lo()).Times(1).WillOnce(
      ReturnRef(index_id_str_lo));

  EXPECT_CALL(*ptr_mock_request, has_all_body_content()).Times(1).WillOnce(
      Return(true));

  std::string valid_value = "{\"Valid-key\":\"Valid-Value\"}";
  EXPECT_CALL(*ptr_mock_request, get_full_body_content_as_string())
      .Times(1)
      .WillOnce(ReturnRef(valid_value));

  action_under_test->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(action_under_test,
                         MotrPutKeyValueActionTest::func_callback, this);

  action_under_test->read_and_validate_key_value();

  ASSERT_EQ(1, call_count);
}

TEST_F(MotrPutKeyValueActionTest,
       ValidateKeyValueValidIndexValidValueListenIncomingData) {
  EXPECT_CALL(*ptr_mock_request, get_index_id_hi()).Times(1).WillOnce(
      ReturnRef(index_id_str_hi));
  EXPECT_CALL(*ptr_mock_request, get_index_id_lo()).Times(1).WillOnce(
      ReturnRef(index_id_str_lo));

  EXPECT_CALL(*ptr_mock_request, has_all_body_content()).Times(1).WillOnce(
      Return(false));

  EXPECT_CALL(*ptr_mock_request, listen_for_incoming_data(_, _)).Times(1);

  action_under_test->read_and_validate_key_value();
}

TEST_F(MotrPutKeyValueActionTest, PutKeyValue) {
  action_under_test->clovis_kv_writer =
      mock_clovis_kvs_writer_factory->mock_clovis_kvs_writer;

  std::string key = "my-buket";
  EXPECT_CALL(*ptr_mock_request, get_key_name()).Times(1).WillOnce(
      ReturnRef(key));

  EXPECT_CALL(*(mock_clovis_kvs_writer_factory->mock_clovis_kvs_writer),
              put_keyval(_, key, _, _, _)).Times(1);

  action_under_test->put_key_value();
}

TEST_F(MotrPutKeyValueActionTest, PutKeyValueSuccessful) {
  action_under_test->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(action_under_test,
                         MotrPutKeyValueActionTest::func_callback, this);

  action_under_test->put_key_value_successful();

  ASSERT_EQ(1, call_count);
}

TEST_F(MotrPutKeyValueActionTest, PutKeyValueFailed) {
  action_under_test->clovis_kv_writer =
      mock_clovis_kvs_writer_factory->mock_clovis_kvs_writer;

  EXPECT_CALL(*(mock_clovis_kvs_writer_factory->mock_clovis_kvs_writer),
              get_state())
      .Times(1)
      .WillOnce(Return(S3ClovisKVSWriterOpState::failed_to_launch));

  EXPECT_CALL(*ptr_mock_request, c_get_full_path())
      .WillOnce(Return("/indexes/123-456"));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(_, _)).Times(1);

  action_under_test->put_key_value_failed();
}

TEST_F(MotrPutKeyValueActionTest, ConsumeIncomingContentReadError) {
  ptr_mock_request->s3_client_read_error = "RequestTimeout";

  action_under_test->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(action_under_test,
                         MotrPutKeyValueActionTest::func_callback, this);

  action_under_test->consume_incoming_content();

  EXPECT_STREQ("RequestTimeout",
               action_under_test->get_s3_error_code().c_str());
  ASSERT_EQ(0, call_count);
}

TEST_F(MotrPutKeyValueActionTest, ConsumeIncomingContentHasAllBodyContent) {
  EXPECT_CALL(*ptr_mock_request, has_all_body_content()).Times(1).WillOnce(
      Return(true));

  std::string invalid_value = "Invalid-json-string";
  EXPECT_CALL(*ptr_mock_request, get_full_body_content_as_string())
      .Times(1)
      .WillOnce(ReturnRef(invalid_value));

  EXPECT_CALL(*ptr_mock_request, c_get_full_path())
      .WillOnce(Return("/indexes/123-456"));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(400, _)).Times(AtLeast(1));

  action_under_test->consume_incoming_content();
}

TEST_F(MotrPutKeyValueActionTest, ConsumeIncomingContentEmptyJsonValue) {
  EXPECT_CALL(*ptr_mock_request, has_all_body_content()).Times(1).WillOnce(
      Return(true));

  std::string invalid_value = "";
  EXPECT_CALL(*ptr_mock_request, get_full_body_content_as_string())
      .Times(1)
      .WillOnce(ReturnRef(invalid_value));

  EXPECT_CALL(*ptr_mock_request, c_get_full_path())
      .WillOnce(Return("/indexes/123-456"));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(400, _)).Times(AtLeast(1));

  action_under_test->consume_incoming_content();
}

TEST_F(MotrPutKeyValueActionTest, ConsumeIncomingContentResume) {
  EXPECT_CALL(*ptr_mock_request, has_all_body_content()).Times(1).WillOnce(
      Return(false));

  EXPECT_CALL(*ptr_mock_request, resume(_)).Times(1);

  action_under_test->consume_incoming_content();
}

TEST_F(MotrPutKeyValueActionTest, ValidJson) {
  std::string valid_json = "{\"Valid-key\":\"Valid-Value\"}";

  bool actual_retval = action_under_test->is_valid_json(valid_json);
  ASSERT_EQ(true, actual_retval);
}

TEST_F(MotrPutKeyValueActionTest, InvalidJson) {
  std::string invalid_json = "Invalid-json-string";
  bool actual_retval = action_under_test->is_valid_json(invalid_json);
  ASSERT_EQ(false, actual_retval);
}

TEST_F(MotrPutKeyValueActionTest, SendFailedResponse) {
  action_under_test->set_s3_error("InternalError");

  EXPECT_CALL(*ptr_mock_request, c_get_full_path())
      .WillOnce(Return("/indexes/123-456"));

  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(500, _)).Times(AtLeast(1));

  action_under_test->send_response_to_s3_client();
}

TEST_F(MotrPutKeyValueActionTest, SendSuccessResponse) {
  action_under_test->set_s3_error("");
  EXPECT_CALL(*ptr_mock_request, send_response(200, _)).Times(AtLeast(1));

  action_under_test->send_response_to_s3_client();
}
