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

#ifndef __S3_M0_UINT128_HELPER_h__
#define __S3_M0_UINT128_HELPER_h__

#include <cstdint>
#include <utility>
#include <string>

#include "motr_helpers.h"

struct m0_fid;

namespace S3M0Uint128Helper {

std::pair<std::string, std::string> to_string_pair(const m0_uint128 &id);
std::string to_string(const m0_uint128 &id);
m0_uint128 to_m0_uint128(const std::string &id_u_lo,
                         const std::string &id_u_hi);
m0_uint128 to_m0_uint128(const std::string &id_str);

void to_string(const struct m0_fid &fid, std::string &s_res);
bool to_m0_fid(const std::string &encoded, struct m0_fid &dst);

template <size_t IntSize>
int non_zero_tmpl_hlpr(const m0_uint128 &id);

template <>
inline int non_zero_tmpl_hlpr<size_t(4u)>(const m0_uint128 &id) {
  const union {
    uint64_t whole;
    struct {
      uint32_t lo;
      uint32_t hi;
    } parts;
  } tmp = {id.u_hi | id.u_lo};

  return tmp.parts.lo | tmp.parts.hi;
}

}  // namespace S3M0Uint128Helper

inline int non_zero(const m0_uint128 &id) {
  return S3M0Uint128Helper::non_zero_tmpl_hlpr<sizeof(int)>(id);
}

inline int zero(const m0_uint128 &id) { return !non_zero(id); }

#endif  // __S3_M0_UINT128_HELPER_h__
