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
#include "motr/client.h"
#include "s3_motr_wrapper.h"
#include "s3_motr_context.h"

class MD5hash {
  MD5_CTX md5ctx = {0};
  MD5_CTX md5ctx_pre_unaligned = {0};
  unsigned char md5_digest[MD5_DIGEST_LENGTH];
  struct m0_pi_seed seed;
  int status;
  bool is_finalized = false;
  bool checksum_saved = false;
  bool is_initialized = false;
  bool native_call = false;
  struct m0_md5_inc_context_pi s3_md5_inc_digest_pi;
  std::shared_ptr<MotrAPI> s3_motr_api = nullptr;
  struct m0_bufvec pi_bufvec;
  struct m0_bufvec pi_update_bufvec;
  m0_pi_calc_flag flag;

 public:
  MD5hash(std::shared_ptr<MotrAPI> motr_api = nullptr, bool call_init = false);
  ~MD5hash();
  int Update(const char *input, size_t length);
  void save_motr_unit_checksum(unsigned char *curr_digest);
  void save_motr_unit_checksum_for_unaligned_bufs(
      unsigned char *curr_digest_at_unit);
  int s3_calculate_unit_pi(struct s3_motr_rw_op_context *rw_ctx,
                           size_t current_attr_index, size_t seed_offset,
                           m0_uint128 oid, unsigned int s3_flag);
  int s3_motr_client_calculate_pi(struct m0_generic_pi *, struct m0_pi_seed *,
                                  struct m0_bufvec *, enum m0_pi_calc_flag,
                                  unsigned char *current_digest,
                                  unsigned char *without_seed);
  int s3_calculate_unaligned_buffs_pi(struct s3_motr_rw_op_context *rw_ctx,
                                      unsigned int s3_flag);
  bool is_checksum_saved();
  void *get_prev_write_checksum();
  void *get_prev_unaligned_checksum();
  int Finalize();
  void Finalized();
  void Initialized();
  void Reset(std::shared_ptr<MotrAPI> motr_api, bool call_init);
  unsigned char *get_md5_digest() { return md5_digest; }

  std::string get_md5_string();
  std::string get_md5_base64enc_string();

  FRIEND_TEST(MD5HashTest, Constructor);
  FRIEND_TEST(MD5HashTest, FinalBasic);
  FRIEND_TEST(MD5HashTest, FinalEmptyStr);
  FRIEND_TEST(MD5HashTest, FinalNumeral);
};
#endif
