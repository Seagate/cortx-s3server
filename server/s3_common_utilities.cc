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