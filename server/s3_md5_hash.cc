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

#include "s3_md5_hash.h"

MD5hash::MD5hash() { status = MD5_Init(&md5ctx); }

int MD5hash::Update(const char *input, size_t length) {
  if (input == NULL) {
    return -1;
  }
  if (status > 0) {
    status = MD5_Update(&md5ctx, input, length);
  }
  if (status < 1) {
    return -1;  // failure
  }
  return 0;  // success
}

int MD5hash::Finalize() {
  if (is_finalized) {
    return 0;
  }
  if (status > 0) {
    status = MD5_Final(md5_digest, &md5ctx);
  }
  if (status < 1) {
    return -1;  // failure
  }
  is_finalized = true;
  return 0;
}

const char hex_tbl[] = "0123456789abcdef";

std::string MD5hash::get_md5_string() {
  if (Finalize() < 0) {
    return std::string();  // failure
  }
  std::string s_hex;
  s_hex.reserve(MD5_DIGEST_LENGTH * 2);

  for (int i = 0; i < MD5_DIGEST_LENGTH; ++i) {
    const unsigned ch = md5_digest[i] & 255;

    s_hex += hex_tbl[ch >> 4];
    s_hex += hex_tbl[ch & 15];
  }
  return s_hex;
}

std::string MD5hash::get_md5_base64enc_string() {
  if (Finalize() < 0) {
    return std::string();  // failure
  }
  return base64_encode(md5_digest, MD5_DIGEST_LENGTH);
}
