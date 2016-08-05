/*
 * COPYRIGHT 2015 SEAGATE LLC
 *
 * THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
 * HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE TECHNOLOGY
 * LIMITED, ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
 * THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
 * BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
 * USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
 * EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.
 *
 * YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
 * THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
 * http://www.seagate.com/contact
 *
 * Original author:  Rajesh Nambiar   <rajesh.nambiar@seagate.com>
 * Original creation date: 1-Oct-2015
 */

#include "s3_uri_to_mero_oid.h"
#include "murmur3_hash.h"
#include "s3_timer.h"
#include "s3_perf_logger.h"
#include "s3_log.h"

void S3UriToMeroOID(const char* name, struct m0_uint128 *object_id) {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  /* MurMur Hash */
  S3Timer timer;
  timer.start();
  size_t len = 0;
  uint64_t hash128_64[2];
  struct m0_uint128 tmp_uint128;
  object_id->u_hi = object_id->u_lo = 0;
  if (name == NULL) {
    s3_log(S3_LOG_FATAL, "The input parameter 'name' is NULL\n");
    return;
  }

  len = strlen(name);
  if (len == 0) {
    // oid should not be 0
    s3_log(S3_LOG_FATAL, "The input parameter 'name' is empty string\n");
    return;
  }
  MurmurHash3_x64_128(name, len, 0, &hash128_64);

  // Reset the higher 8 bits, will be used by Mero
  hash128_64[0] = hash128_64[0] & 0x00ffffffffffffff;
  tmp_uint128.u_hi = hash128_64[0];
  tmp_uint128.u_lo = hash128_64[1];
  int rc = m0_uint128_cmp(&M0_CLOVIS_ID_APP, &tmp_uint128);
  if (rc >= 0) {
    struct m0_uint128 res;
    // ID should be more than M0_CLOVIS_ID_APP
    s3_log(S3_LOG_DEBUG,
           "Id from Murmur hash algorithm less than M0_CLOVIS_ID_APP\n");
    m0_uint128_add(&res, &M0_CLOVIS_ID_APP, &tmp_uint128);
    tmp_uint128.u_hi = res.u_hi;
    tmp_uint128.u_lo = res.u_lo;
    tmp_uint128.u_hi = tmp_uint128.u_hi & 0x00ffffffffffffff;
  }
  *object_id = tmp_uint128;
  s3_log(S3_LOG_DEBUG, "ID for %s is %llu %llu\n", name, object_id->u_hi, object_id->u_lo);

  timer.stop();
  LOG_PERF("S3UriToMeroOID_ns", timer.elapsed_time_in_nanosec());

  s3_log(S3_LOG_DEBUG, "Exiting\n");
  return;
}
