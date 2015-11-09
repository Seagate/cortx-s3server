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
 * Original author:  Kaustubh Deorukhkar   <kaustubh.deorukhkar@seagate.com>
 * Original creation date: 1-Oct-2015
 */

#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_OBJECT_LIST_RESPONSE_H__
#define __MERO_FE_S3_SERVER_S3_OBJECT_LIST_RESPONSE_H__

#include <string>
#include <memory>
#include <vector>
#include <unordered_set>

#include "s3_object_metadata.h"

class S3ObjectListResponse {
  std::string bucket_name;
  std::vector<std::shared_ptr<S3ObjectMetadata>> object_list;

  // We use unordered for performance as the keys are already
  // in sorted order as stored in clovis-kv (cassandra)
  std::unordered_set<std::string> common_prefixes;

  // Generated xml response
  std::string request_prefix;
  std::string request_delimiter;
  std::string request_marker_key;
  std::string max_keys;
  bool response_is_truncated;
  std::string next_marker_key;

  std::string response_xml;

public:
  S3ObjectListResponse();

  void set_bucket_name(std::string name);
  void set_request_prefix(std::string prefix);
  void set_request_delimiter(std::string delimiter);
  void set_request_marker_key(std::string marker);
  void set_max_keys(std::string count);
  void set_response_is_truncated(bool flag);
  void set_next_marker_key(std::string next);

  void add_object(std::shared_ptr<S3ObjectMetadata> object);
  void add_common_prefix(std::string);

  std::string& get_xml();

};

#endif
