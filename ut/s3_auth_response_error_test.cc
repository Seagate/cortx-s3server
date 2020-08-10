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

#include "s3_auth_response_error.h"
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include <memory>
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::Eq;
using ::testing::StrEq;

class S3AuthResponseErrorTest : public testing::Test {
 protected:
  S3AuthResponseErrorTest() {
    response_under_test = std::make_shared<S3AuthResponseError>(auth_error_xml);
  }
  std::shared_ptr<S3AuthResponseError> response_under_test;
  std::string auth_error_xml =
      "<?xml version=\"1.0\" encoding=\"UTF-8\" "
      "standalone=\"no\"?><ErrorResponse "
      "xmlns=\"https://iam.seagate.com/doc/2010-05-08/"
      "\"><Error><Code>SignatureDoesNotMatch</"
      "Code><Message>UnitTestErrorMessage.</Message>"
      "</Error><RequestId>1111</RequestId></ErrorResponse>";
};

// TO validate if internal fields of S3AuthResponseError are correctly set.
TEST_F(S3AuthResponseErrorTest, ConstructorTest) {
  EXPECT_TRUE(response_under_test->is_valid);
}

// To verify if error code field is correctly set.
TEST_F(S3AuthResponseErrorTest, GetErrorCode) {
  EXPECT_STREQ(response_under_test->get_code().c_str(),
               "SignatureDoesNotMatch");
}

// To verify if error message field is correctly set.
TEST_F(S3AuthResponseErrorTest, GetErrorMessage) {
  EXPECT_STREQ(response_under_test->get_message().c_str(),
               "UnitTestErrorMessage.");
}

// To verify if request id field is correctly set.
TEST_F(S3AuthResponseErrorTest, GetRequestId) {
  EXPECT_STREQ(response_under_test->get_request_id().c_str(), "1111");
}

// Test with empty XML.
TEST_F(S3AuthResponseErrorTest, ValidateMustFailForEmptyXML) {
  std::string empty_string("");
  response_under_test = std::make_shared<S3AuthResponseError>(empty_string);
  EXPECT_FALSE(response_under_test->is_valid);
}

// Test with invalid XML.
TEST_F(S3AuthResponseErrorTest, ValidateMustFailForInvalidXML) {
  std::string auth_error_xml =
      "<?xml version=\"1.0\" encoding=\"UTF-8\" "
      "standalone=\"no\"?><ErrorResponse ";
  response_under_test = std::make_shared<S3AuthResponseError>(auth_error_xml);
  EXPECT_FALSE(response_under_test->is_valid);
}

// Test with no error code field.
TEST_F(S3AuthResponseErrorTest, ValidateMustFailForEmptyErrorCode) {
  std::string auth_error_xml =
      "<?xml version=\"1.0\" encoding=\"UTF-8\" "
      "standalone=\"no\"?><ErrorResponse "
      "xmlns=\"https://iam.seagate.com/doc/2010-05-08/"
      "\"><Error><Code></"
      "Code><Message>UnitTestErrorMessage.</Message>"
      "</Error><RequestId>1111</RequestId></ErrorResponse>";
  response_under_test = std::make_shared<S3AuthResponseError>(auth_error_xml);
  EXPECT_FALSE(response_under_test->is_valid);
}

// Test with no error code field.
TEST_F(S3AuthResponseErrorTest, ValidateMustFailForMissingErrorCode) {
  std::string auth_error_xml =
      "<?xml version=\"1.0\" encoding=\"UTF-8\" "
      "standalone=\"no\"?><ErrorResponse "
      "xmlns=\"https://iam.seagate.com/doc/2010-05-08/"
      "\"><Error><NoCode></"
      "NoCode><Message>UnitTestErrorMessage.</Message>"
      "</Error><RequestId>1111</RequestId></ErrorResponse>";
  response_under_test = std::make_shared<S3AuthResponseError>(auth_error_xml);
  EXPECT_FALSE(response_under_test->is_valid);
}
