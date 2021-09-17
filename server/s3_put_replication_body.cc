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

  // bucket_tags.clear();
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

  Json::Value rule_array;
  // Processing each rule node
  while (rule_node != NULL) {
    s3_log(S3_LOG_DEBUG, request_id, " Child Node = %s", rule_node->name);
    if ((!xmlStrcmp(rule_node->name, (const xmlChar *)"Rule"))) {
      unsigned long rule_child_count = xmlChildElementCount(rule_node);
      s3_log(S3_LOG_DEBUG, request_id, "Rule node children count=%ld",
             rule_child_count);
      s3_log(S3_LOG_WARN, request_id, "Two feilds are mandatory\n");
      if (rule_child_count < 2) {
        s3_log(S3_LOG_WARN, request_id, "XML request body Invalid.\n");
        xmlFreeDoc(document);
        is_valid = false;
        return is_valid;
      }

      is_valid = read_rule_node(rule_node);
      if (!is_valid) {
        s3_log(S3_LOG_WARN, request_id, "XML request body Invalid.\n");
        return is_valid;
      }

      // Writing into json file
      Json::Value rule_object;
      rule_object["ID"] = rule_id;
      rule_object["Status"] = rule_status;
      rule_object["Priority"] = rule_priority;
      rule_object["DeleteMarkerReplication"]["Status"] = del_rep_status;
      rule_object["Destination"]["Bucket"] = bucket_str;
      rule_array.append(rule_object);
      rule_node = rule_node->next;
    }
  }

  ReplicationConfiguration["Rules"] = rule_array;

  xmlFreeDoc(document);
  return is_valid;
}

std::string S3PutReplicationBody::get_replication_configuration_as_json() {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);
  Json::FastWriter fastWriter;
  return fastWriter.write(ReplicationConfiguration);
}

ChildNodes S3PutReplicationBody::convert_str_to_enum(const unsigned char *str) {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);

  if (strcmp(reinterpret_cast<const char *>(str), "Undefined") == 0)
    return Undefined;
  else if (strcmp(reinterpret_cast<const char *>(str), "ID") == 0)
    return ID;
  else if (strcmp(reinterpret_cast<const char *>(str), "Status") == 0)
    return Status;
  else if (strcmp(reinterpret_cast<const char *>(str), "Priority") == 0)
    return Priority;
  else if (strcmp(reinterpret_cast<const char *>(str),
                  "DeleteMarkerReplication") == 0)
    return DeleteMarkerReplication;
  else if (strcmp(reinterpret_cast<const char *>(str), "Destination") == 0)
    return Destination;

  return Undefined;
}

bool S3PutReplicationBody::read_rule_node(xmlNodePtr &rule_node) {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);
  // Validate child nodes of rule
  is_priority_present = false;
  is_delete_marker_replication_present = false;
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
        }
      } break;

      case Status: {
        // Status is mandatory field
        if ((!xmlStrcmp(rule_child_node->name, (const xmlChar *)"Status"))) {
          s3_log(S3_LOG_WARN, request_id, "rule_status = %s. \n",
                 rule_child_node->name);
          // validating status value
          xmlChar *val = xmlNodeGetContent(rule_child_node);
          rule_status = reinterpret_cast<char *>(val);
          s3_log(S3_LOG_DEBUG, request_id, "Child Node = %s",
                 rule_status.c_str());
          bool bstatus = validate_status(rule_status);
          if (bstatus == false) {
            s3_log(S3_LOG_WARN, request_id, "Invalid rule_status.\n");
            return false;
          }
        }
      } break;
      case Priority: {
        // Priority is not a mandatory field
        if ((!xmlStrcmp(rule_child_node->name, (const xmlChar *)"Priority"))) {
          s3_log(S3_LOG_WARN, request_id, "rule_priority = %s. \n",
                 rule_child_node->name);

          xmlChar *val = xmlNodeGetContent(rule_child_node);
          std::string rule_priority_str = reinterpret_cast<char *>(val);
          rule_priority = atoi(rule_priority_str.c_str());
          if (!rule_priority) {
            s3_log(S3_LOG_WARN, request_id, "XML rule priority Invalid.\n");
            return false;
          }
          is_priority_present = true;
        }
      } break;

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

      case Destination: {
        // Destination is  a mandatory field
        if ((!xmlStrcmp(rule_child_node->name,
                        (const xmlChar *)"Destination"))) {
          // validate Destination's child nodes
          s3_log(S3_LOG_WARN, request_id, "Destination = %s\n",
                 rule_child_node->name);
          bool bstatus = validate_destination_node(rule_child_node);
          if (bstatus == false) {
            s3_log(S3_LOG_WARN, request_id, "XML request body Invalid.\n");
            return false;
          }
        }

      } break;
    }
    rule_child_node = rule_child_node->next;
  } while (rule_child_node != nullptr);
  return true;
}

bool S3PutReplicationBody::validate_destination_node(
    xmlNodePtr destination_node) {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);
  // bucket is a mandatory field
  xmlNodePtr bucket_node = destination_node->xmlChildrenNode;
  if (bucket_node != NULL) {
    s3_log(S3_LOG_DEBUG, request_id, "Child Node = %s", bucket_node->name);
    if ((xmlStrcmp(bucket_node->name, (const xmlChar *)"Bucket"))) {
      s3_log(S3_LOG_WARN, request_id, "XML request body Invalid.\n");
      return false;
    } else {
      xmlChar *val = xmlNodeGetContent(bucket_node);
      bucket_str = reinterpret_cast<char *>(val);
    }
  }
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
    } else {
      // validate status value
      xmlChar *val = xmlNodeGetContent(status_node);
      del_rep_status = reinterpret_cast<char *>(val);
      bool bstatus = validate_status(del_rep_status);
      if (bstatus == false) {
        s3_log(S3_LOG_WARN, request_id,
               "Invalid delete marker replication status.\n");
        return false;
      }
    }
  }
  return true;
}

bool S3PutReplicationBody::validate_status(const std::string &status) {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);
  if (rule_status.empty()) {
    s3_log(S3_LOG_WARN, request_id, "XML rule_status  Empty.\n");
    return false;
  }
  /*Rule Status::
    Specifies whether the rule is enabled.
    Valid Values: Enabled | Disabled */

  /*Delete Marker replication Status::
      Indicates whether to replicate delete markers.
      Valid Values: Enabled | Disabled */

  if (rule_status.compare("Enabled") == 0 ||
      rule_status.compare("Disabled") == 0) {
    return true;
  }
  s3_log(S3_LOG_WARN, request_id,
         "rule_status = %s. XML rule_status  Invalid.\n", rule_status.c_str());
  return false;
}