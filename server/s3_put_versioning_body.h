/*
 * Copyright (c) 2021 Seagate Technology LLC and/or its Affiliates
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

#ifndef __S3_SERVER_S3_PUT_VERSIONING_BODY_H__
#define __S3_SERVER_S3_PUT_VERSIONING_BODY_H__

#include <gtest/gtest_prod.h>
#include <string>
#include <map>
#include <libxml/xmlmemory.h>

class S3PutVersioningBody {
  std::string xml_content;
  std::string request_id;
  std::string versioning_status;
  bool is_valid;
  std::string s3_error;
  bool parse_and_validate();

 public:
  S3PutVersioningBody(const std::string& xml, const std::string& request);
  virtual bool isOK();
  virtual bool read_status_node(xmlNodePtr& sub_child);
  virtual bool validate_bucket_xml_versioning_status(
      std::string& versioning_status);
  virtual const std::string& get_versioning_status();
  std::string get_additional_error_information();

  // For Testing purpose
  FRIEND_TEST(S3PutVersioningBodyTest, ValidateRequestBodyXml);
  FRIEND_TEST(S3PutVersioningBodyTest, ValidateVersioningEnabledRequest);
  FRIEND_TEST(S3PutVersioningBodyTest, ValidateVersioningSuspendedRequest);
  FRIEND_TEST(S3PutVersioningBodyTest,
              ValidateEmptyVersioningConfigurationXmlTag);
  FRIEND_TEST(S3PutVersioningBodyTest, ValidateEmptyStatusXmlTag);
  FRIEND_TEST(S3PutVersioningBodyTest, ValidateUnversionedStatus);
  FRIEND_TEST(S3PutVersioningBodyTest, ValidateMFADeleteCase);
};

#endif
