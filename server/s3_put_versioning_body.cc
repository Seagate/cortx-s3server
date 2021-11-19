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

#include <cctype>
#include <algorithm>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>

#include <regex>
#include "s3_log.h"
#include "s3_put_versioning_body.h"

S3PutVersioningBody::S3PutVersioningBody(const std::string &xml,
                                         const std::string &request)
    : xml_content(xml), request_id(request), is_valid(false), s3_error("") {

  s3_log(S3_LOG_DEBUG, request_id, "%s Ctor\n", __func__);
  if (!xml.empty()) {
    parse_and_validate();
  }
}

bool S3PutVersioningBody::isOK() { return is_valid; }

bool S3PutVersioningBody::parse_and_validate() {
  /* Sample body:
  <VersioningConfiguration>
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

  xmlNodePtr versioning_configuration_node = xmlDocGetRootElement(document);

  s3_log(S3_LOG_DEBUG, request_id, "Root Node = %s",
         versioning_configuration_node->name);
  // Validate the root node
  if (versioning_configuration_node == NULL ||
      xmlStrcmp(versioning_configuration_node->name,
                (const xmlChar *)"VersioningConfiguration")) {
    s3_log(S3_LOG_WARN, request_id, "XML request body Invalid.\n");
    xmlFreeDoc(document);
    return is_valid;
  }

  // Validate child node
  xmlNodePtr status_node = versioning_configuration_node->xmlChildrenNode;
  if (status_node != NULL) {
    s3_log(S3_LOG_DEBUG, request_id, "Child Node = %s", status_node->name);
    if ((!xmlStrcmp(status_node->name, (const xmlChar *)"Status"))) {

      is_valid = read_status_node(status_node);
      if (!is_valid) {
        s3_log(S3_LOG_WARN, request_id, "XML request body Invalid.\n");
        return is_valid;
      }
      //<MfaDelete>string</MfaDelete> MfaDelete operation is not supported.
      if (status_node->next != nullptr) {

        status_node = status_node->next;
        if ((!xmlStrcmp(status_node->name, (const xmlChar *)"MfaDelete"))) {
          s3_log(S3_LOG_WARN, request_id,
                 "MFADelete operation is not supported.\n");
          is_valid = false;
          s3_error = "OperationNotSupported";
          return is_valid;
        }
      }
    } else {
      s3_log(S3_LOG_WARN, request_id, "XML request body Invalid.\n");
      return is_valid;
    }
  }
  xmlFreeDoc(document);
  return is_valid;
}

std::string S3PutVersioningBody::get_additional_error_information() {
  s3_log(S3_LOG_INFO, request_id, "s3_error in body = %s\n", s3_error.c_str());
  return s3_error;
}

bool S3PutVersioningBody::read_status_node(xmlNodePtr &status_node) {
  s3_log(S3_LOG_INFO, request_id, "read_status_node \n");
  std::string new_versioning_status;
  if (!(xmlStrcmp(status_node->name, (const xmlChar *)"Status"))) {
    xmlChar *val = xmlNodeGetContent(status_node);
    new_versioning_status = reinterpret_cast<char *>(val);
  }

  if (new_versioning_status.empty()) {
    s3_log(S3_LOG_WARN, request_id, "XML request body Invalid: empty node.\n");
    return false;
  } else {
    s3_log(S3_LOG_INFO, request_id, "new_versioning_status = %s\n",
           new_versioning_status.c_str());
  }

  versioning_status = new_versioning_status;
  s3_log(S3_LOG_WARN, request_id, "versioning_status = %s \n",
         versioning_status.c_str());

  return true;
}

bool S3PutVersioningBody::validate_bucket_xml_versioning_status(
    std::string &versioning_status) {

  s3_log(S3_LOG_INFO, request_id, "validate_bucket_xml_versioning_status \n");

  if (versioning_status.empty()) {
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
  if (versioning_status == "Enabled" || versioning_status == "Suspended") {
    return true;
  }
  s3_log(S3_LOG_WARN, request_id,
         "versioning_status = %s. XML Versioning Status Invalid.\n",
         versioning_status.c_str());
  return false;
}

const std::string &S3PutVersioningBody::get_versioning_status() {
  return versioning_status;
}
