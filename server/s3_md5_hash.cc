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
#include "s3_log.h"
#include <assert.h>

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

int MD5hash::s3_motr_client_calculate_pi(m0_generic_pi *pi,
                                         struct m0_pi_seed *seed,
                                         m0_bufvec *pi_bufvec,
                                         enum m0_pi_calc_flag pi_flag,
                                         unsigned char *current_digest,
                                         unsigned char *pi_value_without_seed) {
  int rc = s3_motr_api->motr_client_calculate_pi(
      pi, seed, pi_bufvec, pi_flag, current_digest, pi_value_without_seed);
  return rc;
}

int MD5hash::s3_calculate_unit_pi(struct s3_motr_rw_op_context *rw_ctx,
                                  size_t current_attr_index, size_t seed_offset,
                                  m0_uint128 seed_oid, unsigned int s3_flag) {
  int rc;
  enum m0_pi_calc_flag motr_pi_flag = M0_PI_NO_FLAG;
  struct m0_pi_seed *p_seed = nullptr;
  struct m0_md5_inc_context_pi *s3_pi;
  struct m0_pi_seed seed = {0};
  assert(rw_ctx != nullptr);
  assert(current_attr_index <= rw_ctx->motr_checksums_buf_count);
  s3_pi =
      (struct m0_md5_inc_context_pi *)rw_ctx->attr->ov_buf[current_attr_index];
  if (s3_flag & S3_FIRST_UNIT) {
    motr_pi_flag = M0_PI_CALC_UNIT_ZERO;
  } else {
    if (checksum_saved) {
      // If previous checksum is there then get it
      memcpy(s3_pi->prev_context, &md5ctx, sizeof(MD5_CTX));
    }
  }
  if (s3_flag & S3_SEED_UNIT) {
    seed.data_unit_offset = seed_offset;
    seed.obj_id.f_container = seed_oid.u_hi;
    seed.obj_id.f_key = seed_oid.u_lo;
    p_seed = &seed;
  }

  rw_ctx->pi_bufvec->ov_vec.v_nr = rw_ctx->buffers_per_motr_unit;
  rc = s3_motr_client_calculate_pi(
      (m0_generic_pi *)s3_pi, p_seed, rw_ctx->pi_bufvec, motr_pi_flag,
      (unsigned char *)rw_ctx->current_digest, NULL);
  assert(rc == 0);
  if (s3_flag & S3_FIRST_UNIT) {
    is_initialized = true;
  }
  if (p_seed) {
    memset(&seed, '\0', sizeof(struct m0_pi_seed));
  }
  save_motr_unit_checksum((unsigned char *)rw_ctx->current_digest);
  return rc;
}

int MD5hash::s3_calculate_unaligned_buffs_pi(
    struct s3_motr_rw_op_context *rw_ctx, unsigned int s3_flag) {
  int rc;
  assert(rw_ctx != nullptr);
  enum m0_pi_calc_flag pi_flag = M0_PI_NO_FLAG;
  struct m0_md5_inc_context_pi s3_md5_inc_digest_unaligned_pi = {0};
  s3_md5_inc_digest_unaligned_pi.hdr.pi_type =
      S3Option::get_instance()->get_pi_type();
  if (s3_flag & S3_FIRST_UNIT) {
    pi_flag = M0_PI_CALC_UNIT_ZERO;
  } else {
    memcpy((void *)s3_md5_inc_digest_unaligned_pi.prev_context,
           (void *)&md5ctx_pre_unaligned, sizeof(MD5_CTX));
  }

  // Update (with actual data length) + Finalize without seed
  rc = s3_motr_client_calculate_pi(
      (m0_generic_pi *)&s3_md5_inc_digest_unaligned_pi, NULL, rw_ctx->pi_bufvec,
      pi_flag, (unsigned char *)rw_ctx->current_digest, md5_digest);
  assert(rc == 0);
  if (s3_flag & S3_FIRST_UNIT) {
    is_initialized = true;
  }
  // We have even Finalized (without seed)
  is_finalized = true;
  return rc;
}

int MD5hash::Finalize() {
  if (is_finalized) {
    return 0;
  }
  if (!native_call) {
    if (!is_initialized) {
      status = s3_motr_api->motr_client_calculate_pi(
          (struct m0_generic_pi *)&s3_md5_inc_digest_pi, NULL, &pi_bufvec,
          M0_PI_CALC_UNIT_ZERO, (unsigned char *)&md5ctx, NULL);
      if (status < 0) {
        return -1;  // failure
      }
    }
    memcpy(s3_md5_inc_digest_pi.prev_context, &md5ctx, sizeof(MD5_CTX));
    status = s3_motr_api->motr_client_calculate_pi(
        (struct m0_generic_pi *)&s3_md5_inc_digest_pi, NULL, &pi_bufvec, flag,
        (unsigned char *)&md5ctx, md5_digest);
    if (status < 0) {
      return -1;  // failure
    }
  } else {
    if (!is_initialized) {
      status = MD5_Init(&md5ctx);
    }
    if (status > 0) {
      status = MD5_Final(md5_digest, &md5ctx);
      if (status < 1) {
        return -1;  // failure
      }
    } else {
      return -1;
    }
  }
  is_finalized = true;
  return 0;
}

void MD5hash::Finalized() { is_finalized = true; }

void MD5hash::Initialized() { is_initialized = true; }

void MD5hash::Reset(bool call_init) {
  if (call_init) {
    status = MD5_Init(&md5ctx);
    native_call = true;
    is_initialized = true;
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
  seed = {0};
  s3_md5_inc_digest_pi.hdr.pi_type = M0_PI_TYPE_MD5_INC_CONTEXT;
  pi_bufvec.ov_vec.v_nr = 0;
  flag = (m0_pi_calc_flag)M0_PI_SKIP_CALC_FINAL;
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
