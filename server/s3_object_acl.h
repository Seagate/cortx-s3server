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
 * Original author:  Rajesh Nambiar  <rajesh.nambiar@seagate.com>
 * Original author:  Kaustubh Deorukhkar   <kaustubh.deorukhkar@seagate.com>
 * Original creation date: 1-Oct-2015
 */

#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_OBJECT_ACL_H__
#define __MERO_FE_S3_SERVER_S3_OBJECT_ACL_H__

#include <string>
#include <json/json.h>

class S3ObjectACL {
  std::string owner_id;
  std::string owner_name;
  std::string acl_xml_str;
  std::string acl_metadata;
  std::string response_xml;
  std::string display_name;

 public:
  S3ObjectACL();
  void set_owner_id(std::string id);
  void set_owner_name(std::string name);
  void set_display_name(std::string name);
  std::string get_owner_name();
  std::string to_json();

  void from_json(std::string acl_jsoni_str);

  std::string& get_xml_str();
  std::string& get_acl_metadata();
  std::string insert_display_name(std::string acl_str);
  void set_acl_xml_metadata(std::string acl_str);
};

#endif
