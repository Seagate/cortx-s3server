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
