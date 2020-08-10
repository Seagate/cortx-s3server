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

#include "s3_auth_response_success.h"
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include <memory>
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::Eq;
using ::testing::StrEq;

class S3AuthResponseSuccessTest : public testing::Test {
 protected:
  S3AuthResponseSuccessTest() {
    auth_response_under_test =
        std::make_shared<S3AuthResponseSuccess>(auth_success_xml);
  }

  std::shared_ptr<S3AuthResponseSuccess> auth_response_under_test;
  std::string auth_success_xml =
      "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>"
      "<AuthenticateUserResponse "
      "xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">"
      "<AuthenticateUserResult><UserId>123</UserId><UserName>tester</UserName>"
      "<AccountId>12345</AccountId><AccountName>s3_test</AccountName>"
      "<SignatureSHA256>UvRt9pMPxbsSfHzEi9CIBOmDxJs=</SignatureSHA256></"
      "AuthenticateUserResult>"
      "<ResponseMetadata><RequestId>1111</RequestId></ResponseMetadata></"
      "AuthenticateUserResponse>";
};

// To validate if constructor has initialized internal field
// correctly for valid XML.
TEST_F(S3AuthResponseSuccessTest, ConstructorTest) {
  EXPECT_TRUE(auth_response_under_test->isOK());
}

// Validate uer name retrieved from XML.
TEST_F(S3AuthResponseSuccessTest, GetUserName) {
  EXPECT_EQ(auth_response_under_test->get_user_name(), "tester");
}

// Validate user id parsed from XML.
TEST_F(S3AuthResponseSuccessTest, GetUserId) {
  EXPECT_EQ(auth_response_under_test->get_user_id(), "123");
}

// Validate account name parsed from XML.
TEST_F(S3AuthResponseSuccessTest, GetAccountName) {
  EXPECT_EQ(auth_response_under_test->get_account_name(), "s3_test");
}

// Validate account id parsed from XML.
TEST_F(S3AuthResponseSuccessTest, GetAccountId) {
  EXPECT_EQ(auth_response_under_test->get_account_id(), "12345");
}

// Validate SHA256 signature parsed from XML.
TEST_F(S3AuthResponseSuccessTest, GetSignatureSHA256) {
  EXPECT_EQ(auth_response_under_test->get_signature_sha256(),
            "UvRt9pMPxbsSfHzEi9CIBOmDxJs=");
}

// Validate request id parsed from xml.
TEST_F(S3AuthResponseSuccessTest, GetRequestId) {
  EXPECT_EQ(auth_response_under_test->get_request_id(), "1111");
}

// Test with empty XML.
TEST_F(S3AuthResponseSuccessTest, ValidateMustFailForEmptyXML) {
  std::string empty_string("");
  auth_response_under_test =
      std::make_shared<S3AuthResponseSuccess>(empty_string);
  EXPECT_FALSE(auth_response_under_test->isOK());
}

// Test with invalid XML.
TEST_F(S3AuthResponseSuccessTest, ValidateMustFailForInvalidXML) {
  std::string invalid_auth_success_xml =
      "<?xml version=\"1.0\" encoding=\"UTF-8\" "
      "standalone=\"no\"?><ErrorResponse ";
  auth_response_under_test =
      std::make_shared<S3AuthResponseSuccess>(invalid_auth_success_xml);
  EXPECT_FALSE(auth_response_under_test->isOK());
}

// Test with no error code field.
TEST_F(S3AuthResponseSuccessTest, ValidateMustFailForEmptyUserName) {
  std::string invalid_auth_success_xml =
      "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>"
      "<AuthenticateUserResponse "
      "xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">"
      "<AuthenticateUserResult><UserId>123</UserId>"
      "<AccountId>12345</AccountId><AccountName>s3_test</AccountName>"
      "<SignatureSHA256>UvRt9pMPxbsSfHzEi9CIBOmDxJs=</SignatureSHA256></"
      "AuthenticateUserResult>"
      "<ResponseMetadata><RequestId>1111</RequestId></ResponseMetadata></"
      "AuthenticateUserResponse>";
  auth_response_under_test =
      std::make_shared<S3AuthResponseSuccess>(invalid_auth_success_xml);
  EXPECT_FALSE(auth_response_under_test->isOK());
}

// Test with no error code field.
TEST_F(S3AuthResponseSuccessTest, ValidateMustFailForMissingUserName) {
  std::string invalid_auth_success_xml =
      "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>"
      "<AuthenticateUserResponse "
      "xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">"
      "<AuthenticateUserResult><UserId>123</UserId><UserName></UserName>"
      "<AccountId>12345</AccountId><AccountName>s3_test</AccountName>"
      "<SignatureSHA256>UvRt9pMPxbsSfHzEi9CIBOmDxJs=</SignatureSHA256></"
      "AuthenticateUserResult>"
      "<ResponseMetadata><RequestId>1111</RequestId></ResponseMetadata></"
      "AuthenticateUserResponse>";
  auth_response_under_test =
      std::make_shared<S3AuthResponseSuccess>(invalid_auth_success_xml);
  EXPECT_FALSE(auth_response_under_test->isOK());
}

// Test with no error code field.
TEST_F(S3AuthResponseSuccessTest, ValidateMustFailForEmptyUserId) {
  std::string invalid_auth_success_xml =
      "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>"
      "<AuthenticateUserResponse "
      "xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">"
      "<AuthenticateUserResult><UserId></UserId><UserName>tester</UserName>"
      "<AccountId>12345</AccountId><AccountName>s3_test</AccountName>"
      "<SignatureSHA256>UvRt9pMPxbsSfHzEi9CIBOmDxJs=</SignatureSHA256></"
      "AuthenticateUserResult>"
      "<ResponseMetadata><RequestId>1111</RequestId></ResponseMetadata></"
      "AuthenticateUserResponse>";
  auth_response_under_test =
      std::make_shared<S3AuthResponseSuccess>(invalid_auth_success_xml);
  EXPECT_FALSE(auth_response_under_test->isOK());
}

// Test with no error code field.
TEST_F(S3AuthResponseSuccessTest, ValidateMustFailForMissingUserId) {
  std::string invalid_auth_success_xml =
      "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>"
      "<AuthenticateUserResponse "
      "xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">"
      "<AuthenticateUserResult><UserName>tester</UserName>"
      "<AccountId>12345</AccountId><AccountName>s3_test</AccountName>"
      "<SignatureSHA256>UvRt9pMPxbsSfHzEi9CIBOmDxJs=</SignatureSHA256></"
      "AuthenticateUserResult>"
      "<ResponseMetadata><RequestId>1111</RequestId></ResponseMetadata></"
      "AuthenticateUserResponse>";
  auth_response_under_test =
      std::make_shared<S3AuthResponseSuccess>(invalid_auth_success_xml);
  EXPECT_FALSE(auth_response_under_test->isOK());
}

// Test with no error code field.
TEST_F(S3AuthResponseSuccessTest, ValidateMustFailForEmptyAccountName) {
  std::string invalid_auth_success_xml =
      "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>"
      "<AuthenticateUserResponse "
      "xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">"
      "<AuthenticateUserResult><UserId>123</UserId><UserName>tester</UserName>"
      "<AccountId>12345</AccountId><AccountName></AccountName>"
      "<SignatureSHA256>UvRt9pMPxbsSfHzEi9CIBOmDxJs=</SignatureSHA256></"
      "AuthenticateUserResult>"
      "<ResponseMetadata><RequestId>1111</RequestId></ResponseMetadata></"
      "AuthenticateUserResponse>";
  auth_response_under_test =
      std::make_shared<S3AuthResponseSuccess>(invalid_auth_success_xml);
  EXPECT_FALSE(auth_response_under_test->isOK());
}

// Test with no error code field.
TEST_F(S3AuthResponseSuccessTest, ValidateMustFailForMissingAccountName) {
  std::string invalid_auth_success_xml =
      "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>"
      "<AuthenticateUserResponse "
      "xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">"
      "<AuthenticateUserResult><UserId>123</UserId><UserName>tester</UserName>"
      "<AccountId>12345</AccountId>"
      "<SignatureSHA256>UvRt9pMPxbsSfHzEi9CIBOmDxJs=</SignatureSHA256></"
      "AuthenticateUserResult>"
      "<ResponseMetadata><RequestId>1111</RequestId></ResponseMetadata></"
      "AuthenticateUserResponse>";
  auth_response_under_test =
      std::make_shared<S3AuthResponseSuccess>(invalid_auth_success_xml);
  EXPECT_FALSE(auth_response_under_test->isOK());
}
