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
