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

#ifndef __S3_SERVER_S3_SHA256_H__
#define __S3_SERVER_S3_SHA256_H__

#include <assert.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <string>

class S3sha256 {
 private:
  unsigned char hash[SHA256_DIGEST_LENGTH] = {'\0'};
  char hex_hash[SHA256_DIGEST_LENGTH * 2] = {'\0'};
  SHA256_CTX context;
  int status;

 public:
  S3sha256();
  void reset();
  bool Update(const char *input, size_t length);
  bool Finalize();

  std::string get_hex_hash();
};
#endif
