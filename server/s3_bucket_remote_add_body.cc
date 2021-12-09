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
#include "s3_bucket_remote_add_body.h"

S3BucketRemoteAddBody::S3BucketRemoteAddBody(std::string &xml,
                                             std::string &request)
    : xml_content(xml), request_id(request), is_valid(false) {

  s3_log(S3_LOG_DEBUG, request_id, "%s Ctor\n", __func__);
  if (!xml.empty()) {
    parse_and_validate();
  }
}

bool S3BucketRemoteAddBody::isOK() { return is_valid; }

bool S3BucketRemoteAddBody::parse_and_validate() {
  // Sample body:
  // <RemoteBucketAdd> ACCESSKEY:SECRETKEY@DESTHOSTNAME/DESTBUCKET
  // </RemoteBucketAdd>

  s3_log(S3_LOG_DEBUG, request_id, "Parsing bucket remote add body\n");
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

  xmlNodePtr remote_bucket_add_node = xmlDocGetRootElement(document);

  s3_log(S3_LOG_DEBUG, request_id, "Root Node = %s",
         remote_bucket_add_node->name);
  // Validate the root node
  if (remote_bucket_add_node == NULL ||
      xmlStrcmp(remote_bucket_add_node->name,
                (const xmlChar *)"RemoteBucketAdd")) {
    s3_log(S3_LOG_WARN, request_id, "XML request body Invalid.\n");
    xmlFreeDoc(document);
    return is_valid;
  }
  xmlChar *val = xmlNodeGetContent(remote_bucket_add_node);
  std::string node_content = reinterpret_cast<char *>(val);
  // Validate RemoteBucketAdd node
  if (validate_node_content((node_content))) {
    is_valid = true;
  }

  return is_valid;
}
std::string S3BucketRemoteAddBody::get_creds() { return creds_str; }

std::string S3BucketRemoteAddBody::get_alias_name() { return alias_name; }

bool S3BucketRemoteAddBody::validate_node_content(std::string node_content) {
  // ACCESSKEY:SECRETKEY@DESTHOSTNAME/DESTBUCKET
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);

  if (node_content.empty()) {
    s3_log(S3_LOG_WARN, request_id, "XML request body Invalid: empty node.\n");
    return false;
  }

  std::size_t delim_pos = node_content.find(":");
  if (delim_pos == std::string::npos) {
    s3_log(S3_LOG_WARN, request_id,
           "XML request body Invalid: Access key or secret key not present.\n");
    return false;
  }

  delim_pos = node_content.find("@");
  if (delim_pos == std::string::npos) {
    s3_log(S3_LOG_WARN, request_id,
           "XML request body Invalid: Hostname or IP address not present.\n");
    return false;
  }
  creds_str = node_content.substr(0, delim_pos);

  s3_log(S3_LOG_WARN, request_id, "creds_str = %s\n", creds_str.c_str());

  alias_name = node_content.substr(delim_pos + 1);
  s3_log(S3_LOG_WARN, request_id, "alias_name = %s\n", alias_name.c_str());

  s3_log(S3_LOG_INFO, request_id, "%s EXIT\n", __func__);
  return true;
}
