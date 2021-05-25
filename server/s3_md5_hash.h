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

#ifndef __S3_SERVER_S3_MD5_HASH_H__
#define __S3_SERVER_S3_MD5_HASH_H__

#include <string>
#include <gtest/gtest_prod.h>
#include <openssl/md5.h>

class MD5hash {
  MD5_CTX md5ctx, md5ctx_unit_size;
  unsigned char md5_digest[MD5_DIGEST_LENGTH];
  int status;
  bool is_finalized = false;

 public:
  MD5hash();
  int Update(const char *input, size_t length);
  void save_motr_unit_checksum(unsigned char *curr_digest);
  void save_unaligned_running_checksum(unsigned char *curr_digest);
  void *get_prev_unit_checksum();
  int Finalize();
  void Reset();

  std::string get_md5_string();
  std::string get_md5_base64enc_string();

  FRIEND_TEST(MD5HashTest, Constructor);
  FRIEND_TEST(MD5HashTest, FinalBasic);
  FRIEND_TEST(MD5HashTest, FinalEmptyStr);
  FRIEND_TEST(MD5HashTest, FinalNumeral);
};
#endif
