#pragma once

#ifndef __S3_SERVER_AUDIT_INFO_LOGGER_BASE_H__
#define __S3_SERVER_AUDIT_INFO_LOGGER_BASE_H__

#include <string>

class S3AuditInfoLoggerBase {
 public:
  virtual ~S3AuditInfoLoggerBase() = default;
  virtual int save_msg(std::string const&, std::string const&) = 0;
};

#endif
