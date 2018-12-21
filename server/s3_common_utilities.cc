/*
 * COPYRIGHT 2018 SEAGATE LLC
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
 * Original author:  Prashanth Vanaparthy   <prashanth.vanaparthy@seagate.com>
 * Original creation date: 16-August-2018
 */

#include "s3_common_utilities.h"
#include <algorithm>
#include <cctype>
#include <libxml/parser.h>

bool S3CommonUtilities::string_has_only_digits(const std::string &str) {
  return str.find_first_not_of("0123456789") == std::string::npos;
}

std::string &S3CommonUtilities::ltrim(std::string &str) {
  str.erase(str.begin(), find_if_not(str.begin(), str.end(),
                                     [](int c) { return std::isspace(c); }));
  return str;
}

std::string &S3CommonUtilities::rtrim(std::string &str) {
  str.erase(find_if_not(str.rbegin(), str.rend(),
                        [](int c) { return std::isspace(c); }).base(),
            str.end());
  return str;
}

std::string S3CommonUtilities::trim(const std::string &str) {
  std::string tempstr = str;
  return ltrim(rtrim(tempstr));
}

std::string S3CommonUtilities::s3xmlEncodeSpecialChars(
    const std::string &input) {
  xmlChar *output = xmlEncodeSpecialChars(NULL, BAD_CAST input.c_str());
  std::string data;
  if (output) {
    data = reinterpret_cast<char *>(output);
    xmlFree(output);
  }
  return data;
}

std::string S3CommonUtilities::format_xml_string(std::string tag,
                                                 const std::string &value) {
  std::string format_string = s3xmlEncodeSpecialChars(value);
  if (format_string.empty()) {
    return "<" + tag + "/>";
  }
  return "<" + tag + ">" + format_string + "</" + tag + ">";
}

bool S3CommonUtilities::stoul(const std::string &str, unsigned long &value) {
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

void S3CommonUtilities::find_and_replaceall(std::string &data,
                                            const std::string &to_search,
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