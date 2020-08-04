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

#include "s3_audit_info_logger_syslog.h"
#include "s3_log.h"
#include <syslog.h>

// NOTE: syslog in used by RAS product, so recofiguration should be avoided
// so openlog/closelog should be avoided

S3AuditInfoLoggerSyslog::S3AuditInfoLoggerSyslog(std::string const &msg_filter)
    : rsyslog_msg_filter(msg_filter) {}

S3AuditInfoLoggerSyslog::~S3AuditInfoLoggerSyslog() {}

int S3AuditInfoLoggerSyslog::save_msg(std::string const &cur_request_id,
                                      std::string const &audit_logging_msg) {
  s3_log(S3_LOG_DEBUG, cur_request_id, "Entering\n");
  syslog(LOG_NOTICE, "%s%s", rsyslog_msg_filter.c_str(),
         audit_logging_msg.c_str());
  s3_log(S3_LOG_DEBUG, cur_request_id, "Exiting\n");
  return 0;
}
