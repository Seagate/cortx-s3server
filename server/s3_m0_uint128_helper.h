#pragma once

#ifndef __S3_M0_UINT128_HELPER_h__
#define __S3_M0_UINT128_HELPER_h__

#include <string>
#include "clovis_helpers.h"

class S3M0Uint128Helper {
 public:
  S3M0Uint128Helper() = delete;

  static std::pair<std::string, std::string> to_string_pair(
      const m0_uint128 &id);
  static std::string to_string(const m0_uint128 &id);
  static m0_uint128 to_m0_uint128(const std::string &id_u_lo,
                                  const std::string &id_u_hi);
  static m0_uint128 to_m0_uint128(const std::string &id_str);
};
#endif
