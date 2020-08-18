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

#include "s3_auth_response_success.h"
#include "s3_log.h"

S3AuthResponseSuccess::S3AuthResponseSuccess(std::string &xml)
    : xml_content(xml), is_valid(false) {
  s3_log(S3_LOG_DEBUG, "", "Constructor\n");
  alluserrequest = false;
  is_valid = parse_and_validate();
}

bool S3AuthResponseSuccess::isOK() { return is_valid; }

const std::string &S3AuthResponseSuccess::get_user_name() { return user_name; }

const std::string &S3AuthResponseSuccess::get_canonical_id() {
  return canonical_id;
}

const std::string &S3AuthResponseSuccess::get_email() { return email; }

const std::string &S3AuthResponseSuccess::get_user_id() { return user_id; }

const std::string &S3AuthResponseSuccess::get_account_name() {
  return account_name;
}

const std::string &S3AuthResponseSuccess::get_account_id() {
  return account_id;
}

const std::string &S3AuthResponseSuccess::get_signature_sha256() {
  return signature_SHA256;
}

const std::string &S3AuthResponseSuccess::get_request_id() {
  return request_id;
}

const std::string &S3AuthResponseSuccess::get_acl() { return acl; }

void S3AuthResponseSuccess::set_auth_parameters(xmlNode *child_node) {
  xmlChar *key;
  if (child_node == NULL) {
    s3_log(S3_LOG_DEBUG, "", "child_node is NULL\n");
    return;
  }
  key = xmlNodeGetContent(child_node);
  if (key != NULL) {
    if ((!xmlStrcmp(child_node->name, (const xmlChar *)"UserId"))) {
      s3_log(S3_LOG_DEBUG, "", "UserId = %s\n", (const char *)key);
      user_id = (const char *)key;
    } else if ((!xmlStrcmp(child_node->name, (const xmlChar *)"UserName"))) {
      s3_log(S3_LOG_DEBUG, "", "UserName = %s\n", (const char *)key);
      user_name = (const char *)key;
    } else if ((!xmlStrcmp(child_node->name, (const xmlChar *)"Email"))) {
      s3_log(S3_LOG_DEBUG, "", "Email = %s\n", (const char *)key);
      email = (const char *)key;
    } else if ((!xmlStrcmp(child_node->name, (const xmlChar *)"CanonicalId"))) {
      s3_log(S3_LOG_DEBUG, "", "CanonicalId = %s\n", (const char *)key);
      canonical_id = (const char *)key;
    } else if ((!xmlStrcmp(child_node->name, (const xmlChar *)"AccountName"))) {
      s3_log(S3_LOG_DEBUG, "", "AccountName = %s\n", (const char *)key);
      account_name = (const char *)key;
    } else if ((!xmlStrcmp(child_node->name, (const xmlChar *)"AccountId"))) {
      s3_log(S3_LOG_DEBUG, "", "AccountId =%s\n", (const char *)key);
      account_id = (const char *)key;
    } else if ((!xmlStrcmp(child_node->name,
                           (const xmlChar *)"SignatureSHA256"))) {
      s3_log(S3_LOG_DEBUG, "", "SignatureSHA256 =%s\n", (const char *)key);
      signature_SHA256 = (const char *)key;
    } else if ((!xmlStrcmp(child_node->name, (const xmlChar *)"ACL"))) {
      s3_log(S3_LOG_DEBUG, "", "ACL =%s\n", (const char *)key);
      acl = (const char *)key;
    } else if ((!xmlStrcmp(child_node->name,
                           (const xmlChar *)"AllUserRequest"))) {
      //<Alluserrequest> true/false </Alluserrequest>
      s3_log(S3_LOG_DEBUG, "", "Alluserrequest =%s\n", (const char *)key);
      alluserrequest = (const char *)key;
    }
    xmlFree(key);
  }
}

bool S3AuthResponseSuccess::parse_and_validate() {
  s3_log(S3_LOG_DEBUG, "", "Parsing Auth server response\n");

  if (xml_content.empty()) {
    s3_log(S3_LOG_ERROR, "", "XML response is NULL\n");
    is_valid = false;
    return false;
  }

  s3_log(S3_LOG_DEBUG, "", "Parsing xml = %s\n", xml_content.c_str());
  xmlDocPtr document = xmlParseDoc((const xmlChar *)xml_content.c_str());
  if (document == NULL) {
    s3_log(S3_LOG_ERROR, "", "Auth response xml body is invalid.\n");
    is_valid = false;
    return is_valid;
  }

  xmlNodePtr root_node = xmlDocGetRootElement(document);
  if (root_node == NULL) {
    s3_log(S3_LOG_ERROR, "", "Auth response xml body is invalid.\n");
    xmlFreeDoc(document);
    xmlCleanupCharEncodingHandlers();
    xmlCleanupParser();
    is_valid = false;
    return is_valid;
  }

  xmlNodePtr child = root_node->xmlChildrenNode;

  while (child != NULL) {
    if ((!xmlStrcmp(child->name, (const xmlChar *)"AuthenticateUserResult"))) {
      for (xmlNode *child_node = child->children; child_node != NULL;
           child_node = child_node->next) {
        set_auth_parameters(child_node);
      }  // for
    } else if ((!xmlStrcmp(child->name, (const xmlChar *)"ResponseMetadata"))) {
      for (xmlNode *child_node = child->children; child_node != NULL;
           child_node = child_node->next) {
        xmlChar *key = xmlNodeGetContent(child_node);
        if (key != NULL) {
          if ((!xmlStrcmp(child_node->name, (const xmlChar *)"RequestId"))) {
            s3_log(S3_LOG_DEBUG, "", "RequestId = %s\n", (const char *)key);
            request_id = (const char *)key;
          }
          xmlFree(key);
        }
      }  // for
    } else if ((!xmlStrcmp(child->name,
                           (const xmlChar *)"AuthorizeUserResult"))) {
      for (xmlNode *child_node = child->children; child_node != NULL;
           child_node = child_node->next) {
        set_auth_parameters(child_node);
        }
    }
    child = child->next;
  }
  xmlFreeDoc(document);
  xmlCleanupCharEncodingHandlers();
  xmlCleanupParser();

  if (!alluserrequest) {
    if (user_name.empty() || user_id.empty() || account_name.empty() ||
        account_id.empty()) {
      // We dont have enough user info from auth server.
      s3_log(
          S3_LOG_ERROR, "-",
          "Auth server returned partial User info for authorization result.\n");
      is_valid = false;
    } else {
      s3_log(S3_LOG_DEBUG, "", "Auth server returned complete User info.\n");
      is_valid = true;
    }
  } else if (!acl.empty()) {
    is_valid = true;
  }
  return is_valid;
}
