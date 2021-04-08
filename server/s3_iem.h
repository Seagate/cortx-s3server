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

#ifndef __S3_SERVER_IEM_H__
#define __S3_SERVER_IEM_H__

#include <unistd.h>
#include "s3_log.h"
#include "s3_option.h"

extern S3Option* g_option_instance;

// Note:
// 1. Logs IEM message into syslog as well as to glog.
// 2. For each IEM: timestamp, nodename, pid, filename & line is included in
//    the JSON data format by default. If IEM event has any additional data to
//    send, then pass it as `json_fmt` param.
#define s3_iem_(loglevel, s3_loglevel, event_code, event_desc, json_fmt, ...) \
  do {                                                                        \
    if (loglevel == LOG_ALERT) {                                              \
      s3_syslog(loglevel, "IEC:AS" event_code ":" event_desc);                \
    } else {                                                                  \
      s3_syslog(LOG_ERR, "IEC:ES" event_code ":" event_desc);                 \
    }                                                                         \
    std::string timestamp = s3_get_timestamp();                               \
    s3_log(S3_##s3_loglevel, "",                                              \
           "IEC: " event_code ": " event_desc                                 \
           ": { \"time\": \"%s\", \"node\": \"%s\", \"pid\": "                \
           "%d, \"file\": \"%s\", \"line\": %d" json_fmt " }",                \
           timestamp.c_str(), g_option_instance->get_s3_nodename().c_str(),   \
           getpid(), __FILE__, __LINE__, ##__VA_ARGS__);                      \
  } while (0)

#define s3_iem(loglevel, event_code, event_desc, json_fmt, ...) \
  s3_iem_(loglevel, LOG_INFO, event_code, event_desc, json_fmt, ##__VA_ARGS__)

#define s3_iem_fatal(loglevel, event_code, event_desc, json_fmt, ...) \
  s3_iem_(loglevel, LOG_ERROR, event_code, event_desc, json_fmt, ##__VA_ARGS__)

// Note: Logs IEM message into syslog only.
// Use this macro to send addtional information to CSM
#define s3_iem_syslog(loglevel, event_code, event_description, ...)  \
  do {                                                               \
    if (loglevel == LOG_INFO) {                                      \
      s3_syslog(loglevel, "IEC:IS" event_code ":" event_description, \
                ##__VA_ARGS__);                                      \
    } else {                                                         \
      s3_syslog(LOG_ERR, "IEC:ES" event_code ":" event_description,  \
                ##__VA_ARGS__);                                      \
    }                                                                \
  } while (0)

// IEM helper macros
// S3 Server IEM Event codes, description string & json data format
#define S3_IEM_AUTH_CONN_FAIL "0030010001"
#define S3_IEM_AUTH_CONN_FAIL_STR                                         \
  "S3 server cannot connect to auth server. For more information, refer " \
  "Troubleshooting Guide."
#define S3_IEM_AUTH_CONN_FAIL_JSON ""

#define S3_IEM_CHUNK_PARSING_FAIL "0030020001"
#define S3_IEM_CHUNK_PARSING_FAIL_STR \
  "Invalid chunk payload. For more information, refer Troubleshooting Guide."
#define S3_IEM_CHUNK_PARSING_FAIL_JSON ""

#define S3_IEM_MOTR_CONN_FAIL "0030030001"
#define S3_IEM_MOTR_CONN_FAIL_STR                                        \
  "Motr connection failed. For more information, refer Troubleshooting " \
  "Guide."
#define S3_IEM_MOTR_CONN_FAIL_JSON ""

#define S3_IEM_COLLISION_RES_FAIL "0030040001"
#define S3_IEM_COLLISION_RES_FAIL_STR \
  "Collision resolution failed. Please retry."
#define S3_IEM_COLLISION_RES_FAIL_JSON ""

#define S3_IEM_FATAL_HANDLER "0030050001"
#define S3_IEM_FATAL_HANDLER_STR                                              \
  "Fatal signal error received. For more information, refer Troubleshooting " \
  "Guide."
#define S3_IEM_FATAL_HANDLER_JSON ", \"signal_number\": %d"

#define S3_IEM_DELETE_OBJ_FAIL "0030060001"
#define S3_IEM_DELETE_OBJ_FAIL_STR                                      \
  "Delete operation failed for Object. For more information refer the " \
  "Troubleshooting Guide."
#define S3_IEM_DELETE_OBJ_FAIL_JSON ""

#define S3_IEM_DELETE_IDX_FAIL "0030060002"
#define S3_IEM_DELETE_IDX_FAIL_STR                                     \
  "Delete operation failed for Index. For more information refer the " \
  "Troubleshooting Guide."
#define S3_IEM_DELETE_IDX_FAIL_JSON ""

#define S3_IEM_METADATA_CORRUPTED "0030060003"
#define S3_IEM_METADATA_CORRUPTED_STR \
  "Metadata may be corrupted. Contact Seagate Support."
#define S3_IEM_METADATA_CORRUPTED_JSON ""

#define S3_IEM_CHECKSUM_MISMATCH "0030060004"
#define S3_IEM_CHECKSUM_MISMATCH_STR                                \
  "Data integrity failure (content checksum mismatch). Cluster is " \
  "transitioning to safe mode. Contact Seagate Support."
#define S3_IEM_CHECKSUM_MISMATCH_JSON                                 \
  ", \"bucket_name\": \"%s\", \"object_name\": \"%s\", \"motr_oid\":" \
  " \"%" SCNx64 ":%" SCNx64 "\", \"md5_calc\": \"%s\", \"md5_read\": \"%s\" "

#endif  // __S3_SERVER_IEM_H__
