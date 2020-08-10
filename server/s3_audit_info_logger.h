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

#ifndef __S3_SERVER_AUDIT_INFO_LOGGER__H__
#define __S3_SERVER_AUDIT_INFO_LOGGER__H__

#include "s3_audit_info_logger_base.h"

#include "gtest/gtest_prod.h"

class S3AuditInfoLogger {
 private:
  static S3AuditInfoLoggerBase* audit_info_logger;
  static bool audit_info_logger_enabled;

 public:
  static int init();
  static void finalize();
  static bool is_enabled();
  static int save_msg(std::string const&, std::string const&);
  S3AuditInfoLogger() = delete;

 private:
  FRIEND_TEST(S3AuditInfoLoggerTest, PolicyDisabled);
  FRIEND_TEST(S3AuditInfoLoggerTest, PolicyInvalid);
  FRIEND_TEST(S3AuditInfoLoggerTest, MissedInit);
  FRIEND_TEST(S3AuditInfoLoggerTest, PolicyLog4cxx);
  FRIEND_TEST(S3AuditInfoLoggerTest, PolicySyslog);
  FRIEND_TEST(S3AuditInfoLoggerTest, PolicyRsyslogTcp);
  FRIEND_TEST(S3AuditInfoLoggerTest, PolicyRsyslogTcpBaseNULL);
  FRIEND_TEST(S3AuditInfoLoggerTest, PolicyKafkaWeb);
};

#endif
