#pragma once

#ifndef __S3_SERVER_AUDIT_INFO_LOGGER_LOG4CXX_H__
#define __S3_SERVER_AUDIT_INFO_LOGGER_LOG4CXX_H__

#include "s3_audit_info_logger_base.h"

#include "log4cxx/logger.h"

class S3AuditInfoLoggerLog4cxx : public S3AuditInfoLoggerBase {
 public:
  S3AuditInfoLoggerLog4cxx(std::string const &, std::string const & = "");
  virtual ~S3AuditInfoLoggerLog4cxx();
  virtual int save_msg(std::string const &, std::string const &);

 private:
  log4cxx::LoggerPtr l4c_logger_ptr;
};

#endif
