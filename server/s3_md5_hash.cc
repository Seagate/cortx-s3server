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
#include "motr_helpers.h"
#include "s3_option.h"
#include "s3_log.h"
#include <assert.h>
#include "motr/client.h"

MD5hash::MD5hash(std::shared_ptr<MotrAPI> motr_api, bool call_init) {
  Reset(motr_api, call_init);
}

void MD5hash::save_motr_unit_checksum(unsigned char *current_digest) {
  memcpy((void *)&md5ctx, (void *)current_digest, sizeof(MD5_CTX));
  checksum_saved = true;
}

void MD5hash::save_motr_unit_checksum_for_unaligned_bufs(
    unsigned char *current_digest) {
  memcpy((void *)&md5ctx_pre_unaligned, (void *)current_digest,
         sizeof(MD5_CTX));
}

void *MD5hash::get_prev_unaligned_checksum() {
  return (void *)&md5ctx_pre_unaligned;
}

void *MD5hash::get_prev_write_checksum() { return (void *)&md5ctx; }

bool MD5hash::is_checksum_saved() { return checksum_saved; }

int MD5hash::Update(const char *input, size_t length) {
  if (input == NULL || status < 0) {
    return -1;
  }
  pi_update_bufvec.ov_buf[0] = (void *)input;
  pi_update_bufvec.ov_vec.v_count[0] = length;
  pi_update_bufvec.ov_vec.v_nr = 1;
  struct m0_md5_inc_context_pi s3_update_pi;
  s3_update_pi.pimd5c_hdr.pih_type = M0_PI_TYPE_MD5_INC_CONTEXT;
  memcpy(s3_update_pi.pimd5c_prev_context, &md5ctx, sizeof(MD5_CTX));
  status = s3_motr_api->motr_client_calculate_pi(
      (struct m0_generic_pi *)&s3_update_pi, NULL, &pi_update_bufvec,
      M0_PI_SKIP_CALC_FINAL, (unsigned char *)&md5ctx, NULL);
  if (status < 0) {
    return -1;  // failure
  }
  s3_log(S3_LOG_DEBUG, "", "%s Displaying PI Info next", __func__);
  MD5hash::log_pi_info((struct m0_generic_pi *)&s3_update_pi);
  return 0;  // success
}

int MD5hash::s3_motr_client_calculate_pi(m0_generic_pi *pi,
                                         struct m0_pi_seed *seed,
                                         m0_bufvec *s3pi_bufvec,
                                         enum m0_pi_calc_flag pi_flag,
                                         unsigned char *current_digest,
                                         unsigned char *pi_value_without_seed) {
  int rc = s3_motr_api->motr_client_calculate_pi(
      pi, seed, s3pi_bufvec, pi_flag, current_digest, pi_value_without_seed);
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
    s3_log(S3_LOG_DEBUG, "", "%s First Block Calc", __func__);
  } else {
    if (checksum_saved) {
      // If previous checksum is there then get it
      s3_log(S3_LOG_DEBUG, "", "%s Checksum is saved", __func__);
      memcpy(s3_pi->pimd5c_prev_context, &md5ctx, sizeof(MD5_CTX));
    } else {
      s3_log(S3_LOG_ERROR, "", "%s Checksum is not saved and not first block",
             __func__);
    }
  }
  if (s3_flag & S3_SEED_UNIT) {
    seed.pis_data_unit_offset = seed_offset;
    seed.pis_obj_id.f_container = seed_oid.u_hi;
    seed.pis_obj_id.f_key = seed_oid.u_lo;
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
  s3_log(S3_LOG_DEBUG, "", "%s Displaying PI Info next", __func__);
  MD5hash::log_pi_info((struct m0_generic_pi *)s3_pi);
  return rc;
}

int MD5hash::s3_calculate_unaligned_buffs_pi(
    struct s3_motr_rw_op_context *rw_ctx, unsigned int s3_flag) {
  int rc;
  assert(rw_ctx != nullptr);
  enum m0_pi_calc_flag pi_flag = M0_PI_NO_FLAG;
  struct m0_md5_inc_context_pi s3_md5_inc_digest_unaligned_pi = {0};
  s3_md5_inc_digest_unaligned_pi.pimd5c_hdr.pih_type =
      S3Option::get_instance()->get_pi_type();
  if (s3_flag & S3_FIRST_UNIT) {
    pi_flag = M0_PI_CALC_UNIT_ZERO;
  } else {
    memcpy((void *)s3_md5_inc_digest_unaligned_pi.pimd5c_prev_context,
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

  s3_log(S3_LOG_DEBUG, "", "%s Displaying PI Info next", __func__);
  MD5hash::log_pi_info((struct m0_generic_pi *)&s3_md5_inc_digest_unaligned_pi);

  // We have even Finalized (without seed)
  is_finalized = true;
  return rc;
}

int MD5hash::Finalize() {
  if (is_finalized) {
    return 0;
  }
  pi_bufvec.ov_vec.v_nr = 0;
  // Function gets called in send_response_to_s3_client
  // at this point i feel this is moot, since we have
  // already written data to motr.
  if (!is_initialized) {
    status = s3_motr_api->motr_client_calculate_pi(
        (struct m0_generic_pi *)&s3_md5_inc_digest_pi, NULL, &pi_bufvec,
        M0_PI_CALC_UNIT_ZERO, (unsigned char *)&md5ctx, NULL);
    if (status < 0) {
      return -1;  // failure
    }
    s3_log(S3_LOG_DEBUG, "", "%s Displaying PI Info next", __func__);
    MD5hash::log_pi_info((struct m0_generic_pi *)&s3_md5_inc_digest_pi);
  }
  memcpy(s3_md5_inc_digest_pi.pimd5c_prev_context, &md5ctx, sizeof(MD5_CTX));
  // Pass M0_PI_SKIP_CALC_FINAL flag, finalized gets called and stored in
  // md5_digest (without seed)
  status = s3_motr_api->motr_client_calculate_pi(
      (struct m0_generic_pi *)&s3_md5_inc_digest_pi, NULL, &pi_bufvec,
      M0_PI_SKIP_CALC_FINAL, (unsigned char *)&md5ctx, md5_digest);
  if (status < 0) {
    return -1;  // failure
  }
  s3_log(S3_LOG_DEBUG, "", "%s Displaying PI Info next", __func__);
  MD5hash::log_pi_info((struct m0_generic_pi *)&s3_md5_inc_digest_pi);
  is_finalized = true;
  return 0;
}

void MD5hash::Finalized() { is_finalized = true; }

void MD5hash::Initialized() { is_initialized = true; }

void MD5hash::Reset(std::shared_ptr<MotrAPI> motr_api,
                    bool call_init_for_update) {
  if (motr_api) {
    s3_motr_api = motr_api;
  } else {
    s3_motr_api = std::make_shared<ConcreteMotrAPI>();
  }
  status = 0;
  s3_md5_inc_digest_pi = {0};
  seed = {0};
  s3_md5_inc_digest_pi.pimd5c_hdr.pih_type = M0_PI_TYPE_MD5_INC_CONTEXT;
  pi_update_bufvec = {0};
  pi_bufvec.ov_vec.v_nr = 0;
  if (call_init_for_update) {
    pi_update_bufvec.ov_vec.v_nr = 0;
    pi_update_bufvec.ov_vec.v_count =
        (m0_bcount_t *)calloc(1, sizeof(m0_bcount_t));
    pi_update_bufvec.ov_buf = (void **)calloc(1, sizeof(void *));
    status = s3_motr_api->motr_client_calculate_pi(
        (struct m0_generic_pi *)&s3_md5_inc_digest_pi, NULL, &pi_update_bufvec,
        M0_PI_CALC_UNIT_ZERO, (unsigned char *)&md5ctx, NULL);
    if (status == 0) {
      s3_log(S3_LOG_DEBUG, "", "%s Displaying PI Info next", __func__);
      MD5hash::log_pi_info((struct m0_generic_pi *)&s3_md5_inc_digest_pi);
    }
    is_initialized = true;
  }
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

MD5hash::~MD5hash() {
  if (pi_update_bufvec.ov_vec.v_count) {
    free(pi_update_bufvec.ov_vec.v_count);
  }
  if (pi_update_bufvec.ov_buf) {
    free(pi_update_bufvec.ov_buf);
  }
}

void MD5hash::log_md5_inc_info(struct m0_md5_inc_context_pi *pi_info) {
  s3_log(S3_LOG_INFO, "", "%s ENTRY", __func__);

  const char hex_tbl[] = "0123456789abcdef";
  std::string s_hex;
  s_hex.reserve(sizeof(MD5_CTX) * 2);
  for (size_t i = 0; i < sizeof(MD5_CTX); ++i) {
    const unsigned ch = pi_info->pimd5c_prev_context[i] & 255;
    s_hex += hex_tbl[ch >> 4];
    s_hex += hex_tbl[ch & 15];
  }
  s3_log(S3_LOG_DEBUG, "", "%s prev_context : %s", __func__, s_hex.c_str());

  std::string s_hex_1;
  s_hex_1.reserve(MD5_DIGEST_LENGTH * 2);
  for (size_t i = 0; i < MD5_DIGEST_LENGTH; ++i) {
    const unsigned ch = pi_info->pimd5c_prev_context[i] & 255;
    s_hex_1 += hex_tbl[ch >> 4];
    s_hex_1 += hex_tbl[ch & 15];
  }
  s3_log(S3_LOG_DEBUG, "", "%s pi_value : %s", __func__, s_hex_1.c_str());

  s3_log(S3_LOG_INFO, "", "%s EXIT", __func__);
}

void MD5hash::log_pi_info(struct m0_generic_pi *pi_info) {
  s3_log(S3_LOG_INFO, "", "%s ENTRY", __func__);

  switch (S3Option::get_instance()->get_pi_type()) {
    case M0_PI_TYPE_MD5_INC_CONTEXT:
      s3_log(S3_LOG_DEBUG, "",
             "%s PI Info is of type M0_PI_TYPE_MD5_INC_CONTEXT", __func__);
      MD5hash::log_md5_inc_info((struct m0_md5_inc_context_pi *)pi_info);
      break;
    default:
      s3_log(S3_LOG_INFO, "", "%s Invalid PI Info Option.", __func__);
      break;
  }

  s3_log(S3_LOG_INFO, "", "%s EXIT", __func__);
}