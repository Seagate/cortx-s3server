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

#ifndef __S3_SERVER_ADDB_H__
#define __S3_SERVER_ADDB_H__

#include <cstdint>
#include <vector>
#include <addb2/addb2.h>

#include "s3_addb_plugin_auto.h"
#include "s3_addb_map.h"

// This header defines what is needed to use Motr ADDB.
//
// Motr ADDB is a subsystem which allows to efficiently store certain run-time
// metrics.  They are called ADDB log entries.  Every entry is time stamped
// (with nanosecond precision), and contains up to 15 unsigned 64 bit integers.
// It is up to us what information to store there.  The convention is that the
// first integer is an "action type id" (see enum S3AddbActionTypeId).  This ID
// must be within a range designated to S3 server.  It serves later as a means
// to distinguish between different kinds of log entries that we save to ADDB.
//
// The very first basic usage for ADDB is performance monitoring: s3 server
// will track every API request, and create ADDB log entries when the request
// goes through it's stages, and when it changes state, and also track which
// Motr operations it executes.

// Initialize addb subsystem (see detailed comments in the implementation).
// Depends on s3_log.
int s3_addb_init();

// Used for s3starup calls without any request-objects.
#define S3_ADDB_STARTUP_REQUESTS_ID 1
// Default addb beginning request id.
#define S3_ADDB_FIRST_GENERIC_REQUESTS_ID 2

// Macro to create ADDB log entry.
//
// addb_action_type_id parameter must be a value from enum S3AddbActionTypeId.
// See also Action::get_addb_action_type_id().
//
// Other parameters must be values of type uint64_t (or implicitly
// convertible).  Current limit is 14 parameters (totals to 15 integers in one
// addb entry since we include addb_action_id).
#define ADDB(addb_action_type_id, ...)                                        \
  do {                                                                        \
    const uint64_t addb_id__ = (addb_action_type_id);                         \
    if (FLAGS_addb) {                                                         \
      if (addb_id__ < S3_ADDB_RANGE_START || addb_id__ > S3_ADDB_RANGE_END) { \
        s3_log(S3_LOG_FATAL, "", "Invalid ADDB Action Type ID %" PRIu64 "\n", \
               addb_id__);                                                    \
      } else {                                                                \
        uint64_t addb_params__[] = {__VA_ARGS__};                             \
        constexpr auto addb_params_size__ =                                   \
            sizeof addb_params__ / sizeof addb_params__[0];                   \
        m0_addb2_add(addb_id__, addb_params_size__, addb_params__);           \
      }                                                                       \
    }                                                                         \
  } while (false)

// Macro to create records of free format independent from request state
// can handle up to 13 params
// format of each record should be specified by msrm_id of type MeasurementId
#define ADDB_MSRM(msrm_id, ...) \
  ADDB(S3_ADDB_MEASUREMENT_ID, (msrm_id), __VA_ARGS__)

#endif
