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
#include <json/json.h>
#include <cctype>
#include <algorithm>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>

#include <regex>

#include "s3_log.h"
#include "s3_put_replication_body.h"
#include "s3_common_utilities.h"

// This might change in future
#define NO_OF_RULES_SUPPORTED 50
#define _STR_MATCHED(str, plain_str) \
  strcmp(reinterpret_cast<const char *>(str), plain_str) == 0

S3PutReplicationBody::S3PutReplicationBody(std::string &xml,
                                           std::string &request)
    : xml_content(xml), request_id(request), is_valid(false) {

  s3_log(S3_LOG_DEBUG, request_id, "%s Ctor\n", __func__);
  if (!xml.empty()) {
    parse_and_validate();
  }
}

bool S3PutReplicationBody::isOK() { return is_valid; }

bool S3PutReplicationBody::parse_and_validate() {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);
  /* Sample body(Basic configuration):
  <ReplicationConfiguration>
    <Role>IAM-role-ARN</Role>
    <Rule>
        <ID>Rule-1</ID>
        <Status>rule-Enabled-or-Disabled</Status>
        <Priority>integer</Priority>
        <DeleteMarkerReplication>
           <Status>Disabled</Status>
        </DeleteMarkerReplication>
        <Filter>
         <And>
            <Prefix>string</Prefix>
            <Tag>
               <Key>string</Key>
               <Value>string</Value>
            </Tag>
            ...
         </And>
        </Filter>
        <Destination>
           <Bucket>arn:aws:s3:::bucket-name</Bucket>
        </Destination>
     </Rule>
     <Rule>
         ...
     </Rule>
     ...
</ReplicationConfiguration>

  */
  s3_log(S3_LOG_DEBUG, request_id, "Parsing put bucket replication body\n");

  // clear priority and rule id sets
  rule_id_set.clear();
  rule_priority_set.clear();
  vec_bucket_names.clear();
  is_valid = false;

  if (xml_content.empty()) {
    s3_log(S3_LOG_WARN, request_id, "XML request body Empty.\n");
    return is_valid;
  }
  s3_log(S3_LOG_DEBUG, request_id, "Parsing xml request = %s\n",
         xml_content.c_str());
  xmlDocPtr document = xmlParseDoc((const xmlChar *)xml_content.c_str());
  if (document == NULL) {
    s3_log(S3_LOG_WARN, request_id, "XML request body Invalid.\n");
    return is_valid;
  }

  xmlNodePtr replication_config_node = xmlDocGetRootElement(document);

  s3_log(S3_LOG_DEBUG, request_id, "Root Node = %s",
         replication_config_node->name);
  // Validate the root node
  if (replication_config_node == NULL ||
      xmlStrcmp(replication_config_node->name,
                (const xmlChar *)"ReplicationConfiguration")) {
    s3_log(S3_LOG_WARN, request_id, "XML request body Invalid.\n");
    xmlFreeDoc(document);
    return is_valid;
  }

  // Validate child nodes
  xmlNodePtr role_node = replication_config_node->xmlChildrenNode;
  if (role_node == NULL) {
    s3_log(S3_LOG_DEBUG, request_id, "Child Node = %s", role_node->name);
    if ((xmlStrcmp(role_node->name, (const xmlChar *)"Role"))) {
      s3_log(S3_LOG_WARN, request_id, "XML request body Invalid.\n");
      return is_valid;
    }
  } else {

    xmlChar *val = xmlNodeGetContent(role_node);
    role_str = reinterpret_cast<char *>(val);
    if (role_str.empty()) {
      s3_log(S3_LOG_WARN, request_id, "XML role is Empty.\n");
      return false;
    }
    // Role is yet to be supported in Cortx
    s3_log(S3_LOG_WARN, request_id, "role_str = %s\n", role_str.c_str());
    ReplicationConfiguration["Role"] = role_str;
  }

  // Validate sub-child nodes
  role_node = role_node->next;
  xmlNodePtr rule_node = role_node;
  int number_of_rules_present = 0;
  std::map<std::string, std::string> rule_tags;
  bool is_and_node_present = false;
  bool is_rule_prefix_present = false;
  bool is_filter_empty = false;
  bool is_priority_present = false;
  Json::Value rule_array;
  // Processing each rule node
  while (rule_node != NULL) {
    s3_log(S3_LOG_DEBUG, request_id, " Child Node = %s", rule_node->name);
    if ((!xmlStrcmp(rule_node->name, (const xmlChar *)"Rule"))) {
      unsigned long rule_child_count = xmlChildElementCount(rule_node);
      s3_log(S3_LOG_DEBUG, request_id, "Rule node children count=%lu",
             rule_child_count);
      s3_log(S3_LOG_WARN, request_id, "Two feilds are mandatory\n");
      if (rule_child_count < 2) {
        s3_log(S3_LOG_WARN, request_id, "XML request body Invalid.\n");
        xmlFreeDoc(document);
        is_valid = false;
        return is_valid;
      }

      is_valid = read_rule_node(rule_node, rule_tags, is_and_node_present,
                                is_rule_prefix_present, is_filter_empty,
                                is_priority_present);
      if (!is_valid) {
        s3_log(S3_LOG_WARN, request_id, "XML request body Invalid.\n");
        return is_valid;
      }

      // Writing into json file
      Json::Value rule_object;
      if (!rule_id.empty()) rule_object["ID"] = rule_id;
      if (!rule_status.empty()) rule_object["Status"] = rule_status;
      if (is_priority_present) rule_object["Priority"] = rule_priority;

      if (!del_rep_status.empty())
        rule_object["DeleteMarkerReplication"]["Status"] = del_rep_status;
      if (!bucket_str.empty())
        rule_object["Destination"]["Bucket"] = bucket_str;

      Json::Value and_node;
      // Append rule tags in rule_array if present
      Json::Value filter_node;
      Json::Value tag_node(Json::arrayValue);
      Json::Value tag_object;
      // If tag,and nodes are present in filter
      if (!rule_tags.empty() && is_and_node_present &&
          !is_rule_prefix_present) {

        for (const auto &tag : rule_tags) {

          tag_object["Key"] = tag.first;
          tag_object["Value"] = tag.second;
          tag_node.append(tag_object);
        }
        and_node["Tag"] = tag_node;
        filter_node["And"] = and_node;
        rule_object["Filter"] = filter_node;

      } else if (!rule_tags.empty() && is_and_node_present &&
                 is_rule_prefix_present) {
        // If tag,and,Prefix nodes are present  in filter
        Json::Value prefix_node;
        and_node["Prefix"] = rule_prefix;
        for (const auto &tag : rule_tags) {
          tag_object["Key"] = tag.first;
          tag_object["Value"] = tag.second;
          tag_node.append(tag_object);
        }
        and_node["Tag"] = tag_node;
        filter_node["And"] = and_node;
        rule_object["Filter"] = filter_node;

      } else if (!rule_tags.empty() && !is_and_node_present) {
        // If only tag node is present in filter
        for (const auto &tag : rule_tags) {

          filter_node["Tag"]["Key"] = tag.first;
          filter_node["Tag"]["Value"] = tag.second;
        }
        rule_object["Filter"] = filter_node;
      } else if (is_rule_prefix_present && !is_and_node_present) {

        // If only Prefix node is present in filter
        filter_node["Prefix"] = rule_prefix;
        rule_object["Filter"] = filter_node;
      }

      // Filter can be empty
      if (is_filter_empty) {
        rule_object["Filter"];
      }

      rule_array.append(rule_object);
      number_of_rules_present++;
      rule_node = rule_node->next;
    } else {
      is_valid = false;
      s3_log(S3_LOG_WARN, request_id,
             "XML request body Invalid.Atleast one rule needed in replication "
             "policy\n");

      return is_valid;
    }

    // clear rule tags
    rule_tags.clear();
  }

  if (number_of_rules_present > NO_OF_RULES_SUPPORTED) {
    is_valid = false;
    s3_log(S3_LOG_WARN, request_id,
           "XML request body Invalid.Number of rules exceeds in replication "
           "policy\n");

    return is_valid;
  }

  ReplicationConfiguration["Rules"] = rule_array;

  xmlFreeDoc(document);
  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
  return is_valid;
}

std::string S3PutReplicationBody::get_replication_configuration_as_json() {
  Json::FastWriter fastWriter;
  return fastWriter.write(ReplicationConfiguration);
}

std::vector<std::string> &S3PutReplicationBody::get_destination_bucket_list() {
  return vec_bucket_names;
}

std::string S3PutReplicationBody::get_additional_error_information() {
  s3_log(S3_LOG_INFO, request_id, "s3_error in body = %s\n", s3_error.c_str());
  return s3_error;
}

ChildNodes S3PutReplicationBody::convert_str_to_enum(const unsigned char *str) {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);

  if (_STR_MATCHED(str, "Undefined"))
    return Undefined;
  else if (_STR_MATCHED(str, "ID"))
    return ID;
  else if (_STR_MATCHED(str, "Status"))
    return Status;
  else if (_STR_MATCHED(str, "Priority"))
    return Priority;
  else if (_STR_MATCHED(str, "DeleteMarkerReplication"))
    return DeleteMarkerReplication;
  else if (_STR_MATCHED(str, "Destination"))
    return Destination;
  else if (_STR_MATCHED(str, "Filter"))
    return Filter;
  else if (_STR_MATCHED(str, "Prefix"))
    return Prefix;
  else if (_STR_MATCHED(str, "ExistingObjectReplication"))
    return ExistingObjectReplication;
  else if (_STR_MATCHED(str, "SourceSelectionCriteria"))
    return SourceSelectionCriteria;
  return Undefined;
}

bool S3PutReplicationBody::read_rule_node(
    xmlNodePtr &rule_node, std::map<std::string, std::string> &rule_tags,
    bool &is_and_node_present, bool &is_rule_prefix_present,
    bool &is_filter_empty, bool is_priority_present) {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);
  rule_number_cnt++;
  s3_log(S3_LOG_INFO, request_id, "Reading rule[%d]\n", rule_number_cnt);
  // Validate child nodes of rule
  int number_of_rules_enable = 0;
  bool is_destination_node_present = false;
  bool is_delete_marker_replication_present = false;
  bool is_rule_tag_present = false;
  bool is_filter_present = false;
  bool is_v1_prefix_present = false;
  xmlNodePtr rule_child_node = rule_node->xmlChildrenNode;
  do {

    if (rule_child_node == NULL) {
      s3_log(S3_LOG_WARN, request_id, "XML request body Invalid.\n");
      return false;
    }
    s3_log(S3_LOG_DEBUG, request_id, "Rule Child Node = %s",
           rule_child_node->name);
    ChildNodes child_nodes =
        convert_str_to_enum((const xmlChar *)(rule_child_node->name));

    s3_log(S3_LOG_DEBUG, request_id, "child_nodes enum value = %d",
           child_nodes);

    switch (child_nodes) {
      case Undefined: {
        s3_log(S3_LOG_WARN, request_id, "XML request body Invalid.\n");
        return false;
      } break;
      case ID: {
        // ID is not mandatory field
        if ((!xmlStrcmp(rule_child_node->name, (const xmlChar *)"ID"))) {
          s3_log(S3_LOG_WARN, request_id, "rule_ID = %s. \n",
                 rule_child_node->name);
          xmlChar *val = xmlNodeGetContent(rule_child_node);

          rule_id = reinterpret_cast<char *>(val);
          if (rule_id.empty()) {
            s3_log(S3_LOG_WARN, request_id, "XML rule id Empty.\n");
            return false;
          }
          if (rule_id_set.find(rule_id) != rule_id_set.end()) {

            s3_log(S3_LOG_WARN, request_id,
                   "XML request body Invalid: Rule-Id  duplicated.\n");
            s3_error = "InvalidArgumentDuplicateRuleID";
            return false;
          }
          s3_log(S3_LOG_DEBUG, request_id, "Rule id %s ", rule_id.c_str());
          rule_id_set.insert(rule_id);
        }
      } break;

      case Status:
        // Status is mandatory field
        if ((!xmlStrcmp(rule_child_node->name, (const xmlChar *)"Status"))) {
          s3_log(S3_LOG_WARN, request_id, "rule_status = %s. \n",
                 rule_child_node->name);
          // validating status value
          xmlChar *val = xmlNodeGetContent(rule_child_node);
          rule_status = reinterpret_cast<char *>(val);
          s3_log(S3_LOG_DEBUG, request_id, "Child Node = %s",
                 rule_status.c_str());
          bool bstatus = validate_rule_status(rule_status, rule_child_node,
                                              number_of_rules_enable);
          if (bstatus == false) {
            s3_log(S3_LOG_WARN, request_id, "Invalid rule_status.\n");
            return false;
          }
        }
        break;
      case Priority:
        // Priority is not a mandatory field
        if ((!xmlStrcmp(rule_child_node->name, (const xmlChar *)"Priority"))) {
          s3_log(S3_LOG_WARN, request_id, "rule_priority = %s. \n",
                 rule_child_node->name);

          xmlChar *val = xmlNodeGetContent(rule_child_node);
          std::string rule_priority_str = reinterpret_cast<char *>(val);
          if (!S3CommonUtilities::stoi(rule_priority_str.c_str(),
                                       rule_priority) ||
              rule_priority < 0) {
            s3_log(S3_LOG_WARN, request_id, "XML rule priority is Invalid.\n");
            s3_error = "InvalidRequestForPriority";
            return false;
          }
          if (rule_priority_set.find(rule_priority) !=
              rule_priority_set.end()) {

            s3_log(S3_LOG_WARN, request_id,
                   "XML request body Invalid: rule_priority  duplicated.\n");
            s3_error = "InvalidArgumentDuplicateRulePriority";
            return false;
          }
          s3_log(S3_LOG_DEBUG, request_id, "rule_priority %d ", rule_priority);
          rule_priority_set.insert(rule_priority);
          is_priority_present = true;
        }
        break;

      case DeleteMarkerReplication: {
        // DeleteMarkerReplication is not a mandatory field
        if ((!xmlStrcmp(rule_child_node->name,
                        (const xmlChar *)"DeleteMarkerReplication"))) {
          // validate status value
          bool bstatus =
              validate_delete_marker_replication_status(rule_child_node);
          if (bstatus == false) {
            s3_log(S3_LOG_WARN, request_id, "XML request body Invalid.\n");
            return false;
          }
          is_delete_marker_replication_present = true;
        }
      } break;

      case Filter: {

        if ((!xmlStrcmp(rule_child_node->name, (const xmlChar *)"Filter"))) {
          is_filter_present = true;
          // validate child nodes of filter
          unsigned long Filter_child_count =
              xmlChildElementCount(rule_child_node);
          // Filter can be empty
          if (Filter_child_count == 0) {
            is_filter_empty = true;
          } else {
            if (!validate_child_nodes_of_filter(
                     rule_child_node, rule_tags, is_and_node_present,
                     is_rule_prefix_present, is_rule_tag_present)) {

              s3_log(S3_LOG_WARN, request_id, "XML request body Invalid.\n");
              return false;
            }
          }
        }
      } break;

      case Prefix: {
        if ((!xmlStrcmp(rule_child_node->name, (const xmlChar *)"Prefix"))) {
          is_v1_prefix_present = true;
          s3_log(S3_LOG_WARN, request_id,
                 "Using v1 API when parsing bucket replication.\n");
          xmlChar *val = xmlNodeGetContent(rule_child_node);
          rule_prefix = reinterpret_cast<char *>(val);
          s3_log(S3_LOG_DEBUG, request_id, "rule_priority %d ", rule_priority);
        }
      } break;

      case Destination: {
        // Destination is  a mandatory field
        if ((!xmlStrcmp(rule_child_node->name,
                        (const xmlChar *)"Destination"))) {
          // validate Destination's child nodes
          s3_log(S3_LOG_WARN, request_id, "Destination = %s\n",
                 rule_child_node->name);
          is_destination_node_present = true;
          bool bstatus = validate_destination_node(rule_child_node);
          if (bstatus == false) {
            s3_log(S3_LOG_WARN, request_id, "XML request body Invalid.\n");
            return false;
          }
        }

      } break;
      case ExistingObjectReplication: {
        // Account is not a mandatory field
        // In future scope.(Not supported currently)
        not_implemented_rep_field(rule_child_node, "ExistingObjectReplication");
        return false;
      } break;

      case SourceSelectionCriteria: {
        // Account is not a mandatory field
        // In future scope.(Not supported currently)
        not_implemented_rep_field(rule_child_node, "SourceSelectionCriteria");
        return false;
      } break;
      default: {
        // Xml nodes should be one of the elements in Enum
        s3_log(S3_LOG_WARN, request_id, "XML request body Invalid.\n");
      }
    }  // end of switch cases

    rule_child_node = rule_child_node->next;
  } while (rule_child_node != nullptr);

  if (!is_destination_node_present) {
    s3_log(S3_LOG_WARN, request_id,
           "XML request body Invalid.Destination node not present\n");
    return false;
  }

  if (is_v1_prefix_present) {
    // This is the old v1 API version of prefix, which AWS supports for
    // backwards compatibility. It is mutually exclusive with Filter,
    // and also restricts the other options available. We check that none
    // of the newer options are used, and fudge the flags to make things
    // look like v2, with prefix expressed via Filter and default values
    // for DeleteMarkerReplication and Priority to mainain compatibility
    // with the old API behavior.

    if (is_filter_present || is_priority_present ||
        is_delete_marker_replication_present) {
      s3_log(S3_LOG_WARN, request_id,
             "v1 API only supports ID, Prefix, Status, "
             "SourceSelectionCriteria, and Destination.\n");
      return false;
    }

    is_filter_present = true;
    is_rule_prefix_present = true;
    is_priority_present = true;
    rule_priority = 0;
    is_delete_marker_replication_present = true;
    del_rep_status = "Enabled";
    s3_log(S3_LOG_WARN, request_id,
           "Falling back to defaults for v1 replication API: "
           "DeleteMarkerReplication Enabled, Priority 0.\n");
  }

  if (!is_filter_present) {
    s3_log(S3_LOG_WARN, request_id,
           "XML request body Invalid.Filter node not present\n");
    return false;
  }

  // If filter is present DeleteMarkerReplication and priority is mandetory
  if (!is_delete_marker_replication_present || !is_priority_present) {
    s3_log(S3_LOG_WARN, request_id,
           "XML request body Invalid.If filter is present then "
           "DeleteMarkerReplication and priority must be present\n");
    s3_error = "InvalidRequestForFilter";
    return false;
  }
  // If Tag present in filter then DeleteMarkerReplication must be disable
  if (is_rule_tag_present && (del_rep_status.compare("Disabled") != 0)) {
    s3_log(S3_LOG_WARN, request_id,
           "XML request body Invalid.If Tag present in filter then "
           "DeleteMarkerReplication must be disable\n");
    s3_error = "InvalidRequestForTagFilter";
    return false;
  }

  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
  return true;
}

bool S3PutReplicationBody::validate_child_nodes_of_filter(
    xmlNodePtr filter_node, std::map<std::string, std::string> &rule_tags,
    bool &is_and_node_present, bool &is_rule_prefix_present,
    bool &is_rule_tag_present) {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);
  xmlNodePtr child_node = filter_node->xmlChildrenNode;
  is_and_node_present = false;
  is_rule_prefix_present = false;
  is_rule_tag_present = false;
  unsigned long filter_child_count = xmlChildElementCount(filter_node);
  s3_log(S3_LOG_DEBUG, request_id, " filter children count=%lu",
         filter_child_count);

  if ((!xmlStrcmp(child_node->name, (const xmlChar *)"And"))) {
    s3_log(S3_LOG_DEBUG, request_id, "Child Node = %s", child_node->name);
    is_and_node_present = true;
    // count child nodes of And node - And child nodes should be greater than
    // two
    unsigned long and_child_count = xmlChildElementCount(child_node);
    s3_log(S3_LOG_DEBUG, request_id, " children count=%lu", and_child_count);
    if (and_child_count < 2) {
      s3_log(S3_LOG_WARN, request_id,
             "XML request body Invalid.Atlease two nodes required inside And "
             "node\n");
      return false;
    }
    xmlNodePtr and_child_node = child_node->xmlChildrenNode;
    s3_log(S3_LOG_DEBUG, request_id, "and_child_node name =%s",
           and_child_node->name);
    do {
      if ((!xmlStrcmp(and_child_node->name, (const xmlChar *)"Prefix"))) {
        // validate prefix node
        validate_prefix_node(and_child_node);
        is_rule_prefix_present = true;

      } else if ((!xmlStrcmp(and_child_node->name, (const xmlChar *)"Tag"))) {
        // validate Tag node
        s3_log(S3_LOG_WARN, request_id, "Validating Tag node-1\n");
        bool is_tag_valid =
            validate_tag_node(and_child_node, rule_tags, is_rule_tag_present);
        if (!is_tag_valid) {
          s3_log(S3_LOG_WARN, request_id, "Tags validation failed.\n");
          return false;
        }
      } else {
        s3_log(S3_LOG_WARN, request_id,
               "XML request body Invalid: unknown And child Node.\n");
        return false;
      }

      if (and_child_node->next != NULL) {
        and_child_node = and_child_node->next;
      } else {
        break;
      }
    } while (and_child_node && and_child_node->name);

  } else if ((!xmlStrcmp(child_node->name, (const xmlChar *)"Prefix"))) {
    s3_log(S3_LOG_DEBUG, request_id, "Child Node = %s", child_node->name);
    // validate prefix node
    if (is_and_node_present == false && filter_child_count > 1) {
      s3_log(S3_LOG_WARN, request_id,
             "XML request body Invalid: Required And node.\n");
      return false;
    }
    validate_prefix_node(child_node);
    is_rule_prefix_present = true;

  } else if ((!xmlStrcmp(child_node->name, (const xmlChar *)"Tag"))) {
    s3_log(S3_LOG_DEBUG, request_id, "Child Node = %s", child_node->name);
    // validate Tag node
    if (is_and_node_present == false && filter_child_count > 1) {
      s3_log(S3_LOG_WARN, request_id,
             "XML request body Invalid: Required And node.\n");
      return false;
    }
    s3_log(S3_LOG_WARN, request_id, "Validating Tag node-2\n");
    bool is_tag_valid =
        validate_tag_node(child_node, rule_tags, is_rule_tag_present);
    if (!is_tag_valid) {
      s3_log(S3_LOG_WARN, request_id, "Tags validation failed.\n");
      return false;
    }

  } else {
    s3_log(S3_LOG_WARN, request_id,
           "XML request body Invalid: unknown Filter child Node.\n");
    return false;
  }

  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
  return true;
}

bool S3PutReplicationBody::validate_prefix_node(xmlNodePtr prefix_node) {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);
  xmlChar *val = xmlNodeGetContent(prefix_node);

  rule_prefix = reinterpret_cast<char *>(val);
  // prefix can be empty

  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
  return true;
}

bool S3PutReplicationBody::validate_tag_node(
    xmlNodePtr &tag_node, std::map<std::string, std::string> &rule_tags,
    bool &is_rule_tag_present) {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);

  if ((!xmlStrcmp(tag_node->name, (const xmlChar *)"Tag"))) {
    unsigned long key_count = xmlChildElementCount(tag_node);
    s3_log(S3_LOG_DEBUG, request_id, "Tag node children count=%lu", key_count);
    if (key_count != 2) {
      s3_log(S3_LOG_WARN, request_id, "Tag node is Invalid.\n");
      return false;
    }
    bool is_valid_key_value = read_key_value_node(tag_node, rule_tags);
    if (!is_valid_key_value) {
      s3_log(S3_LOG_WARN, request_id, "Key Value nodes in tag are Invalid.\n");
      return false;
    }
    if (!rule_tags.empty()) is_rule_tag_present = true;
  }

  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
  return true;
}
bool S3PutReplicationBody::read_key_value_node(
    xmlNodePtr &tag_node, std::map<std::string, std::string> &rule_tags) {
  // Validate key values node
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);
  xmlNodePtr key_value_node = tag_node->xmlChildrenNode;
  std::string tag_key, tag_value;
  while (key_value_node && key_value_node->name) {
    s3_log(S3_LOG_DEBUG, request_id, "Key_Value_node = %s",
           key_value_node->name);
    xmlChar *val = xmlNodeGetContent(key_value_node);

    if (!(xmlStrcmp(key_value_node->name, (const xmlChar *)"Key"))) {
      tag_key = reinterpret_cast<char *>(val);
    } else if (!(xmlStrcmp(key_value_node->name, (const xmlChar *)"Value"))) {
      tag_value = reinterpret_cast<char *>(val);
    } else {
      s3_log(S3_LOG_WARN, request_id,
             "XML request body Invalid: unknown tag %s.\n",
             reinterpret_cast<const char *>(key_value_node->name));
      xmlFree(val);
      return false;
    }
    xmlFree(val);
    // Only a single pair of Key-Value exists within Tag Node.
    key_value_node = key_value_node->next;
  }
  // Unlike key tag_value can be empty
  if (tag_key.empty()) {
    s3_log(S3_LOG_WARN, request_id, "XML request body Invalid: empty node.\n");
    return false;
  }

  if (rule_tags.count(tag_key) >= 1) {
    s3_log(S3_LOG_WARN, request_id,
           "XML request body Invalid: tag duplication.\n");
    s3_error = "InvalidTag";
    return false;
  }
  s3_log(S3_LOG_DEBUG, request_id, "Add tag %s = %s ", tag_key.c_str(),
         tag_value.c_str());
  rule_tags[tag_key] = tag_value;

  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
  return true;
}

DestinationChildNodes S3PutReplicationBody::convert_str_to_Destination_enum(
    const unsigned char *str) {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);

  if (_STR_MATCHED(str, "UndefinedDestChildNode"))
    return UndefinedDestChildNode;
  else if (_STR_MATCHED(str, "AccessControlTranslation"))
    return AccessControlTranslation;
  else if (_STR_MATCHED(str, "Account"))
    return Account;
  else if (_STR_MATCHED(str, "Bucket"))
    return Bucket;
  else if (_STR_MATCHED(str, "EncryptionConfiguration"))
    return EncryptionConfiguration;
  else if (_STR_MATCHED(str, "Metrics"))
    return Metrics;
  else if (_STR_MATCHED(str, "ReplicationTime"))
    return ReplicationTime;
  else if (_STR_MATCHED(str, "StorageClass"))
    return StorageClass;

  return UndefinedDestChildNode;
}

void S3PutReplicationBody::not_implemented_rep_field(xmlNodePtr dest_child_node,
                                                     const char *str) {
  if ((!xmlStrcmp(dest_child_node->name, (const xmlChar *)str))) {
    s3_log(S3_LOG_WARN, request_id,
           "XML request body Invalid: %s "
           "feature is not implemented.\n",
           str);
    s3_error = "ReplicationFieldNotImplemented";
  }
}
bool S3PutReplicationBody::validate_destination_node(
    xmlNodePtr destination_node) {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);
  // bucket is a mandatory field
  xmlNodePtr dest_child_node = destination_node->xmlChildrenNode;
  if (dest_child_node == NULL) {
    s3_log(S3_LOG_WARN, request_id, "XML request body Invalid.\n");
    return false;
  }
  while (dest_child_node != nullptr) {
    s3_log(S3_LOG_DEBUG, request_id, "Child Node = %s", dest_child_node->name);

    DestinationChildNodes dest_child_nodes = convert_str_to_Destination_enum(
        (const xmlChar *)(dest_child_node->name));

    s3_log(S3_LOG_DEBUG, request_id, "child_nodes enum value = %d",
           dest_child_nodes);
    switch (dest_child_nodes) {
      case UndefinedDestChildNode: {
        s3_log(S3_LOG_WARN, request_id, "XML request body Invalid.\n");
        return false;
      } break;

      case Bucket: {
        if ((!xmlStrcmp(dest_child_node->name, (const xmlChar *)"Bucket"))) {

          xmlChar *val = xmlNodeGetContent(dest_child_node);
          bucket_str = reinterpret_cast<char *>(val);
          if (bucket_str.empty()) {
            s3_log(S3_LOG_WARN, request_id, "XML bucket is Empty.\n");
            s3_error = "InvalidArgumentDestinationBucket";
            return false;
          }
          vec_bucket_names.push_back(bucket_str);
        }
      } break;

      case AccessControlTranslation: {
        // AccessControlTranslation is not a mandatory field
        // In future scope.(Not supported currently)
        not_implemented_rep_field(dest_child_node, "AccessControlTranslation");
        return false;
      } break;

      case Account: {
        // Account is not a mandatory field
        // In future scope.(Not supported currently)
        not_implemented_rep_field(dest_child_node, "Account");
        return false;
      } break;

      case EncryptionConfiguration: {
        // EncryptionConfiguration is not a mandatory field
        // Not supported currently.
        if ((!xmlStrcmp(dest_child_node->name,
                        (const xmlChar *)"EncryptionConfiguration"))) {
          s3_log(S3_LOG_WARN, request_id,
                 "XML request body Invalid: EncryptionConfiguration "
                 "feature is not implemented.\n");
          s3_error = "ReplicationOperationNotSupported";
          return false;
        }
      } break;

      case Metrics: {
        // Metrics is not a mandatory field
        // Not supported currently.
        not_implemented_rep_field(dest_child_node, "Metrics");
        return false;
      } break;

      case ReplicationTime: {
        // ReplicationTime is not a mandatory field
        // Not supported currently.
        not_implemented_rep_field(dest_child_node, "ReplicationTime");
        return false;
      } break;

      case StorageClass: {
        // StorageClass is not a mandatory field
        // Not supported currently.
        if ((!xmlStrcmp(dest_child_node->name,
                        (const xmlChar *)"StorageClass"))) {
          s3_log(S3_LOG_WARN, request_id,
                 "XML request body Invalid: StorageClass feature "
                 "is not implemented.\n");
          s3_error = "ReplicationOperationNotSupported";
          return false;
        }
      } break;
      default: {
        // Xml nodes should be one of the elements in Enum
        s3_log(S3_LOG_WARN, request_id, "XML request body Invalid.\n");
      }
    }

    dest_child_node = dest_child_node->next;
  }

  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
  return true;
}
bool S3PutReplicationBody::validate_delete_marker_replication_status(
    xmlNodePtr del_rep_node) {

  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);
  xmlNodePtr status_node = del_rep_node->xmlChildrenNode;
  if (status_node != NULL) {
    s3_log(S3_LOG_DEBUG, request_id, "Child Node = %s", status_node->name);
    if ((xmlStrcmp(status_node->name, (const xmlChar *)"Status"))) {
      s3_log(S3_LOG_WARN, request_id, "XML request body Invalid.\n");
      return false;
    }
    // validate status value
    /*Delete Marker replication Status::
    Indicates whether to replicate delete markers.
    Valid Values: Enabled | Disabled */
    xmlChar *val = xmlNodeGetContent(status_node);
    del_rep_status = reinterpret_cast<char *>(val);
    if (del_rep_status.compare("Enabled") == 0 ||
        del_rep_status.compare("Disabled") == 0) {
      return true;
    } else {
      s3_log(S3_LOG_WARN, request_id,
             "del_rep_status = %s. XML del_rep_status  Invalid.\n",
             del_rep_status.c_str());
      return false;
    }
  }
  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
  return true;
}

bool S3PutReplicationBody::validate_rule_status(const std::string &status,
                                                xmlNodePtr &node_name,
                                                int &number_of_rules_enable) {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);
  if (rule_status.empty()) {
    s3_log(S3_LOG_WARN, request_id, "XML rule_status  Empty.\n");
    return false;
  }
  /*Rule Status::
    Specifies whether the rule is enabled.
    Valid Values: Enabled | Disabled */

  if (rule_status.compare("Enabled") == 0 ||
      rule_status.compare("Disabled") == 0) {
    if ((!xmlStrcmp(node_name->name, (const xmlChar *)"Status")) &&
        (rule_status.compare("Enabled") == 0))
      number_of_rules_enable++;
    return true;
  }
  s3_log(S3_LOG_WARN, request_id,
         "rule_status = %s. XML rule_status  Invalid.\n", rule_status.c_str());
  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
  return false;
}
