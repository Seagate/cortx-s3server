#pragma once

#ifndef __S3_COMMON_UTILITIES_h__
#define __S3_COMMON_UTILITIES_h__

#include <cstdint>
#include <string>

namespace S3CommonUtilities {

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

#endif
