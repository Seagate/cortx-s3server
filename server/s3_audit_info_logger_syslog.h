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

#ifndef __S3_SERVER_AUDIT_INFO_LOGGER_SYSLOG_H__
#define __S3_SERVER_AUDIT_INFO_LOGGER_SYSLOG_H__

#include "s3_audit_info_logger_base.h"

class S3AuditInfoLoggerSyslog : public S3AuditInfoLoggerBase {
 public:
  S3AuditInfoLoggerSyslog(std::string const &msg_filter =
                              "s3server-audit-logging");
  virtual ~S3AuditInfoLoggerSyslog();
  virtual int save_msg(std::string const &, std::string const &);

 private:
  std::string rsyslog_msg_filter;
};

#endif
