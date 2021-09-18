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

#ifndef __S3_SERVER_S3_PUT_REPLICATION_BODY_H__
#define __S3_SERVER_S3_PUT_REPLICATION_BODY_H__

#include <json/json.h>
#include <gtest/gtest_prod.h>
#include <string>
#include <map>
#include <libxml/xmlmemory.h>

// #define BUCKET_MAX_TAGS 50
// #define OBJECT_MAX_TAGS 10
// #define TAG_KEY_MAX_LENGTH 128
// #define TAG_VALUE_MAX_LENGTH 256
enum ChildNodes {
  Undefined,
  ID,
  Status,
  Priority,
  DeleteMarkerReplication,
  Destination
};
class S3PutReplicationBody {
  std::string xml_content;
  std::string request_id;
  std::string role_str;
  std::string rule_id;
  std::string rule_status;
  int rule_priority;
  std::string del_rep_status;
  std::string bucket_str;
  bool is_valid;
  bool parse_and_validate();
  Json::Value ReplicationConfiguration;

 public:
  S3PutReplicationBody(std::string& xml, std::string& request);
  virtual bool isOK();
  virtual bool read_rule_node(xmlNodePtr& sub_child);
  bool validate_destination_node(xmlNodePtr destination_node);
  bool validate_delete_marker_replication_status(xmlNodePtr del_rep_node);
  bool validate_rule_status(const std::string& status, xmlNodePtr& node_name,
                            int& number_of_rules_enable);
  ChildNodes convert_str_to_enum(const unsigned char* str);
  virtual std::string get_replication_configuration_as_json();
};

#endif
