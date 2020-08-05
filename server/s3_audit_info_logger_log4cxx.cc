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

#include "s3_audit_info_logger_log4cxx.h"
#include "s3_log.h"

#include "log4cxx/basicconfigurator.h"
#include "log4cxx/helpers/exception.h"
#include "log4cxx/propertyconfigurator.h"
#include "log4cxx/helpers/fileinputstream.h"
#include "log4cxx/helpers/properties.h"
#include <sys/stat.h>

using namespace log4cxx;
using namespace log4cxx::helpers;

S3AuditInfoLoggerLog4cxx::S3AuditInfoLoggerLog4cxx(
    std::string const &log_config_path, std::string const &audit_log_filepath)
    : l4c_logger_ptr(0) {
  // NOTE: can throw exception
  FileInputStreamPtr prop_config_file(new FileInputStream(log_config_path));
  Properties properties;
  properties.load(prop_config_file);
  if (!audit_log_filepath.empty()) {
    // Override the audit log file path specified in audit config file
    // to the value provided in the constructor
    // LOG4CXX_DECODE_CHAR(logger_filepath, audit_log_filepath);
    properties.setProperty(LOG4CXX_STR("log4j.appender.Audit.File"),
                           LOG4CXX_STR(audit_log_filepath.c_str()));
    s3_log(S3_LOG_INFO, "", "Audit log file path is set to [%s]\n",
           audit_log_filepath.c_str());
  }
  log4cxx::PropertyConfigurator::configure(properties);
  l4c_logger_ptr = Logger::getLogger("Audit_Logger");
}

S3AuditInfoLoggerLog4cxx::~S3AuditInfoLoggerLog4cxx() { l4c_logger_ptr = 0; }

int S3AuditInfoLoggerLog4cxx::save_msg(std::string const &cur_request_id,
                                       std::string const &audit_logging_msg) {
  s3_log(S3_LOG_DEBUG, cur_request_id, "Entering\n");
  LOG4CXX_INFO(l4c_logger_ptr, audit_logging_msg);
  s3_log(S3_LOG_DEBUG, cur_request_id, "Exiting\n");
  return 0;
}
