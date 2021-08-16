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

#include <cctype>
#include <algorithm>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>

#include <regex>
#include "s3_log.h"
#include "s3_put_versioning_body.h"

S3PutVersioningBody::S3PutVersioningBody(std::string &xml, std::string &request)
    : xml_content(xml), request_id(request), is_valid(false) {

  s3_log(S3_LOG_DEBUG, request_id, "%s Ctor\n", __func__);
  if (!xml.empty()) {
    parse_and_validate();
  }
}

bool S3PutVersioningBody::isOK() { return is_valid; }

bool S3PutVersioningBody::parse_and_validate() {
  /* Sample body:
  <VersioningConfiguration>
   <MfaDelete>string</MfaDelete>
   <Status>string</Status>
</VersioningConfiguration>

  */
  s3_log(S3_LOG_DEBUG, request_id, "Parsing put bucket versioning body\n");

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

  xmlNodePtr VersioningConfiguration_node = xmlDocGetRootElement(document);

  s3_log(S3_LOG_DEBUG, request_id, "Root Node = %s",
         VersioningConfiguration_node->name);
  // Validate the root node
  if (VersioningConfiguration_node == NULL ||
      xmlStrcmp(VersioningConfiguration_node->name,
                (const xmlChar *)"VersioningConfiguration")) {
    s3_log(S3_LOG_WARN, request_id, "XML request body Invalid.\n");
    xmlFreeDoc(document);
    return is_valid;
  }

  // Validate child node
  xmlNodePtr Status_node = VersioningConfiguration_node->xmlChildrenNode;
  if (Status_node != NULL) {
    s3_log(S3_LOG_DEBUG, request_id, "Child Node = %s", Status_node->name);
    if ((xmlStrcmp(Status_node->name, (const xmlChar *)"Status"))) {

      is_valid = read_status_node(Status_node);
      if (!is_valid) {
        s3_log(S3_LOG_WARN, request_id, "XML request body Invalid.\n");
        return is_valid;
      }
    } else {
      s3_log(S3_LOG_WARN, request_id, "XML request body Invalid.\n");
      return is_valid;
    }
  }
  xmlFreeDoc(document);
  return is_valid;
}

bool S3PutVersioningBody::read_status_node(xmlNodePtr &Status_node) {
  // Validate Status node string
  // xmlNodePtr key_value_node = tag_node->xmlChildrenNode;
  std::string Status_value;
  if (!(xmlStrcmp(Status_node->name, (const xmlChar *)"Status"))) {
    xmlChar *val = xmlNodeGetContent(Status_node);
    Status_value = reinterpret_cast<char *>(val);
  }

  if (Status_value.empty()) {
    s3_log(S3_LOG_WARN, request_id, "XML request body Invalid: empty node.\n");
    return false;
  }

  versioning_Status = Status_value;

  return true;
}

bool S3PutVersioningBody::validate_bucket_xml_versioning_status(
    std::string &versioning_Status) {

  if (versioning_Status.empty()) {
    s3_log(S3_LOG_WARN, request_id, "XML Versioning Status Empty.\n");
    return false;
  }
  /*Versioning Status are::
  Enabled:: Enables versioning for the objects in the bucket. All objects added
  to the bucket receive a unique version ID.

  Suspended:: Disables versioning for the objects in the bucket. All objects
  added to the bucket receive the version ID null.

  Unversioned:: If the versioning state has never been set on a bucket, it has
  no versioning state*/
  if (versioning_Status.compare("Enabled") != 0 ||
      versioning_Status.compare("Suspended") != 0) {
    s3_log(S3_LOG_WARN, request_id, "XML Versioning Status Invalid.\n");
    return false;
  }
  return true;
}

const std::string &S3PutVersioningBody::get_versioning_status() {
  return versioning_Status;
}
