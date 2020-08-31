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

#include "motr_helpers.h"
#include "s3_uri_to_motr_oid.h"
#include "fid/fid.h"
#include "murmur3_hash.h"
#include "s3_common.h"
#include "s3_log.h"
#include "s3_perf_logger.h"
#include "s3_stats.h"
#include "s3_timer.h"
#include "s3_iem.h"

int S3UriToMotrOID(std::shared_ptr<MotrAPI> s3_motr_api, const char *name,
                   const std::string &request_id, m0_uint128 *ufid,
                   S3MotrEntityType type) {
  s3_log(S3_LOG_DEBUG, "", "Entering\n");
  int rc;
  S3Timer timer;
  struct m0_uint128 tmp_uint128;
  struct m0_fid index_fid;

  bool is_murmurhash_fid =
      S3Option::get_instance()->is_murmurhash_oid_enabled();
  if (ufid == NULL) {
    s3_log(S3_LOG_ERROR, request_id, "Invalid argument, ufid pointer is NULL");
    return -EINVAL;
  }
  if (name == NULL) {
    s3_log(S3_LOG_ERROR, request_id,
           "Invalid argument, input parameter 'name' is NULL\n");
    return -EINVAL;
  }

  timer.start();

  if (is_murmurhash_fid) {
    // Murmur Hash Algorithm usage for OID generation
    size_t len;
    uint64_t hash128_64[2];

    len = strlen(name);
    if (len == 0) {
      // oid should not be 0
      s3_log(S3_LOG_ERROR, request_id,
             "The input parameter 'name' is empty string\n");
      return -EINVAL;
    }
    MurmurHash3_x64_128(name, len, 0, &hash128_64);

    //
    // Reset the higher 37 bits, will be used by Motr
    // The lower 91 bits used by S3
    // https://jts.seagate.com/browse/CASTOR-2155

    hash128_64[0] = hash128_64[0] & 0x0000000007ffffff;
    tmp_uint128.u_hi = hash128_64[0];
    tmp_uint128.u_lo = hash128_64[1];

    // Ensure OID does not fall in motr and S3 reserved range.
    struct m0_uint128 s3_range = {0ULL, 0ULL};
    s3_range.u_lo = S3_OID_RESERVED_COUNT;

    struct m0_uint128 reserved_range = {0ULL, 0ULL};
    m0_uint128_add(&reserved_range, &M0_ID_APP, &s3_range);

    rc = m0_uint128_cmp(&reserved_range, &tmp_uint128);
    if (rc >= 0) {
      struct m0_uint128 res;
      // ID should be more than M0_ID_APP
      s3_log(S3_LOG_DEBUG, "",
             "Id from Murmur hash algorithm less than M0_ID_APP\n");
      m0_uint128_add(&res, &reserved_range, &tmp_uint128);
      tmp_uint128.u_hi = res.u_hi;
      tmp_uint128.u_lo = res.u_lo;
      tmp_uint128.u_hi = tmp_uint128.u_hi & 0x0000000007ffffff;
    }
    *ufid = tmp_uint128;
  } else {
    // Unique OID generation by motr.
    if (s3_motr_api == NULL) {
      s3_motr_api = std::make_shared<ConcreteMotrAPI>();
    }
    rc = s3_motr_api->m0_h_ufid_next(ufid);
    if (rc != 0) {
      s3_log(S3_LOG_ERROR, request_id, "Failed to generate UFID\n");
      // May need to change error code to something better in future -- TODO
      s3_iem(LOG_ALERT, S3_IEM_MOTR_CONN_FAIL, S3_IEM_MOTR_CONN_FAIL_STR,
             S3_IEM_MOTR_CONN_FAIL_JSON);
      return rc;
    }
  }

  if (type == S3MotrEntityType::index) {
    index_fid = M0_FID_TINIT('x', ufid->u_hi, ufid->u_lo);
    ufid->u_hi = index_fid.f_container;
    ufid->u_lo = index_fid.f_key;
  }

  s3_log(S3_LOG_DEBUG, request_id,
         "uri = %s entity = %s oid = "
         "%" SCNx64 " : %" SCNx64 "\n",
         name, motr_entity_type_to_string(type).c_str(), ufid->u_hi,
         ufid->u_lo);
  timer.stop();
  LOG_PERF("S3UriToMotrOID_ns", request_id.c_str(),
           timer.elapsed_time_in_nanosec());
  s3_stats_timing("uri_to_motr_oid", timer.elapsed_time_in_millisec());

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  return 0;
}
