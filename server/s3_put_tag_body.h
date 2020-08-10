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

#ifndef __S3_SERVER_S3_PUT_TAG_BODY_H__
#define __S3_SERVER_S3_PUT_TAG_BODY_H__

#include <gtest/gtest_prod.h>
#include <string>
#include <map>
#include <libxml/xmlmemory.h>

#define BUCKET_MAX_TAGS 50
#define OBJECT_MAX_TAGS 10
#define TAG_KEY_MAX_LENGTH 128
#define TAG_VALUE_MAX_LENGTH 256

class S3PutTagBody {
  std::string xml_content;
  std::string request_id;
  std::map<std::string, std::string> bucket_tags;

  bool is_valid;
  bool parse_and_validate();

 public:
  S3PutTagBody(std::string& xml, std::string& request);
  virtual bool isOK();
  virtual bool read_key_value_node(xmlNodePtr& sub_child);
  virtual bool validate_bucket_xml_tags(
      std::map<std::string, std::string>& bucket_tags_as_map);
  virtual bool validate_object_xml_tags(
      std::map<std::string, std::string>& object_tags_as_map);
  virtual const std::map<std::string, std::string>& get_resource_tags_as_map();

  // For Testing purpose
  FRIEND_TEST(S3PutTagBodyTest, ValidateRequestBodyXml);
  FRIEND_TEST(S3PutTagBodyTest, ValidateRequestCompareContents);
  FRIEND_TEST(S3PutTagBodyTest, ValidateRepeatedKeysXml);
  FRIEND_TEST(S3PutTagBodyTest, ValidateEmptyTagSetXml);
  FRIEND_TEST(S3PutTagBodyTest, ValidateEmptyTagsXml);
  FRIEND_TEST(S3PutTagBodyTest, ValidateEmptyKeyXml);
  FRIEND_TEST(S3PutTagBodyTest, ValidateEmptyValueXml);
  FRIEND_TEST(S3PutTagBodyTest, ValidateValidRequestTags);
  FRIEND_TEST(S3PutTagBodyTest, ValidateRequestInvalidTagSize);
  FRIEND_TEST(S3PutTagBodyTest, ValidateRequestInvalidTagCount);
};

#endif
