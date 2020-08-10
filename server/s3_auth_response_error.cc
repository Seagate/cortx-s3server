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

#include <utility>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>

#include "s3_auth_response_error.h"
#include "s3_log.h"

S3AuthResponseError::S3AuthResponseError(std::string xml)
    : xml_content(std::move(xml)) {
  s3_log(S3_LOG_DEBUG, "", "Constructor\n");
  is_valid = parse_and_validate();
}

S3AuthResponseError::S3AuthResponseError(std::string error_code_,
                                         std::string error_message_,
                                         std::string request_id_)
    : is_valid(false),
      error_code(std::move(error_code_)),
      error_message(std::move(error_message_)),
      request_id(std::move(request_id_)) {
  s3_log(S3_LOG_DEBUG, "", "Constructor\n");
}

bool S3AuthResponseError::isOK() const { return is_valid; }

const std::string& S3AuthResponseError::get_code() const { return error_code; }

const std::string& S3AuthResponseError::get_message() const {
  return error_message;
}

const std::string& S3AuthResponseError::get_request_id() const {
  return request_id;
}

bool S3AuthResponseError::parse_and_validate() {
  s3_log(S3_LOG_DEBUG, "", "Parsing Auth server Error response\n");

  if (xml_content.empty()) {
    s3_log(S3_LOG_ERROR, "", "XML response is NULL\n");
    is_valid = false;
    return false;
  }

  s3_log(S3_LOG_DEBUG, "", "Parsing error xml = %s\n", xml_content.c_str());
  xmlDocPtr document = xmlParseDoc((const xmlChar*)xml_content.c_str());
  if (document == NULL) {
    s3_log(S3_LOG_ERROR, "", "Auth response xml body is invalid.\n");
    is_valid = false;
    return is_valid;
  }

  xmlNodePtr root_node = xmlDocGetRootElement(document);
  if (root_node == NULL) {
    s3_log(S3_LOG_ERROR, "", "Auth response xml body is invalid.\n");
    xmlFreeDoc(document);
    is_valid = false;
    return is_valid;
  }

  xmlChar* key = NULL;
  xmlNodePtr child = root_node->xmlChildrenNode;

  while (child != NULL) {
    if ((!xmlStrcmp(child->name, (const xmlChar*)"Error"))) {
      for (xmlNode* child_node = child->children; child_node != NULL;
           child_node = child_node->next) {
        s3_log(S3_LOG_DEBUG, "", "child->name = %s\n",
               (const char*)child->name);
        key = xmlNodeGetContent(child_node);
        if ((!xmlStrcmp(child_node->name, (const xmlChar*)"Code"))) {
          s3_log(S3_LOG_DEBUG, "", "Code = %s\n", (const char*)key);
          error_code = (const char*)key;
        } else if ((!xmlStrcmp(child_node->name, (const xmlChar*)"Message"))) {
          s3_log(S3_LOG_DEBUG, "", "Message = %s\n", (const char*)key);
          error_message = (const char*)key;
        }
        if (key != NULL) {
          xmlFree(key);
          key = NULL;
        }
      }
    } else if ((!xmlStrcmp(child->name, (const xmlChar*)"RequestId"))) {
      key = xmlNodeGetContent(child);
      s3_log(S3_LOG_DEBUG, "", "RequestId = %s\n", (const char*)key);
      request_id = (const char*)key;
      if (key != NULL) {
        xmlFree(key);
        key = NULL;
      }
    }
    child = child->next;
  }  // while
  xmlFreeDoc(document);
  if (error_code.empty()) {
    is_valid = false;
  } else {
    is_valid = true;
  }
  return is_valid;
}
