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

#include "s3_error_codes.h"
#include "s3_common_utilities.h"

S3Error::S3Error(std::string error_code, std::string req_id,
                 std::string res_key, std::string error_message)
    : code(error_code),
      request_id(req_id),
      resource_key(res_key),
      auth_error_message(error_message),
      details(S3ErrorMessages::get_instance()->get_details(error_code)) {}

int S3Error::get_http_status_code() { return details.get_http_status_code(); }

std::string& S3Error::to_xml() {
  if (get_http_status_code() == -1) {
    // Object state is invalid, Wrong error code.
    xml_message = "";
    return xml_message;
  }
  // clang-format off
  xml_message = "";
  xml_message = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  xml_message += "<Error>\n";
  xml_message += S3CommonUtilities::format_xml_string("Code", code);
  if (auth_error_message.empty()) {
  xml_message +=
      S3CommonUtilities::format_xml_string("Message", details.get_message());
  } else {
    xml_message +=
        S3CommonUtilities::format_xml_string("Message", auth_error_message);
  }
  xml_message += S3CommonUtilities::format_xml_string("Resource", resource_key);
  xml_message += S3CommonUtilities::format_xml_string("RequestId", request_id);
  xml_message += "</Error>\n";
  // clang-format on
  return xml_message;
}
