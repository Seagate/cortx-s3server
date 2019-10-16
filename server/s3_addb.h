/*
 * COPYRIGHT 2019 SEAGATE LLC
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
 * Original author: Evgeniy Brazhnikov
 * Original creation date: 4-Oct-2019
 */

#pragma once

#include <cstdint>
#include <vector>
#include <addb2/addb2.h>

// If new API would be added and new operation would be needed for
// ADDB logging then the enum below should be updated and the function
// get_addb_id() need to be overridden accordingly.

enum S3_ADDB_ENTRY_TYPE_ID : unsigned {
  ADDB_UNUSED_ID = 0,
  ADDB_RANGE_START_ID =
      0x10000,  // == M0_AVI_S3_RANGE_START (defined in addb2/identifier.h)
  // helper IDs e.g. for linking requests
  ADDB_REQUEST_ID = ADDB_RANGE_START_ID,
  ADDB_REQUEST_TO_CLOVIS_ID,
  // Different request types
  ADDB_BUCKET_DELETE_ID,
  ADDB_BUCKET_DELETE_POLICY_ID,
  ADDB_BUCKET_DELETE_TAGS_ID,
  ADDB_BUCKET_GET_ID,
  ADDB_BUCKET_GET_ACL_ID,
  ADDB_BUCKET_GET_LOCATION_ID,
  ADDB_BUCKET_GET_POLICY_ID,
  ADDB_BUCKET_GET_TAGS_ID,
  ADDB_BUCKET_HEAD_ID,
  ADDB_BUCKET_PUT_ID,
  ADDB_BUCKET_PUT_ACL_ID,
  ADDB_BUCKET_PUT_POLICY_ID,
  ADDB_BUCKET_PUT_TAGS_ID,
  ADDB_OBJECT_DELETE_ID,
  ADDB_OBJECT_DELETE_TAGS_ID,
  ADDB_OBJECT_GET_ID,
  ADDB_OBJECT_GET_ACL_ID,
  ADDB_OBJECT_GET_TAGS_ID,
  ADDB_OBJECT_HEAD_ID,
  ADDB_OBJECT_POST_MULTIPART_ID,
  ADDB_OBJECT_PUT_ID,
  ADDB_OBJECT_PUT_ACL_ID,
  ADDB_OBJECT_PUT_CHUNK_ID,
  ADDB_OBJECT_PUT_MULTIPART_ID,
  ADDB_OBJECT_PUT_TAGS_ID
};

// The goal is to distinguish ActionState values from task_list indexes.
#define ADDB_TASK_LIST_OFFSET 256u

// The id should be (the result of) get_addb_id().
// Other parameters should be (arbitrary) values of type uint64_t
// (or implicitly convertible)

#define ADDB(id, ...)                                           \
  do {                                                          \
    const uint64_t addb_id__ = (id);                            \
    if (FLAGS_addb && (addb_id__ != ADDB_UNUSED_ID)) {          \
      uint64_t addb_params__[] = {__VA_ARGS__};                 \
      constexpr auto addb_pars_size__ =                         \
          sizeof addb_params__ / sizeof addb_params__[0];       \
      m0_addb2_add(addb_id__, addb_pars_size__, addb_params__); \
    }                                                           \
  } while (false)
