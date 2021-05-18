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

#include "base64.h"
#include "s3_log.h"
#include "s3_m0_uint128_helper.h"

namespace S3M0Uint128Helper {

std::pair<std::string, std::string> to_string_pair(const m0_uint128 &id) {
  return std::make_pair(
      base64_encode((unsigned char const *)&id.u_hi, sizeof(id.u_hi)),
      base64_encode((unsigned char const *)&id.u_lo, sizeof(id.u_lo)));
}

std::string to_string(const m0_uint128 &id) {
  return base64_encode((unsigned char const *)&id.u_hi, sizeof(id.u_hi)) + "-" +
         base64_encode((unsigned char const *)&id.u_lo, sizeof(id.u_lo));
}

m0_uint128 to_m0_uint128(const std::string &id_u_lo,
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

m0_uint128 to_m0_uint128(const std::string &id_str) {
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

void to_string(const struct m0_fid &fid, std::string &s_res) {
  s3_log(S3_LOG_DEBUG, "", "Entering with fid %" SCNx64 " : %" SCNx64 "\n",
         fid.f_container, fid.f_key);
  s_res = base64_encode(reinterpret_cast<unsigned char const *>(&fid),
                        sizeof(struct m0_fid));
}

bool to_m0_fid(const std::string &encoded, struct m0_fid &dst) {
  std::string s_decoded = base64_decode(encoded);
  const bool f_correct = (s_decoded.size() == sizeof(struct m0_fid));

  if (f_correct) {
    memcpy(&dst, s_decoded.c_str(), sizeof(struct m0_fid));
  } else {
    s3_log(S3_LOG_DEBUG, "", "Incorrect encoded m0_fid");
    memset(&dst, 0, sizeof(struct m0_fid));
  }
  return f_correct;
}

}  // namespace S3M0Uint128Helper
