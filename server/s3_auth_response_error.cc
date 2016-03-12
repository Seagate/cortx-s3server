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

#include "s3_auth_response_error.h"
#include "s3_log.h"

S3AuthResponseError::S3AuthResponseError(std::string& xml) : xml_content(xml), is_valid(false) {
  s3_log(S3_LOG_DEBUG, "Constructor\n");
  is_valid = parse_and_validate();
}

bool S3AuthResponseError::isOK() {
  return is_valid;
}

const std::string& S3AuthResponseError::get_code() {
  return error_code;
}

const std::string& S3AuthResponseError::get_message() {
  return error_message;
}

const std::string& S3AuthResponseError::get_request_id() {
  return request_id;
}

bool S3AuthResponseError::parse_and_validate() {
  s3_log(S3_LOG_DEBUG, "Parsing Auth server Error response\n");

  if (xml_content.empty()) {
    s3_log(S3_LOG_ERROR, "XML response is NULL\n");
    is_valid = false;
    return false;
  }

  s3_log(S3_LOG_DEBUG, "Parsing error xml = %s\n", xml_content.c_str());
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
    s3_log(S3_LOG_DEBUG, "child->name = %s\n", (const char*)child->name);
    key = xmlNodeGetContent(child);
    if ((!xmlStrcmp(child->name, (const xmlChar *)"Code"))) {
      s3_log(S3_LOG_DEBUG, "Code = %s\n", (const char*)key);
      error_code = (const char*)key;
    } else if ((!xmlStrcmp(child->name, (const xmlChar *)"Message"))) {
      s3_log(S3_LOG_DEBUG, "Message = %s\n", (const char*)key);
      error_message = (const char*)key;
    } else if ((!xmlStrcmp(child->name, (const xmlChar *)"RequestId"))) {
      s3_log(S3_LOG_DEBUG, "RequestId = %s\n", (const char*)key);
      request_id = (const char*)key;
    }
    if(key != NULL) {
      xmlFree(key);
      key = NULL;
    }
    child = child->next;
  } // while
  xmlFreeDoc(document);
  if (error_code.empty()) {
    is_valid = false;
  } else {
    is_valid = true;
  }
  return is_valid;
}
