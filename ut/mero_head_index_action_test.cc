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

#include "mock_s3_clovis_wrapper.h"
#include "mock_mero_request_object.h"
#include "mock_s3_factory.h"
#include "mero_head_index_action.h"
#include "s3_m0_uint128_helper.h"

using ::testing::Eq;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::_;
using ::testing::AtLeast;

class MeroHeadIndexActionTest : public testing::Test {
 protected:
  MeroHeadIndexActionTest() {
    evhtp_request_t *req = NULL;
    EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();
    index_id = {0x1ffff, 0x1ffff};
    zero_index_id = {0ULL, 0ULL};
    call_count_one = 0;

    auto index_id_str_pair = S3M0Uint128Helper::to_string_pair(index_id);
    index_id_str_hi = index_id_str_pair.first;
    index_id_str_lo = index_id_str_pair.second;

    auto zero_index_id_str_pair =
        S3M0Uint128Helper::to_string_pair(zero_index_id);
    zero_index_id_str_hi = zero_index_id_str_pair.first;
    zero_index_id_str_lo = zero_index_id_str_pair.second;

    ptr_mock_request =
        std::make_shared<MockMeroRequestObject>(req, evhtp_obj_ptr);

    std::map<std::string, std::string> input_headers;
    input_headers["Authorization"] = "1";
    EXPECT_CALL(*ptr_mock_request, get_in_headers_copy()).Times(1).WillOnce(
        ReturnRef(input_headers));
    ptr_mock_s3_clovis_api = std::make_shared<MockS3Clovis>();
    // Owned and deleted by shared_ptr in MeroHeadIndexAction
    mock_clovis_kvs_reader_factory =
        std::make_shared<MockS3ClovisKVSReaderFactory>(ptr_mock_request,
                                                       ptr_mock_s3_clovis_api);

    action_under_test.reset(new MeroHeadIndexAction(
        ptr_mock_request, mock_clovis_kvs_reader_factory));
  }

  std::shared_ptr<MockMeroRequestObject> ptr_mock_request;
  std::shared_ptr<MockS3Clovis> ptr_mock_s3_clovis_api;
  std::shared_ptr<MockS3ClovisKVSReaderFactory> mock_clovis_kvs_reader_factory;
  std::shared_ptr<MeroHeadIndexAction> action_under_test;

  struct m0_uint128 index_id;
  std::string index_id_str_lo;
  std::string index_id_str_hi;
  struct m0_uint128 zero_index_id;
  std::string zero_index_id_str_lo;
  std::string zero_index_id_str_hi;

  int call_count_one;

 public:
  void func_callback_one() { call_count_one += 1; }
};

TEST_F(MeroHeadIndexActionTest, ConstructorTest) {
  EXPECT_NE(0, action_under_test->number_of_tasks());
}

TEST_F(MeroHeadIndexActionTest, ValidIndexId) {
  EXPECT_CALL(*ptr_mock_request, get_index_id_hi()).Times(1).WillOnce(
      ReturnRef(index_id_str_hi));
  EXPECT_CALL(*ptr_mock_request, get_index_id_lo()).Times(1).WillOnce(
      ReturnRef(index_id_str_lo));

  action_under_test->clear_tasks();

  ACTION_TASK_ADD_OBJPTR(action_under_test,
                         MeroHeadIndexActionTest::func_callback_one, this);

  action_under_test->validate_request();

  EXPECT_EQ(1, call_count_one);
}

TEST_F(MeroHeadIndexActionTest, InvalidIndexId) {
  EXPECT_CALL(*ptr_mock_request, get_index_id_hi()).Times(1).WillOnce(
      ReturnRef(zero_index_id_str_hi));
  EXPECT_CALL(*ptr_mock_request, get_index_id_lo()).Times(1).WillOnce(
      ReturnRef(zero_index_id_str_lo));
  // lookup index should not be called
  EXPECT_CALL(*(mock_clovis_kvs_reader_factory->mock_clovis_kvs_reader),
              lookup_index(_, _, _)).Times(0);
  // Report error Bad request
  EXPECT_CALL(*ptr_mock_request, c_get_full_path())
      .WillOnce(Return("/indexes/123-456"));
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(400, _)).Times(AtLeast(1));

  action_under_test->validate_request();
}

TEST_F(MeroHeadIndexActionTest, EmptyIndexId) {
  std::string empty_index;
  EXPECT_CALL(*ptr_mock_request, get_index_id_hi()).Times(1).WillOnce(
      ReturnRef(empty_index));
  EXPECT_CALL(*ptr_mock_request, get_index_id_lo()).Times(1).WillOnce(
      ReturnRef(empty_index));
  // lookup_index should not be called
  EXPECT_CALL(*(mock_clovis_kvs_reader_factory->mock_clovis_kvs_reader),
              lookup_index(_, _, _)).Times(0);
  // Report error - Bad request
  EXPECT_CALL(*ptr_mock_request, c_get_full_path())
      .WillOnce(Return("/indexes/123-456"));
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(400, _)).Times(1);
  action_under_test->validate_request();
}

TEST_F(MeroHeadIndexActionTest, CheckIndexExist) {
  action_under_test->index_id = index_id;

  EXPECT_CALL(*(mock_clovis_kvs_reader_factory->mock_clovis_kvs_reader),
              lookup_index(_, _, _)).Times(1);

  action_under_test->check_index_exist();
  EXPECT_TRUE(action_under_test->clovis_kv_reader != nullptr);
}

TEST_F(MeroHeadIndexActionTest, CheckIndexExistSuccess) {
  action_under_test->clear_tasks();
  ACTION_TASK_ADD_OBJPTR(action_under_test,
                         MeroHeadIndexActionTest::func_callback_one, this);
  action_under_test->check_index_exist_success();
  EXPECT_EQ(1, call_count_one);
}

TEST_F(MeroHeadIndexActionTest, CheckIndexExistFailureMissing) {
  action_under_test->clovis_kv_reader =
      mock_clovis_kvs_reader_factory->mock_clovis_kvs_reader;

  EXPECT_CALL(*ptr_mock_request, c_get_full_path())
      .WillOnce(Return("/indexes/123-456"));
  EXPECT_CALL(*(mock_clovis_kvs_reader_factory->mock_clovis_kvs_reader),
              get_state())
      .WillRepeatedly(Return(S3ClovisKVSReaderOpState::missing));

  EXPECT_CALL(*ptr_mock_request, send_response(404, _)).Times(1);

  action_under_test->check_index_exist_failure();
}

TEST_F(MeroHeadIndexActionTest, CheckIndexExistFailureInternalError) {
  action_under_test->clovis_kv_reader =
      mock_clovis_kvs_reader_factory->mock_clovis_kvs_reader;

  EXPECT_CALL(*ptr_mock_request, c_get_full_path())
      .WillOnce(Return("/indexes/123-456"));
  EXPECT_CALL(*(mock_clovis_kvs_reader_factory->mock_clovis_kvs_reader),
              get_state())
      .WillRepeatedly(Return(S3ClovisKVSReaderOpState::failed));

  EXPECT_CALL(*ptr_mock_request, send_response(500, _)).Times(1);

  action_under_test->check_index_exist_failure();
}

TEST_F(MeroHeadIndexActionTest, CheckIndexExistFailureFailedToLaunch) {
  action_under_test->clovis_kv_reader =
      mock_clovis_kvs_reader_factory->mock_clovis_kvs_reader;
  EXPECT_CALL(*(mock_clovis_kvs_reader_factory->mock_clovis_kvs_reader),
              get_state())
      .WillRepeatedly(Return(S3ClovisKVSReaderOpState::failed_to_launch));

  EXPECT_CALL(*ptr_mock_request, c_get_full_path())
      .WillOnce(Return("/indexes/123-456"));
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request,
              set_out_header_value(Eq("Retry-After"), Eq("1"))).Times(1);
  EXPECT_CALL(*ptr_mock_request, send_response(503, _)).Times(1);

  action_under_test->check_index_exist_failure();
}

TEST_F(MeroHeadIndexActionTest, SendSuccessResponse) {
  EXPECT_CALL(*ptr_mock_request, send_response(200, _)).Times(1);

  action_under_test->send_response_to_s3_client();
}

TEST_F(MeroHeadIndexActionTest, SendBadRequestResponse) {
  action_under_test->set_s3_error("BadRequest");

  EXPECT_CALL(*ptr_mock_request, c_get_full_path())
      .WillOnce(Return("/indexes/123-456"));
  EXPECT_CALL(*ptr_mock_request, set_out_header_value(_, _)).Times(AtLeast(1));
  EXPECT_CALL(*ptr_mock_request, send_response(400, _)).Times(AtLeast(1));

  action_under_test->send_response_to_s3_client();
}
