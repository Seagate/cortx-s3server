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

#include <gtest/gtest.h>

#include "mock_s3_motr_wrapper.h"
#include "mock_s3_factory.h"
#include "mock_s3_request_object.h"
#include "s3_ut_common.h"
#include "s3_m0_uint128_helper.h"

#include "s3_data_usage_action.h"

using ::testing::Eq;
using ::testing::Ne;
using ::testing::Invoke;
using ::testing::StrEq;
using ::testing::ReturnRef;
using ::testing::Return;
using ::testing::_;

class S3DataUsageActionTest : public testing::Test {
 protected:
  std::shared_ptr<MockS3RequestObject> mock_request;
  std::shared_ptr<MockS3Motr> mock_s3_motr_api;
  std::shared_ptr<MockS3MotrKVSReaderFactory> mock_kvs_reader_factory;
  std::shared_ptr<S3DataUsageAction> action_under_test;

  std::string mock_xml;

 public:
  S3DataUsageActionTest() {
    evhtp_request_t *req = NULL;
    EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();

    mock_request = std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr);
    mock_s3_motr_api = std::make_shared<MockS3Motr>();
    EXPECT_CALL(*mock_s3_motr_api, m0_h_ufid_next(_))
        .WillRepeatedly(Invoke(dummy_helpers_ufid_next));
    mock_kvs_reader_factory = std::make_shared<MockS3MotrKVSReaderFactory>(
        mock_request, mock_s3_motr_api);

    std::map<std::string, std::string> input_headers;
    EXPECT_CALL(*mock_request, get_in_headers_copy()).Times(1).WillOnce(
        ReturnRef(input_headers));

    action_under_test = std::make_shared<S3DataUsageAction>(
        mock_request, mock_kvs_reader_factory, mock_s3_motr_api);
  }
};

TEST_F(S3DataUsageActionTest, Constructor) {
  EXPECT_TRUE(action_under_test->motr_kvs_reader_factory != nullptr);
  EXPECT_TRUE(action_under_test->motr_kvs_api != nullptr);
  EXPECT_TRUE(action_under_test->motr_kvs_reader != nullptr);
  EXPECT_TRUE(action_under_test->last_key.empty());
}

TEST_F(S3DataUsageActionTest, GetDataUsageCountersMissing) {
  EXPECT_CALL(*(mock_kvs_reader_factory->mock_motr_kvs_reader), get_state())
      .Times(1)
      .WillRepeatedly(Return(S3MotrKVSReaderOpState::missing));
  action_under_test->get_data_usage_counters();
  action_under_test->get_next_keyval_failure();
  EXPECT_FALSE(action_under_test->is_error_state());
  EXPECT_STREQ(action_under_test->create_json_response().c_str(), "[]\n");
}

TEST_F(S3DataUsageActionTest, GetDataUsageCountersFailed) {
  EXPECT_CALL(*(mock_kvs_reader_factory->mock_motr_kvs_reader), get_state())
      .Times(1)
      .WillRepeatedly(Return(S3MotrKVSReaderOpState::failed_to_launch));

  EXPECT_CALL(*mock_request, c_get_full_path())
      .WillOnce(Return("/s3/data_usage"));
  action_under_test->get_data_usage_counters();
  action_under_test->get_next_keyval_failure();
  EXPECT_TRUE(action_under_test->is_error_state());
  EXPECT_STREQ("ServiceUnavailable",
               action_under_test->get_s3_error_code().c_str());
}

TEST_F(S3DataUsageActionTest, GetDataUsageCountersSuccess) {
  std::map<std::string, std::pair<int, std::string>> result_keys_values;
  result_keys_values.insert(std::make_pair(
      "acc1/1", std::make_pair(0, "{\"bytes_count\":5,\"objects_count\":2}")));
  result_keys_values.insert(std::make_pair(
      "acc1/2",
      std::make_pair(0, "{\"bytes_count\":-2,\"objects_count\":-1}")));
  result_keys_values.insert(std::make_pair(
      "acc2/1",
      std::make_pair(0, "{\"bytes_count\":13,\"objects_count\":13}")));
  std::string expected_json =
      "[{\"acc1\":{\"bytes_count\":3}},{\"acc2\":{\"bytes_count\":13}}]\n";

  EXPECT_CALL(*(mock_kvs_reader_factory->mock_motr_kvs_reader),
              get_key_values()).WillRepeatedly(ReturnRef(result_keys_values));

  action_under_test->get_data_usage_counters();
  action_under_test->get_next_keyval_success();
  EXPECT_FALSE(action_under_test->is_error_state());
  EXPECT_STREQ(action_under_test->create_json_response().c_str(),
               expected_json.c_str());
}
