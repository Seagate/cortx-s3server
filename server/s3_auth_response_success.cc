/*
 * COPYRIGHT 2016 SEAGATE LLC
 *
 * THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
 * HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE TECHNOLOGY
 * LIMITED, ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
 * THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
 * BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
 * USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
 * EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.
 *
 * YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
 * THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
 * http://www.seagate.com/contact
 *
 * Original author:  Rajesh Nambiar   <rajesh.nambiar@seagate.com>
 * Author:  Kaustubh Deorukhkar   <kaustubh.deorukhkar@seagate.com>
 * Original creation date: 10-Mar-2016
 */

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "s3_auth_response_success.h"
#include "s3_log.h"

S3AuthResponseSuccess::S3AuthResponseSuccess(std::string& xml) : xml_content(xml), is_valid(false) {
  s3_log(S3_LOG_DEBUG, "Constructor\n");
  is_valid = parse_and_validate();
}

bool S3AuthResponseSuccess::isOK() {
  return is_valid;
}

const std::string& S3AuthResponseSuccess::get_user_name() {
  return user_name;
}

const std::string& S3AuthResponseSuccess::get_user_id() {
  return user_id;
}

const std::string& S3AuthResponseSuccess::get_account_name() {
  return account_name;
}

const std::string& S3AuthResponseSuccess::get_account_id() {
  return account_id;
}

const std::string& S3AuthResponseSuccess::get_signature_sha256() {
  return signature_SHA256;
}

const std::string& S3AuthResponseSuccess::get_request_id() {
  return request_id;
}

bool S3AuthResponseSuccess::parse_and_validate() {
  s3_log(S3_LOG_DEBUG, "Parsing Auth server response\n");

  if (xml_content.empty()) {
    s3_log(S3_LOG_ERROR, "XML response is NULL\n");
    is_valid = false;
    return false;
  }

  s3_log(S3_LOG_DEBUG, "Parsing xml = %s\n", xml_content.c_str());
  xmlDocPtr document = xmlParseDoc((const xmlChar*)xml_content.c_str());
  if (document == NULL ) {
    s3_log(S3_LOG_ERROR, "Auth response xml body is invalid.\n");
    is_valid = false;
    return is_valid;
  }

  xmlNodePtr root_node = xmlDocGetRootElement(document);
  if(root_node == NULL) {
    s3_log(S3_LOG_ERROR, "Auth response xml body is invalid.\n");
    xmlFreeDoc(document);
    is_valid = false;
    return is_valid;
  }

  xmlChar *key = NULL;
  xmlNodePtr child = root_node->xmlChildrenNode;

  while (child != NULL) {
    if ((!xmlStrcmp(child->name, (const xmlChar *)"AuthenticateUserResult"))) {
      for (xmlNode *child_node = child->children; child_node != NULL; child_node = child_node->next) {
        key = xmlNodeGetContent(child_node);
        if ((!xmlStrcmp(child_node->name, (const xmlChar *)"UserId"))) {
          s3_log(S3_LOG_DEBUG, "UserId = %s\n", (const char*)key);
          user_id = (const char*)key;
        } else if ((!xmlStrcmp(child_node->name, (const xmlChar *)"UserName"))) {
          s3_log(S3_LOG_DEBUG, "UserName = %s\n", (const char*)key);
          user_name = (const char*)key;
        } else if ((!xmlStrcmp(child_node->name, (const xmlChar *)"AccountName"))) {
          s3_log(S3_LOG_DEBUG, "AccountName = %s\n", (const char*)key);
          account_name = (const char*)key;
        } else if((!xmlStrcmp(child_node->name, (const xmlChar *)"AccountId"))) {
          s3_log(S3_LOG_DEBUG, "AccountId =%s\n", (const char*)key);
          account_id = (const char*)key;
        } else if((!xmlStrcmp(child_node->name, (const xmlChar *)"SignatureSHA256"))) {
          s3_log(S3_LOG_DEBUG, "SignatureSHA256 =%s\n", (const char*)key);
          signature_SHA256 = (const char*)key;
        }

        if(key != NULL) {
          xmlFree(key);
          key = NULL;
        }
      }  // for
    } else if ((!xmlStrcmp(child->name, (const xmlChar *)"ResponseMetadata"))) {
      for (xmlNode *child_node = child->children; child_node != NULL; child_node = child_node->next) {
        key = xmlNodeGetContent(child_node);
        if ((!xmlStrcmp(child_node->name, (const xmlChar *)"RequestId"))) {
          s3_log(S3_LOG_DEBUG, "RequestId = %s\n", (const char*)key);
          request_id = (const char*)key;
        }
        if(key != NULL) {
          xmlFree(key);
          key = NULL;
        }
      }  // for
    } else if ((!xmlStrcmp(child->name, (const xmlChar *)"AuthorizeUserResult"))) {
      for (xmlNode *child_node = child->children; child_node != NULL; child_node = child_node->next) {
        key = xmlNodeGetContent(child_node);
        if ((!xmlStrcmp(child_node->name, (const xmlChar *)"UserId"))) {
          s3_log(S3_LOG_DEBUG, "UserId = %s\n", (const char*)key);
          user_id = (const char*)key;
        } else if ((!xmlStrcmp(child_node->name, (const xmlChar *)"UserName"))) {
          s3_log(S3_LOG_DEBUG, "UserName = %s\n", (const char*)key);
          user_name = (const char*)key;
        } else if ((!xmlStrcmp(child_node->name, (const xmlChar *)"AccountName"))) {
          s3_log(S3_LOG_DEBUG, "AccountName = %s\n", (const char*)key);
          account_name = (const char*)key;
        } else if((!xmlStrcmp(child_node->name, (const xmlChar *)"AccountId"))) {
          s3_log(S3_LOG_DEBUG, "AccountId =%s\n", (const char*)key);
          account_id = (const char*)key;
        }

        if(key != NULL) {
          xmlFree(key);
          key = NULL;
        }
      }
    }
    child = child->next;
  }
  xmlFreeDoc(document);
  if (user_name.empty() || user_id.empty() || account_name.empty() || account_id.empty()) {
    // We dont have enough user info from auth server.
    s3_log(S3_LOG_FATAL, "Auth server returned partial User info for authorization result.\n");
    is_valid = false;
  } else {
    s3_log(S3_LOG_DEBUG, "Auth server returned complete User info.\n");
    is_valid = true;
  }
  return is_valid;
}
