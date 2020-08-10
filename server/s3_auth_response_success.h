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

#pragma once

#ifndef __S3_SERVER_S3_AUTH_RESPONSE_SUCCESS_H__
#define __S3_SERVER_S3_AUTH_RESPONSE_SUCCESS_H__

#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include <gtest/gtest_prod.h>
#include <string>

class S3AuthResponseSuccess {
  std::string xml_content;
  bool is_valid;

  std::string user_name;
  std::string canonical_id;
  std::string user_id;
  std::string account_name;
  std::string account_id;
  std::string signature_SHA256;
  std::string request_id;
  std::string acl;
  std::string email;
  bool alluserrequest;

  bool parse_and_validate();
  void set_auth_parameters(xmlNode* child_node);

 public:
  S3AuthResponseSuccess(std::string& xml);
  bool isOK();

  const std::string& get_user_name();
  const std::string& get_email();
  const std::string& get_canonical_id();
  const std::string& get_user_id();
  const std::string& get_account_name();
  const std::string& get_account_id();
  const std::string& get_signature_sha256();
  const std::string& get_request_id();
  const std::string& get_acl();

  FRIEND_TEST(S3AuthResponseSuccessTest, ConstructorTest);
  FRIEND_TEST(S3AuthResponseSuccessTest, GetUserName);
  FRIEND_TEST(S3AuthResponseSuccessTest, GetAccountName);
  FRIEND_TEST(S3AuthResponseSuccessTest, GetAccountId);
  FRIEND_TEST(S3AuthResponseSuccessTest, GetSignatureSHA256);
  FRIEND_TEST(S3AuthResponseSuccessTest, GetRequestId);
  FRIEND_TEST(S3AuthResponseSuccessTest, EnsureErrorCodeSetForEmptyXML);
  FRIEND_TEST(S3AuthResponseSuccessTest, ValidateMustFailForEmptyXM);
  FRIEND_TEST(S3AuthResponseSuccessTest, ValidateMustFailForInvalidXML);
  FRIEND_TEST(S3AuthResponseSuccessTest, ValidateMustFailForEmptyUserName);
  FRIEND_TEST(S3AuthResponseSuccessTest, ValidateMustFailForMissingUserName);
  FRIEND_TEST(S3AuthResponseSuccessTest, ValidateMustFailForEmptyUserId);
  FRIEND_TEST(S3AuthResponseSuccessTest, ValidateMustFailForMissingUserId);
  FRIEND_TEST(S3AuthResponseSuccessTest, ValidateMustFailForEmptyAccountName);
  FRIEND_TEST(S3AuthResponseSuccessTest, ValidateMustFailForMissingAccountName);
};

#endif
