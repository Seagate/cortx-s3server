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
