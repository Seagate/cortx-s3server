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

#ifndef __S3_SERVER_ADDB_MAP_H__
#define __S3_SERVER_ADDB_MAP_H__

#include <addb2/addb2_internal.h>

typedef enum {
  // general purpose states
  ACTS_START = 0,
  ACTS_RUNNING,
  ACTS_COMPLETE,
  ACTS_PAUSED,
  ACTS_STOPPED,  // Aborted
  ACTS_ERROR,

  // auth states
  ACTS_AUTH_OP_MT_SUCC,      // auth op: succ cb call in libevent main thread
  ACTS_AUTH_OP_MT_FAILED,    // auth op: failed cb call in libevent main thread
  ACTS_AUTH_OP_SCHED_MT,     // schedule auth op fini cb to libevetn main thread
  ACTS_AUTH_OP_ON_CONN_ERR,  // auth client connection error
  ACTS_AUTH_OP_ON_AUTHZ_RESP,             // authorisation responce recived
  ACTS_AUTH_OP_ON_ACL_RESP,               // ACL resonce received
  ACTS_AUTH_OP_ON_POLICY_RESP,            // policy responce received
  ACTS_AUTH_OP_ON_AUTH_RESP,              // authentication responce received
  ACTS_AUTH_OP_ON_TIMEOUT,                // operation timeout
  ACTS_AUTH_OP_ON_EVENT_CONNECTED,        // auth sever connection established
  ACTS_AUTH_OP_ON_EVENT_ERROR,            // connetcion op error
  ACTS_AUTH_OP_ON_EVENT_TIMEOUT,          // connection op timeout
  ACTS_AUTH_OP_ON_EVENT_EOF,              // connection op get EOF flag
  ACTS_AUTH_OP_ON_WRITE,                  // done writing to underlying socket
  ACTS_AUTH_OP_ON_HEADERS_READ,           // http headers received
  ACTS_AUTH_OP_CTX_CONSTRUCT,             // context constructor called
  ACTS_AUTH_OP_CTX_DESTRUCT,              // context destructor called
  ACTS_AUTH_OP_CTX_NEW_CONN,              // new connection initiated
  ACTS_AUTH_CLNT_CONSTRUCT,               // client constructor called
  ACTS_AUTH_CLNT_DESTRUCT,                // client destructor called
  ACTS_AUTH_CLNT_EXEC_AUTH_CONN,          // exec request
  ACTS_AUTH_CLNT_TRIGGER_AUTH,            // trigger authentification
  ACTS_AUTH_CLNT_VALIDATE_ACL,            // validate ACL
  ACTS_AUTH_CLNT_VALIDATE_ACL_SUCC,       // ACL succ cb called
  ACTS_AUTH_CLNT_VALIDATE_ACL_FAILED,     // ACL failed cb called
  ACTS_AUTH_CLNT_VALIDATE_POLICY,         // validate policy
  ACTS_AUTH_CLNT_VALIDATE_POLICY_SUCC,    // policy succ cb called
  ACTS_AUTH_CLNT_VALIDATE_POLICY_FAILED,  // policy failed cb called
  ACTS_AUTH_CLNT_CHK_AUTHZ_R,             // repeat authorisation call
  ACTS_AUTH_CLNT_CHK_AUTHZ,               // authorisation triggered
  ACTS_AUTH_CLNT_CHK_AUTHZ_SUCC,          // authorisation succ cb called
  ACTS_AUTH_CLNT_CHK_AUTHZ_FAILED,        // authorisation failed cb called
  ACTS_AUTH_CLNT_CHK_AUTH_R,              // repeat authentification call
  ACTS_AUTH_CLNT_CHK_AUTH,                // authentification triggered
  ACTS_AUTH_CLNT_CHK_AUTH_SUCC,           // authentification succ cb called
  ACTS_AUTH_CLNT_CHK_AUTH_FAILED,         // authentification failed cb called
  ACTS_AUTH_CLNT_CHUNK_AUTH_INIT,         // authentification chunk init
  ACTS_AUTH_CLNT_CHUNK_ADD_CHKSUMM,  // authentification chunk add checksumm
  ACTS_AUTH_CLNT_CHUNK_AUTH_SUCC,    // authentification chunk succ cb called
  ACTS_AUTH_CLNT_CHUNK_AUTH_FAILED,  // authentification chunk failed cb called

  // Used to distinguish ActionState values from task_list indexes.
  ADDB_TASK_LIST_OFFSET
} ActionState;

// Record format description to be used together with ADDB_MSRM
typedef enum {
  // records of this type will contain only timestamp and some int value
  ADDB_MSRM_TRACE_POINT,

  // ---
  ADDB_MEASUREMENT_LIST_OFFSET
} MeasurementId;

extern const char* g_s3_to_addb_idx_func_name_map[];
extern const uint64_t g_s3_to_addb_idx_func_name_map_size;

const char* addb_measurement_to_name(uint64_t msrm_idx);
const char* addb_idx_to_s3_state(uint64_t addb_idx);

#endif
