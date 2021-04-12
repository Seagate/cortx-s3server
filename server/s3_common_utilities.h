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

#pragma once

#ifndef __S3_COMMON_UTILITIES_h__
#define __S3_COMMON_UTILITIES_h__

#include <cstdint>
#include <string>

namespace S3CommonUtilities {

class S3Obfuscator {
 public:
  std::string virtual encode(const std::string &input) = 0;
  std::string virtual decode(const std::string &input) = 0;
};

class S3XORObfuscator : public S3Obfuscator {
 private:
  std::string obfuscate(const std::string &input);

 public:
  S3XORObfuscator();
  std::string virtual encode(const std::string &input);
  std::string virtual decode(const std::string &input);
};

// return true, if passed string has only digits
bool string_has_only_digits(const std::string &str);
// trims below leading charactors of given string
// space (0x20, ' ')
// form feed (0x0c, '\f')
// line feed (0x0a, '\n')
// carriage return (0x0d, '\r')
// horizontal tab (0x09, '\t')
// vertical tab (0x0b, '\v')
std::string &ltrim(std::string &str);
// trims below trailing charactors of given string
// space (0x20, ' ')
// form feed (0x0c, '\f')
// line feed (0x0a, '\n')
// carriage return (0x0d, '\r')
// horizontal tab (0x09, '\t')
// vertical tab (0x0b, '\v')
std::string &rtrim(std::string &str);
// trims below leading and trialing charactors of given string
// space (0x20, ' ')
// form feed (0x0c, '\f')
// line feed (0x0a, '\n')
// carriage return (0x0d, '\r')
// horizontal tab (0x09, '\t')
// vertical tab (0x0b, '\v')
std::string trim(const std::string &str);
// wrapper method for std::stoul
// return true, when string is valid to convert
// else return false
bool stoul(const std::string &str, unsigned long &value);
// wrapper method for std::stoi
// return true, when string is valid to convert
// else return false
bool stoi(const std::string &str, int &value);
// input: A string to convert to XML.
// returns new string with the substitution done
std::string s3xmlEncodeSpecialChars(const std::string &input);

std::string format_xml_string(std::string tag, const std::string &value,
                              bool append_quotes = false);
void find_and_replaceall(std::string &data, const std::string &to_search,
                         const std::string &replace_str);
bool is_yaml_value_null(const std::string &value);

std::string evhtp_error_flags_description(uint8_t errtype);

}  // namespace S3CommonUtilities

// Common graceful shutdown
void s3_kickoff_graceful_shutdown(int ignore);

#endif
