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

#ifndef __S3_SERVER_S3_BUCKET_REMOTE_ADD_BODY_H__
#define __S3_SERVER_S3_BUCKET_REMOTE_ADD_BODY_H__

#include <gtest/gtest_prod.h>
#include <string>
#include <map>
#include <libxml/xmlmemory.h>

/*#define BUCKET_MAX_TAGS 50
#define OBJECT_MAX_TAGS 10
#define TAG_KEY_MAX_LENGTH 128
#define TAG_VALUE_MAX_LENGTH 256*/

class S3BucketRemoteAddBody {
  std::string xml_content;
  std::string request_id;
  // std::map<std::string, std::string> bucket_tags;
  std::string alias_name, creds_str;
  bool is_valid;
  bool parse_and_validate();

 public:
  S3BucketRemoteAddBody(std::string& xml, std::string& request);
  bool validate_node_content(std::string node_content);
  virtual bool isOK();
  std::string get_creds();
  std::string get_alias_name();

  // To Do: Add unit test cases
};

#endif
