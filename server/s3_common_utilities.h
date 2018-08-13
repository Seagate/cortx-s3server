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
#pragma once

#ifndef __S3_COMMON_UTILITIES_h__
#define __S3_COMMON_UTILITIES_h__

#include <string>

class S3CommonUtilities {
 public:
  S3CommonUtilities() = delete;
  S3CommonUtilities(const S3CommonUtilities &) = delete;
  S3CommonUtilities &operator=(const S3CommonUtilities &) = delete;

  // return true, if passed string has only digits
  static bool string_has_only_digits(const std::string &str);
  // trims below leading charactors of given string
  // space (0x20, ' ')
  // form feed (0x0c, '\f')
  // line feed (0x0a, '\n')
  // carriage return (0x0d, '\r')
  // horizontal tab (0x09, '\t')
  // vertical tab (0x0b, '\v')
  static std::string &ltrim(std::string &str);
  // trims below trailing charactors of given string
  // space (0x20, ' ')
  // form feed (0x0c, '\f')
  // line feed (0x0a, '\n')
  // carriage return (0x0d, '\r')
  // horizontal tab (0x09, '\t')
  // vertical tab (0x0b, '\v')
  static std::string &rtrim(std::string &str);
  // trims below leading and trialing charactors of given string
  // space (0x20, ' ')
  // form feed (0x0c, '\f')
  // line feed (0x0a, '\n')
  // carriage return (0x0d, '\r')
  // horizontal tab (0x09, '\t')
  // vertical tab (0x0b, '\v')
  static std::string trim(const std::string &str);
};
#endif