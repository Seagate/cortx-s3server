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

#include "s3_service_list_response.h"
#include "s3_common_utilities.h"

S3ServiceListResponse::S3ServiceListResponse() {
  s3_log(S3_LOG_DEBUG, "", "Constructor\n");
  bucket_list.clear();
}

void S3ServiceListResponse::set_owner_name(std::string name) {
  owner_name = name;
}

void S3ServiceListResponse::set_owner_id(std::string id) { owner_id = id; }

void S3ServiceListResponse::add_bucket(
    std::shared_ptr<S3BucketMetadata> bucket) {
  bucket_list.push_back(bucket);
}

// clang-format off
std::string& S3ServiceListResponse::get_xml() {
  response_xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
  response_xml +=
      "<ListAllMyBucketsResult>"
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01\">";
  response_xml += "<Owner>";
  response_xml += S3CommonUtilities::format_xml_string("ID", owner_id);
  response_xml +=
      S3CommonUtilities::format_xml_string("DisplayName", owner_name);
  response_xml += "</Owner>";
  response_xml += "<Buckets>";
  for (auto&& bucket : bucket_list) {
    response_xml += "<Bucket>";
    response_xml +=
        S3CommonUtilities::format_xml_string("Name", bucket->get_bucket_name());
    response_xml += S3CommonUtilities::format_xml_string(
        "CreationDate", bucket->get_creation_time());
    response_xml += "</Bucket>";
  }
  response_xml += "</Buckets>";
  response_xml += "</ListAllMyBucketsResult>";

  return response_xml;
}
// clang-format on
