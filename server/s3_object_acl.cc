/*
 * COPYRIGHT 2015 SEAGATE LLC
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
 * Original author:  Rajesh Nambiar  <rajesh.nambiar"seagate.com>
 * Original author:  Kaustubh Deorukhkar   <kaustubh.deorukhkar@seagate.com>
 * Original creation date: 20-May-2016
 */

#include "s3_object_acl.h"
#include "s3_log.h"
#include <json/json.h>
#include "base64.h"
#include <assert.h>
#include <string.h>

S3ObjectACL::S3ObjectACL(): acl_xml_str(""), acl_metadata("") {}

void S3ObjectACL::set_owner_id(std::string id) {
  owner_id = id;
}

void S3ObjectACL::set_owner_name(std::string name) {
  owner_name = name;
}

std::string S3ObjectACL::get_owner_name() {
  return owner_name;
}

void S3ObjectACL::from_json(std::string acl_json_str) {
  s3_log(S3_LOG_DEBUG, "Called\n");
  acl_metadata = acl_json_str;
  acl_xml_str = base64_decode(acl_metadata);
}

std::string& S3ObjectACL::get_xml_str() {
  return acl_xml_str;
}

std::string& S3ObjectACL::get_acl_metadata() {
  return acl_metadata;
}

std::string S3ObjectACL::insert_display_name(std::string acl_str) {
  char *acl = (char *)acl_str.c_str();
  char *partial_acl_str = (char *)malloc(acl_str.length());
  char *partial_acl;
  std::string final_acl;
  while((partial_acl = strstr(acl, "</ID>"))) {
    memset(partial_acl_str, '\0', acl_str.length());
    // Get DisplayName by querying the auth server
    if (partial_acl[5] == '\n') {
      partial_acl = partial_acl + 5;
      strncpy(partial_acl_str, acl, partial_acl - acl + 1);
      final_acl += partial_acl_str;
      final_acl += "\n<DisplayName>s3_test</DisplayName>\n";
    } else {
      partial_acl = partial_acl + 4;
      strncpy(partial_acl_str, acl, partial_acl - acl + 1);
      final_acl += partial_acl_str;
      final_acl += "<DisplayName>s3_test</DisplayName>";
    }
    acl = partial_acl;
  }
  final_acl += acl;
  free(partial_acl_str);
  return final_acl;
}

void S3ObjectACL::set_acl_xml_metadata(std::string acl_str) {
  acl_xml_str = acl_str;
  return;
}
