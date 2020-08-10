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

#include "s3_m0_uint128_helper.h"
#include "base64.h"

std::pair<std::string, std::string> S3M0Uint128Helper::to_string_pair(
    const m0_uint128 &id) {
  return std::make_pair(
      base64_encode((unsigned char const *)&id.u_hi, sizeof(id.u_hi)),
      base64_encode((unsigned char const *)&id.u_lo, sizeof(id.u_lo)));
}

std::string S3M0Uint128Helper::to_string(const m0_uint128 &id) {
  return base64_encode((unsigned char const *)&id.u_hi, sizeof(id.u_hi)) + "-" +
         base64_encode((unsigned char const *)&id.u_lo, sizeof(id.u_lo));
}

m0_uint128 S3M0Uint128Helper::to_m0_uint128(const std::string &id_u_lo,
                                            const std::string &id_u_hi) {
  m0_uint128 id = {0ULL, 0ULL};
  std::string dec_id_u_hi = base64_decode(id_u_hi);
  std::string dec_id_u_lo = base64_decode(id_u_lo);
  if ((dec_id_u_hi.size() == sizeof(id.u_hi)) &&
      (dec_id_u_lo.size() == sizeof(id.u_lo))) {
    memcpy((void *)&id.u_hi, dec_id_u_hi.c_str(), sizeof(id.u_hi));
    memcpy((void *)&id.u_lo, dec_id_u_lo.c_str(), sizeof(id.u_lo));
  }
  return id;
}

m0_uint128 S3M0Uint128Helper::to_m0_uint128(const std::string &id_str) {
  m0_uint128 id = {0ULL, 0ULL};
  std::size_t delim_pos = id_str.find("-");
  if (delim_pos != std::string::npos) {
    std::string dec_id_u_hi = base64_decode(id_str.substr(0, delim_pos));
    std::string dec_id_u_lo = base64_decode(id_str.substr(delim_pos + 1));
    if ((dec_id_u_hi.size() == sizeof(id.u_hi)) &&
        (dec_id_u_lo.size() == sizeof(id.u_lo))) {
      memcpy((void *)&id.u_hi, dec_id_u_hi.c_str(), sizeof(id.u_hi));
      memcpy((void *)&id.u_lo, dec_id_u_lo.c_str(), sizeof(id.u_lo));
    }
  }
  return id;
}