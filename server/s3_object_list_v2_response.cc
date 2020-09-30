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

#include <evhttp.h>
#include "s3_object_list_v2_response.h"
#include "s3_common_utilities.h"
#include "s3_log.h"

S3ObjectListResponseV2::S3ObjectListResponseV2(const std::string& encoding_type)
    : S3ObjectListResponse(encoding_type),
      continuation_token(""),
      fetch_owner(false),
      start_after(""),
      response_v2_xml("") {
  s3_log(S3_LOG_DEBUG, "", "Constructor\n");
  s3_log(S3_LOG_DEBUG, "", "Encoding type = %s", encoding_type.c_str());
}

void S3ObjectListResponseV2::set_continuation_token(const std::string& token) {
  continuation_token = token;
}

void S3ObjectListResponseV2::set_fetch_owner(bool& need_owner_info) {
  fetch_owner = need_owner_info;
}
void S3ObjectListResponseV2::set_start_after(
    const std::string& in_start_after) {
  start_after = in_start_after;
}

std::string& S3ObjectListResponseV2::get_xml(
    const std::string& requestor_canonical_id,
    const std::string& bucket_owner_user_id,
    const std::string& requestor_user_id) {
  // clang-format off
  response_xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
  response_xml +=
      "<ListBucketResult xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">";
  response_xml += S3CommonUtilities::format_xml_string("Name", bucket_name);
  response_xml +=
      S3CommonUtilities::format_xml_string("Prefix", request_prefix);
  // When 'Delimiter' is specified in the request, the response should have
  // 'Delimiter'
  if (!this->get_request_delimiter().empty()) {
    response_xml +=
        S3CommonUtilities::format_xml_string("Delimiter", request_delimiter);
  }
  if (encoding_type == "url") {
    response_xml += S3CommonUtilities::format_xml_string("EncodingType", "url");
  }
  response_xml += S3CommonUtilities::format_xml_string("KeyCount", key_count);
  // If 'continuation-token' specified in request, include it in the response
  if (!continuation_token.empty()) {
    response_xml += S3CommonUtilities::format_xml_string("ContinuationToken",
                                                         continuation_token);
  }
  response_xml += S3CommonUtilities::format_xml_string("MaxKeys", max_keys);
  // When is_truncated is true, the response should have
  // "NextContinuationToken".
  // Refer AWS S3 ListObjects V2 documentation for NextContinuationToken.
  if (this->response_is_truncated) {
    response_xml += S3CommonUtilities::format_xml_string(
        "NextContinuationToken", next_marker_key);
  }
  // If 'start-after' specified in request, include it in response
  if (!start_after.empty()) {
    response_xml +=
        S3CommonUtilities::format_xml_string("StartAfter", start_after);
  }
  response_xml += S3CommonUtilities::format_xml_string(
      "IsTruncated", (response_is_truncated ? "true" : "false"));

  for (auto&& object : object_list) {
    response_xml += "<Contents>";
    response_xml += S3CommonUtilities::format_xml_string(
        "Key", get_response_format_key_value(object->get_object_name()));
    response_xml += S3CommonUtilities::format_xml_string(
        "LastModified", object->get_last_modified_iso());
    response_xml +=
        S3CommonUtilities::format_xml_string("ETag", object->get_md5(), true);
    response_xml += S3CommonUtilities::format_xml_string(
        "Size", object->get_content_length_str());
    response_xml += S3CommonUtilities::format_xml_string(
        "StorageClass", object->get_storage_class());
    if (this->fetch_owner) {
      response_xml += "<Owner>";
      response_xml += S3CommonUtilities::format_xml_string(
          "ID", object->get_canonical_id());
      response_xml += S3CommonUtilities::format_xml_string(
          "DisplayName", object->get_account_name());
      response_xml += "</Owner>";
    }
    response_xml += "</Contents>";
  }

  for (auto&& prefix : common_prefixes) {
    response_xml += "<CommonPrefixes>";
    std::string prefix_no_delimiter = prefix;
    // Remove the delimiter from the end
    prefix_no_delimiter.pop_back();
    std::string uri_encode_prefix =
        get_response_format_key_value(prefix_no_delimiter);
    // Add the delimiter at the end
    uri_encode_prefix += request_delimiter;
    response_xml +=
        S3CommonUtilities::format_xml_string("Prefix", uri_encode_prefix);
    response_xml += "</CommonPrefixes>";
  }

  response_xml += "</ListBucketResult>";
  // clang-format on
  return response_xml;
}
