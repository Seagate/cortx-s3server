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

#include "s3_sha256.h"

S3sha256::S3sha256() { reset(); }

void S3sha256::reset() { status = SHA256_Init(&context); }

bool S3sha256::Update(const char *input, size_t length) {
  if (input == NULL) {
    return false;
  }
  if (status == 0) {
    return false;  // failure
  }
  status = SHA256_Update(&context, input, length);
  if (status == 0) {
    return false;  // failure
  }
  return true;  // success
}

bool S3sha256::Finalize() {
  status = SHA256_Final(hash, &context);
  if (status == 0) {
    return false;  // failure
  }

  for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
    sprintf((char *)(&hex_hash[i * 2]), "%02x", (int)hash[i]);
  }
  return true;
}

std::string S3sha256::get_hex_hash() {
  if (status == 0) {
    return std::string("");  // failure
  }
  return std::string(hex_hash);
}
