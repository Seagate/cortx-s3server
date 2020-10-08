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

#include <cctype>
#include <sstream>
#include <algorithm>
#include <libxml/parser.h>
#include <evhtp.h>

#include "s3_common_utilities.h"
#include "s3_log.h"
#include "s3_option.h"

namespace S3CommonUtilities {

S3XORObfuscator::S3XORObfuscator() : S3Obfuscator() {}

// Implement simple XOR obfuscator by xoring each byte of the string with
// constant = strlen + 255
std::string S3XORObfuscator::obfuscate(const std::string &input) {
  std::string output = "";
  if (!input.empty()) {
    uint8_t xorbyte = (uint8_t)(input.length() + 255);
    for (const auto &c : input) {
      output.push_back(c ^ xorbyte);
    }
  }
  return output;
}

std::string S3XORObfuscator::encode(const std::string &input) {
  return obfuscate(input);
}

std::string S3XORObfuscator::decode(const std::string &input) {
  return obfuscate(input);
}

bool string_has_only_digits(const std::string &str) {
  return str.find_first_not_of("0123456789") == std::string::npos;
}

std::string &ltrim(std::string &str) {
  str.erase(str.begin(), find_if_not(str.begin(), str.end(),
                                     [](int c) { return std::isspace(c); }));
  return str;
}

std::string &rtrim(std::string &str) {
  str.erase(find_if_not(str.rbegin(), str.rend(),
                        [](int c) { return std::isspace(c); }).base(),
            str.end());
  return str;
}

std::string trim(const std::string &str) {
  std::string tempstr = str;
  return ltrim(rtrim(tempstr));
}

std::string s3xmlEncodeSpecialChars(const std::string &input) {
  xmlChar *output = xmlEncodeSpecialChars(NULL, BAD_CAST input.c_str());
  std::string data;
  if (output) {
    data = reinterpret_cast<char *>(output);
    xmlFree(output);
  }
  return data;
}

std::string format_xml_string(std::string tag, const std::string &value,
                              bool append_quotes) {

  std::string format_string = s3xmlEncodeSpecialChars(value);
  if (format_string.empty()) {
    return "<" + tag + "/>";
  }
  if (append_quotes) {
    format_string = "\"" + format_string + "\"";
  }
  return "<" + tag + ">" + format_string + "</" + tag + ">";
}

bool stoul(const std::string &str, unsigned long &value) {
  bool isvalid = true;
  try {
    value = std::stoul(str);
  }
  catch (const std::invalid_argument &ia) {
    isvalid = false;
  }
  catch (const std::out_of_range &oor) {
    isvalid = false;
  }
  return isvalid;
}

bool stoi(const std::string &str, int &value) {
  bool isvalid = true;
  try {
    value = std::stoi(str);
  }
  catch (const std::invalid_argument &ia) {
    isvalid = false;
  }
  catch (const std::out_of_range &oor) {
    isvalid = false;
  }
  return isvalid;
}

void find_and_replaceall(std::string &data, const std::string &to_search,
                         const std::string &replace_str) {
  // return, if search string is empty
  if (to_search.empty()) return;
  // nothing to match
  if (data.empty()) return;
  // Get the first occurrence
  size_t pos = data.find(to_search);

  // Repeat till end is reached
  while (pos != std::string::npos) {
    // Replace this occurrence of Sub String
    data.replace(pos, to_search.size(), replace_str);
    // Get the next occurrence from the current position
    pos = data.find(to_search, pos + replace_str.size());
  }
}

bool is_yaml_value_null(const std::string &value) {
  // 'null | Null | NULL | ~ ' represents empty string in value
  // https://yaml.org/spec/1.2/spec.html#id2805071
  return ((value == "null") || (value == "Null") || (value == "NULL") ||
          (value == "~"));
}

std::string evhtp_error_flags_description(uint8_t errtype) {
  std::ostringstream oss;

  if (errtype & BEV_EVENT_READING) {
    oss << "Reading ";
  }
  if (errtype & BEV_EVENT_WRITING) {
    oss << "Writing ";
  }
  if (errtype & BEV_EVENT_EOF) {
    oss << "EOF ";
  }
  if (errtype & BEV_EVENT_ERROR) {
    oss << "Error ";
  }
  if (errtype & BEV_EVENT_TIMEOUT) {
    oss << "Timeout ";
  }
  if (errtype & BEV_EVENT_CONNECTED) {
    oss << "Connected ";
  }
  std::string errtype_str = oss.str();
  oss.clear();

  if (!errtype_str.empty()) {
    errtype_str.resize(errtype_str.size() - 1);
  }
  return errtype_str;
}

}  // namespace S3CommonUtilities

void s3_kickoff_graceful_shutdown(int ignore) {
  extern int global_shutdown_in_progress;
  extern evbase_t *global_evbase_handle;
  if (!global_shutdown_in_progress) {
    global_shutdown_in_progress = 1;
    // signal handler
    S3Option *option_instance = S3Option::get_instance();

    // trigger rollbacks & stop handling new requests
    option_instance->set_is_s3_shutting_down(true);
    s3_log(S3_LOG_INFO, "", "Calling event_base_loopexit\n");
    event_base_loopexit(global_evbase_handle, NULL);
  }
  return;
}
