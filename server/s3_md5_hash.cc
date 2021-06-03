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
#include "s3_option.h"

MD5hash::MD5hash(bool call_init) { Reset(call_init); }
MD5hash::MD5hash(std::shared_ptr<MotrAPI> motr_api) { Reset(motr_api); }

void MD5hash::save_motr_unit_checksum(unsigned char *current_digest) {
  memcpy((void *)&md5ctx, (void *)current_digest, sizeof(MD5_CTX));
  checksum_saved = true;
}

void MD5hash::save_motr_unit_checksum_for_unaligned_bufs(unsigned char *current_digest) {
  memcpy((void *)&md5ctx_pre_unaligned, (void *)current_digest, sizeof(MD5_CTX));
}

void *MD5hash::get_prev_unaligned_checksum() { return (void *)&md5ctx_pre_unaligned; }

void *MD5hash::get_prev_write_checksum() { return (void *)&md5ctx; }

bool MD5hash::is_checksum_saved() { return checksum_saved; }

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
  memcpy(s3_md5_inc_digest_pi.prev_context, &md5ctx, sizeof(MD5_CTX));
  status = s3_motr_api->motr_client_calculate_pi(
      (struct m0_generic_pi *)&s3_md5_inc_digest_pi, NULL, &pi_bufvec, flag,
      (unsigned char *)&md5ctx, md5_digest);
  if (status < 0) {
    return -1;  // failure
  }
  is_finalized = true;
  return 0;
}

void MD5hash::Finalized() { is_finalized = true; }

void MD5hash::Reset(bool call_init) {
  if (call_init) {
    status = MD5_Init(&md5ctx);
  }
  is_finalized = false;
}

void MD5hash::Reset(std::shared_ptr<MotrAPI> motr_api) {
  if (motr_api) {
    s3_motr_api = std::move(motr_api);
  } else {
    s3_motr_api = std::make_shared<ConcreteMotrAPI>();
  }
  s3_md5_inc_digest_pi = {0};
  s3_md5_inc_digest_pi.hdr.pi_type = M0_PI_TYPE_MD5_INC_CONTEXT;
  pi_bufvec.ov_vec.v_nr = 0;
  flag = (m0_pi_calc_flag)0;
  is_finalized = false;
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
