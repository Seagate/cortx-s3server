/*
 * COPYRIGHT 2020 SEAGATE LLC
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
 * Original author:  Dmitrii Surnin   <dmitrii.surnin@seagate.com>
 * Original creation date: 23-Jan-2020
 */

#include "s3_addb_map.h"

#include <assert.h>
#include <string.h>

const char* action_state_map[ADDB_TASK_LIST_OFFSET] = {
    "START",                          "RUNNING",
    "COMPLETE",                       "PAUSED",
    "STOPPED",                        "ERROR",
    "AUTH_OP_MT_SUCC",                "AUTH_OP_MT_FAILED",
    "AUTH_OP_SCHED_MT",               "AUTH_OP_ON_CONN_ERR",
    "AUTH_OP_ON_AUTHZ_RESP",          "AUTH_OP_ON_ACL_RESP",
    "AUTH_OP_ON_POLICY_RESP",         "AUTH_OP_ON_AUTH_RESP",
    "AUTH_OP_ON_TIMEOUT",             "AUTH_OP_ON_EVENT_CONNECTED",
    "AUTH_OP_ON_EVENT_ERROR",         "AUTH_OP_ON_EVENT_TIMEOUT",
    "AUTH_OP_ON_EVENT_EOF",           "AUTH_OP_ON_WRITE",
    "AUTH_OP_ON_HEADERS_READ",        "AUTH_OP_CTX_CONSTRUCT",
    "AUTH_OP_CTX_DESTRUCT",           "AUTH_OP_CTX_NEW_CONN",
    "AUTH_CLNT_CONSTRUCT",            "AUTH_CLNT_DESTRUCT",
    "AUTH_CLNT_EXEC_AUTH_CONN",       "AUTH_CLNT_TRIGGER_AUTH",
    "AUTH_CLNT_VALIDATE_ACL",         "AUTH_CLNT_VALIDATE_ACL_SUCC",
    "AUTH_CLNT_VALIDATE_ACL_FAILED",  "AUTH_CLNT_VALIDATE_POLICY",
    "AUTH_CLNT_VALIDATE_POLICY_SUCC", "AUTH_CLNT_VALIDATE_POLICY_FAILED",
    "AUTH_CLNT_CHK_AUTHZ_R",          "AUTH_CLNT_CHK_AUTHZ",
    "AUTH_CLNT_CHK_AUTHZ_SUCC",       "AUTH_CLNT_CHK_AUTHZ_FAILED",
    "AUTH_CLNT_CHK_AUTH_R",           "AUTH_CLNT_CHK_AUTH",
    "AUTH_CLNT_CHK_AUTH_SUCC",        "AUTH_CLNT_CHK_AUTH_FAILED",
    "AUTH_CLNT_CHUNK_AUTH_INIT",      "AUTH_CLNT_CHUNK_ADD_CHKSUMM",
    "AUTH_CLNT_CHUNK_AUTH_SUCC",      "AUTH_CLNT_CHUNK_AUTH_FAILED", };

const char* addb_idx_to_s3_state(uint64_t addb_idx) {
  if (addb_idx < ADDB_TASK_LIST_OFFSET) {
    assert(action_state_map[addb_idx]);
    return action_state_map[addb_idx];
  }

  uint64_t map_idx = addb_idx - ADDB_TASK_LIST_OFFSET;

  assert(map_idx < g_s3_to_addb_idx_func_name_map_size);
  assert(g_s3_to_addb_idx_func_name_map[map_idx]);

  return g_s3_to_addb_idx_func_name_map[map_idx];
}
