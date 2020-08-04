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

#include "s3_option.h"
#include "s3_log.h"
#include "s3_audit_info_logger.h"
#include "s3_audit_info_logger_rsyslog_tcp.h"
#include "s3_audit_info_logger_log4cxx.h"
#include "s3_audit_info_logger_syslog.h"
#include "s3_audit_info_logger_kafka_web.h"

#include <stdexcept>

S3AuditInfoLoggerBase* S3AuditInfoLogger::audit_info_logger = nullptr;
bool S3AuditInfoLogger::audit_info_logger_enabled = false;

int S3AuditInfoLogger::init() {
  s3_log(S3_LOG_DEBUG, "", "Entering\n");
  int ret = 0;
  audit_info_logger = nullptr;
  audit_info_logger_enabled = false;
  std::string policy = S3Option::get_instance()->get_audit_logger_policy();
  if (policy == "disabled") {
    s3_log(S3_LOG_INFO, "", "Audit logger disabled by policy settings\n");
  } else if (policy == "rsyslog-tcp") {
    s3_log(S3_LOG_INFO, "", "Audit logger:> tcp %s:%d msgid:> %s\n",
           S3Option::get_instance()->get_audit_logger_host().c_str(),
           S3Option::get_instance()->get_audit_logger_port(),
           S3Option::get_instance()->get_audit_logger_rsyslog_msgid().c_str());
    try {
      audit_info_logger = new S3AuditInfoLoggerRsyslogTcp(
          S3Option::get_instance()->get_eventbase(),
          S3Option::get_instance()->get_audit_logger_host(),
          S3Option::get_instance()->get_audit_logger_port(),
          S3Option::get_instance()->get_audit_max_retry_count(),
          S3Option::get_instance()->get_audit_logger_rsyslog_msgid());
      audit_info_logger_enabled = true;
    }
    catch (std::exception const& ex) {
      s3_log(S3_LOG_ERROR, "", "Cannot create Rsyslog logger %s", ex.what());
      audit_info_logger = nullptr;
      ret = -1;
    }
  } else if (policy == "log4cxx") {
    s3_log(S3_LOG_INFO, "", "Audit logger:> log4cxx cfg_file %s\n",
           S3Option::get_instance()->get_s3_audit_config().c_str());
    try {
      audit_info_logger = new S3AuditInfoLoggerLog4cxx(
          S3Option::get_instance()->get_s3_audit_config(),
          S3Option::get_instance()->get_audit_log_dir() + "/audit/audit.log");
      audit_info_logger_enabled = true;
    }
    catch (...) {
      s3_log(S3_LOG_FATAL, "", "Cannot create Log4cxx logger");
      audit_info_logger = nullptr;
      ret = -1;
    }
  } else if (policy == "syslog") {
    s3_log(S3_LOG_INFO, "", "Audit logger:> syslog\n");
    audit_info_logger = new S3AuditInfoLoggerSyslog(
        S3Option::get_instance()->get_audit_logger_rsyslog_msgid());
    audit_info_logger_enabled = true;
  } else if (policy == "kafka-web") {
    s3_log(S3_LOG_INFO, "", "Audit logger:> kafka-web %s:%u%s",
           S3Option::get_instance()->get_audit_logger_host().c_str(),
           (unsigned)S3Option::get_instance()->get_audit_logger_port(),
           S3Option::get_instance()->get_audit_logger_kafka_web_path().c_str());
    try {
      audit_info_logger = new S3AuditInfoLoggerKafkaWeb(
          S3Option::get_instance()->get_eventbase(),
          S3Option::get_instance()->get_audit_logger_host(),
          S3Option::get_instance()->get_audit_logger_port(),
          S3Option::get_instance()->get_audit_logger_kafka_web_path());
      audit_info_logger_enabled = true;
    }
    catch (std::exception const& ex) {
      s3_log(S3_LOG_ERROR, "", "Cannot create Kafka Web logger %s", ex.what());
      audit_info_logger = nullptr;
      ret = -1;
    }
  } else {
    s3_log(S3_LOG_INFO, "", "Audit logger disabled by unknown policy %s\n",
           policy.c_str());
    ret = -1;
  }

  s3_log(S3_LOG_DEBUG, "", "Exiting ret %d\n", ret);
  return ret;
}

int S3AuditInfoLogger::save_msg(std::string const& cur_request_id,
                                std::string const& audit_logging_msg) {
  if (audit_info_logger) {
    return audit_info_logger->save_msg(cur_request_id, audit_logging_msg);
  }
  return 1;
}

bool S3AuditInfoLogger::is_enabled() { return audit_info_logger_enabled; }

void S3AuditInfoLogger::finalize() {
  s3_log(S3_LOG_DEBUG, "", "Entering\n");
  delete audit_info_logger;
  audit_info_logger = nullptr;
  audit_info_logger_enabled = false;
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}
