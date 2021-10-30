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

enum ChildNodes {
  Undefined,
  ID,
  Status,
  Priority,
  DeleteMarkerReplication,
  Destination,
  Filter,
  Prefix,
  ExistingObjectReplication,
  SourceSelectionCriteria
};

enum DestinationChildNodes {
  UndefinedDestChildNode,
  AccessControlTranslation,
  Account,
  Bucket,
  EncryptionConfiguration,
  Metrics,
  ReplicationTime,
  StorageClass
};
class S3PutReplicationBody {
  std::string xml_content;
  std::string request_id;
  std::string role_str;
  std::string rule_id;
  std::set<std::string> rule_id_set;
  std::string rule_prefix;
  std::multimap<std::string, std::string> bucket_tags;
  std::string rule_status;
  int rule_priority;
  std::set<int> rule_priority_set;
  std::string del_rep_status;
  std::string bucket_str;
  bool is_valid;
  std::string s3_error;
  bool parse_and_validate();
  Json::Value ReplicationConfiguration;
  int rule_number_cnt{-1};
  std::vector<std::string> vec_bucket_names;

 public:
  S3PutReplicationBody(std::string &xml, std::string &request);
  virtual bool isOK();
  virtual bool read_rule_node(xmlNodePtr &sub_child,
                              std::map<std::string, std::string> &rule_tags,
                              bool &is_and_node_present,
                              bool &is_rule_prefix_present,
                              bool &is_filter_empty, bool is_priority_present);
  bool validate_destination_node(xmlNodePtr destination_node);
  bool validate_delete_marker_replication_status(xmlNodePtr del_rep_node);
  bool validate_rule_status(const std::string &status, xmlNodePtr &node_name,
                            int &number_of_rules_enable);
  ChildNodes convert_str_to_enum(const unsigned char *str);
  DestinationChildNodes convert_str_to_Destination_enum(
      const unsigned char *str);
  virtual std::string get_replication_configuration_as_json();
  virtual std::vector<std::string> &get_destination_bucket_list();
  bool validate_child_nodes_of_filter(
      xmlNodePtr filter_node, std::map<std::string, std::string> &rule_tags,
      bool &is_and_node_present, bool &is_rule_prefix_present,
      bool &is_rule_tag_present);
  bool validate_prefix_node(xmlNodePtr prefix_node);
  bool validate_tag_node(xmlNodePtr &tag_node,
                         std::map<std::string, std::string> &rule_tags,
                         bool &is_rule_tag_present);
  bool read_key_value_node(xmlNodePtr &tag_node,
                           std::map<std::string, std::string> &rule_tags);
  std::string get_additional_error_information();
  void not_implemented_rep_field(xmlNodePtr destination_node, const char *str);

  // For Testing purpose
  FRIEND_TEST(S3PutReplicationBodyTest, ValidateRequestBodyXml);
  FRIEND_TEST(S3PutReplicationBodyTest, ValidateNumberOfRuleNodes);
  FRIEND_TEST(S3PutReplicationBodyTest, ValidateAtleastOneRuleIsEnabled);
  FRIEND_TEST(S3PutReplicationBodyTest, ValidateRuleStatusValue);
  FRIEND_TEST(S3PutReplicationBodyTest, ValidateEmptyRuleStatusValue);
  FRIEND_TEST(S3PutReplicationBodyTest, ValidateDeleteReplicationStatusValue);
  FRIEND_TEST(S3PutReplicationBodyTest,
              ValidateDeleteReplicationInvalidStatusValue);
  FRIEND_TEST(S3PutReplicationBodyTest, ValidateEmptyDestinationNode);
  FRIEND_TEST(S3PutReplicationBodyTest, ValidateEmptyBucketNode);
  FRIEND_TEST(S3PutReplicationBodyTest, ValidateRequestSkippedRuleID);
  FRIEND_TEST(S3PutReplicationBodyTest, ValidateRequestSkippedAndNode);
  FRIEND_TEST(S3PutReplicationBodyTest, InvalidAndNode);
  FRIEND_TEST(S3PutReplicationBodyTest, ValidateRequestForPrefix);
  FRIEND_TEST(S3PutReplicationBodyTest, ValidateRequestWithOneTag);
  FRIEND_TEST(S3PutReplicationBodyTest, InvalidTagNodes);
  FRIEND_TEST(S3PutReplicationBodyTest, ValidateDuplicateKeyRequest);
  FRIEND_TEST(S3PutReplicationBodyTest, InvalidEmptyKeyRequest);
  FRIEND_TEST(S3PutReplicationBodyTest, InvalidEmptyFilter);
  FRIEND_TEST(S3PutReplicationBodyTest, ValidateRequestForTag);
  FRIEND_TEST(S3PutReplicationBodyTest, ValidateRequestForFilter);
  FRIEND_TEST(S3PutReplicationBodyTest, ValidateRequestForMultipleRules);
  FRIEND_TEST(S3PutReplicationBodyTest, ValidateDuplicateRuleIDRequest);
  FRIEND_TEST(S3PutReplicationBodyTest, ValidateDuplicateRulePriority);
  FRIEND_TEST(S3PutReplicationBodyTest,
              ValidateRequestForDuplicateKeyInMultipleRules);
  FRIEND_TEST(S3PutReplicationBodyTest, ValidateStorageClassNotSupported);
  FRIEND_TEST(S3PutReplicationBodyTest,
              ValidateEncryptionConfigurationNotSupported);
  FRIEND_TEST(S3PutReplicationBodyTest,
              ValidateAccessControlTranslationNotImplemented);
  FRIEND_TEST(S3PutReplicationBodyTest, ValidateAccountNotImplemented);
  FRIEND_TEST(S3PutReplicationBodyTest, ValidateMetricsNotImplemented);
  FRIEND_TEST(S3PutReplicationBodyTest, ValidateReplicationTimeNotImplemented);
  FRIEND_TEST(S3PutReplicationBodyTest,
              ValidateExistingObjectReplicationNotImplemented);
  FRIEND_TEST(S3PutReplicationBodyTest,
              ValidateSourceSelectionCriteriaNotImplemented);
  FRIEND_TEST(S3PutReplicationBodyTest, ValidateV1Request);
};

#endif
