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

#include "s3_object_list_v2_response.h"
#include "gtest/gtest.h"
#include "mock_s3_async_buffer_opt_container.h"
#include "mock_s3_factory.h"
#include "mock_s3_object_metadata.h"
#include "mock_s3_part_metadata.h"
#include "mock_s3_request_object.h"

using ::testing::Eq;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::HasSubstr;

#define CHECK_XML_RESPONSE                        \
  do {                                            \
    EXPECT_THAT(response, HasSubstr("obj1"));     \
    EXPECT_THAT(response, HasSubstr("abcd"));     \
    EXPECT_THAT(response, HasSubstr("1024"));     \
    EXPECT_THAT(response, HasSubstr("STANDARD")); \
  } while (0)

class S3ObjectListResponseV2Test : public testing::Test {
 protected:
  S3ObjectListResponseV2Test() {
    evhtp_request_t *req = NULL;
    EvhtpInterface *evhtp_obj_ptr = new EvhtpWrapper();
    bucket_name = "seagatebucket";
    object_name = "objname";

    object_list_indx_oid = {0x11ffff, 0x1ffff};

    async_buffer_factory =
        std::make_shared<MockS3AsyncBufferOptContainerFactory>(
            S3Option::get_instance()->get_libevent_pool_buffer_size());

    mock_request = std::make_shared<MockS3RequestObject>(req, evhtp_obj_ptr,
                                                         async_buffer_factory);
    EXPECT_CALL(*mock_request, get_bucket_name())
        .WillRepeatedly(ReturnRef(bucket_name));
    EXPECT_CALL(*mock_request, get_object_name())
        .WillRepeatedly(ReturnRef(object_name));
    mock_request->set_user_id("1");
    mock_request->set_user_name("s3user");
    mock_request->set_account_id("1");
    mock_request->set_account_name("s3user");
    mock_request->set_canonical_id("qWwZGnGYTga8gbpcuY79SA");

    response_under_test = std::make_shared<S3ObjectListResponseV2>("url");
  }

  virtual void SetUp() {
    response_under_test->set_bucket_name("test.bucket.name");
    response_under_test->set_request_marker_key("test_request_marker_key");
    response_under_test->set_next_marker_key("test_next_marker_key");
  }

  std::shared_ptr<S3ObjectListResponseV2> response_under_test;
  std::shared_ptr<MockS3RequestObject> mock_request;
  std::shared_ptr<MockS3AsyncBufferOptContainerFactory> async_buffer_factory;
  struct m0_uint128 object_list_indx_oid;
  std::string bucket_name, object_name;
};

// Test fields are initialized to their default values.
TEST_F(S3ObjectListResponseV2Test, ConstructorTest) {
  EXPECT_EQ("", response_under_test->continuation_token);
  EXPECT_FALSE(response_under_test->fetch_owner);
  EXPECT_EQ("", response_under_test->start_after);
  EXPECT_EQ("", response_under_test->response_v2_xml);
}

TEST_F(S3ObjectListResponseV2Test, VerifyObjectListV2Setters) {
  response_under_test->set_continuation_token("efabdcrgh");
  bool fetch_owner = true;
  response_under_test->set_fetch_owner(fetch_owner);
  response_under_test->set_start_after("abcd");

  EXPECT_EQ("abcd", response_under_test->start_after);
  EXPECT_TRUE(response_under_test->fetch_owner);
  EXPECT_EQ("efabdcrgh", response_under_test->continuation_token);
}

TEST_F(S3ObjectListResponseV2Test, getXML_V2) {
  std::shared_ptr<MockS3ObjectMetadata> mock_obj =
      std::make_shared<MockS3ObjectMetadata>(mock_request);
  mock_obj->set_object_list_index_oid(object_list_indx_oid);

  response_under_test->add_object(mock_obj);

  response_under_test->set_continuation_token("efabdcrghxyeefgh89jghr1");
  bool fetch_owner = true;
  response_under_test->set_fetch_owner(fetch_owner);
  response_under_test->set_start_after("start-after-obj");
  response_under_test->set_response_is_truncated(false);
  response_under_test->set_next_marker_key("Next_efabdcrghxyeefgh89jghr1");

  EXPECT_CALL(*mock_obj, get_object_name()).WillOnce(Return("obj1"));
  EXPECT_CALL(*mock_obj, get_last_modified_iso())
      .WillOnce(Return("last_modified"));
  EXPECT_CALL(*mock_obj, get_md5()).WillOnce(Return("abcd"));
  EXPECT_CALL(*mock_obj, get_content_length_str()).WillOnce(Return("1024"));
  EXPECT_CALL(*mock_obj, get_storage_class()).WillOnce(Return("STANDARD"));
  EXPECT_CALL(*mock_obj, get_canonical_id()).Times(1);
  EXPECT_CALL(*mock_obj, get_account_name()).WillOnce(Return("s3user"));

  std::string response = response_under_test->get_xml(
      "requestor_can_id", "owner_xyz", "requestor_id");

  // Generic xml checks
  CHECK_XML_RESPONSE;

  // Following are extra checks in response for V2 style specific
  // Check StartAfter in response
  EXPECT_THAT(response, HasSubstr("start-after-obj"));
  // Check ContinuationToken in response
  EXPECT_THAT(response, HasSubstr("efabdcrghxyeefgh89jghr1"));
  // Expect Owner field in the response (since fetch-owner=true)
  EXPECT_THAT(response, HasSubstr("<Owner>"));
  EXPECT_THAT(response, HasSubstr("s3user"));

  // Since is_truncated is false, response should not have next continuation
  // token
  EXPECT_THAT(response, Not(HasSubstr("Next_efabdcrghxyeefgh89jghr1")));
}
