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

void S3UriToMeroOID(const char* name, struct m0_uint128 *object_id) {
  /* MurMur Hash */
  size_t len = 0;
  uint64_t hash128_64[2];

  len = strlen(name);
  MurmurHash3_x64_128(name, len, 0, &hash128_64);

  /* Truncate higher 32 bits as they are used by mero/clovis */
  // hash128_64[0] = hash128_64[0] & 0x0000ffff;
  hash128_64[1] = hash128_64[1] & 0xffff0000;

  *object_id = M0_CLOVIS_ID_APP;
  object_id->u_hi = hash128_64[0];
  object_id->u_lo = object_id->u_lo & hash128_64[1];
  return;
}
