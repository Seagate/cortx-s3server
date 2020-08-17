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

#include <string>
#include "motr_helpers.h"

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
