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

#include <libxml/parser.h>
#include <libxml/xmlmemory.h>

#include "s3_log.h"
#include "s3_put_bucket_body.h"

S3PutBucketBody::S3PutBucketBody(std::string &xml)
    : xml_content(xml), is_valid(false) {
  s3_log(S3_LOG_DEBUG, "", "Constructor\n");
  parse_and_validate();
}

bool S3PutBucketBody::isOK() { return is_valid; }

bool S3PutBucketBody::parse_and_validate() {
  /* Sample body:
  <CreateBucketConfiguration xmlns="http://s3.amazonaws.com/doc/2006-03-01/">
    <LocationConstraint>EU</LocationConstraint>
  </CreateBucketConfiguration >
  */
  s3_log(S3_LOG_DEBUG, "", "Parsing put bucket body\n");

  if (xml_content.empty()) {
    // Default location intended.
    location_constraint = "us-west-2";
    is_valid = true;
    return true;
  }
  s3_log(S3_LOG_DEBUG, "", "Parsing xml request = %s\n", xml_content.c_str());
  xmlDocPtr document = xmlParseDoc((const xmlChar *)xml_content.c_str());
  if (document == NULL) {
    s3_log(S3_LOG_WARN, "", "S3PutBucketBody XML request body Invalid.\n");
    is_valid = false;
    return false;
  }

  xmlNodePtr root_node = xmlDocGetRootElement(document);

  // Validate the root node
  if (root_node == NULL ||
      xmlStrcmp(root_node->name,
                (const xmlChar *)"CreateBucketConfiguration")) {
    s3_log(S3_LOG_WARN, "", "S3PutBucketBody XML request body Invalid.\n");
    xmlFreeDoc(document);
    is_valid = false;
    return false;
  }

  // Get location if specified.
  xmlNodePtr child = root_node->xmlChildrenNode;
  xmlChar *key = NULL;
  while (child != NULL) {
    if ((!xmlStrcmp(child->name, (const xmlChar *)"LocationConstraint"))) {
      key = xmlNodeGetContent(child);
      if (key == NULL) {
        s3_log(S3_LOG_WARN, "", "S3PutBucketBody XML request body Invalid.\n");
        xmlFree(key);
        xmlFreeDoc(document);
        is_valid = false;
        return false;
      }
      location_constraint = (char *)key;
      is_valid = true;
      xmlFree(key);
      break;
    } else {
      child = child->next;
    }
  }
  xmlFreeDoc(document);
  return is_valid;
}

std::string S3PutBucketBody::get_location_constraint() {
  return location_constraint;
}
