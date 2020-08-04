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

const char* measurement_map[ADDB_MEASUREMENT_LIST_OFFSET] = {"TRACE_POINT", };

const char* addb_measurement_to_name(uint64_t msrm_idx) {
  assert(msrm_idx < ADDB_MEASUREMENT_LIST_OFFSET);
  assert(measurement_map[msrm_idx] != NULL);
  return measurement_map[msrm_idx];
}

const char* addb_idx_to_s3_state(uint64_t addb_idx) {
  if (addb_idx < ADDB_TASK_LIST_OFFSET) {
    assert(action_state_map[addb_idx] != NULL);
    return action_state_map[addb_idx];
  }

  uint64_t map_idx = addb_idx - ADDB_TASK_LIST_OFFSET;

  assert(map_idx < g_s3_to_addb_idx_func_name_map_size);
  assert(g_s3_to_addb_idx_func_name_map[map_idx] != NULL);

  return g_s3_to_addb_idx_func_name_map[map_idx];
}
