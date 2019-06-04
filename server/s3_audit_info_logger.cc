/*
 * COPYRIGHT 2019 SEAGATE LLC
 *
 * THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
 * HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE TECHNOLOGY
 * LIMITED, ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
 * THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
 * BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
 * USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
 * EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.
 *
 * YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
 * THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
 * http://www.seagate.com/contact
 *
 * Original author:  Dmitrii Surnin <dmitrii.surnin@seagate.com>
 * Original creation date: 10-June-2019
 */

#include "s3_option.h"
#include "s3_log.h"
#include "s3_audit_info_logger.h"
#include "s3_audit_info_logger_rsyslog_tcp.h"

S3AuditInfoLoggerBase* S3AuditInfoLogger::audit_info_logger = nullptr;

int S3AuditInfoLogger::init(struct event_base* base) {
  s3_log(S3_LOG_DEBUG, "", "Entering\n");
  audit_info_logger = nullptr;
  std::string policy = S3Option::get_instance()->get_audit_logger_policy();
  if (S3Option::get_instance()->get_s3_audit_format_type() ==
      AuditFormatType::NONE) {
    s3_log(S3_LOG_INFO, "", "Audit logger disabled by format settitng NONE\n");
  } else if (policy == "disabled") {
    s3_log(S3_LOG_INFO, "", "Audit logger disabled by policy settings\n");
  } else if (policy == "rsyslog-tcp") {
    s3_log(S3_LOG_INFO, "", "Audit logger:> tcp %s:%d msgid:> %s\n",
           S3Option::get_instance()->get_audit_logger_host().c_str(),
           S3Option::get_instance()->get_audit_logger_port(),
           S3Option::get_instance()->get_audit_logger_rsyslog_msgid().c_str());
    audit_info_logger = new S3AuditInfoLoggerRsyslogTcp(
        base, S3Option::get_instance()->get_audit_logger_host(),
        S3Option::get_instance()->get_audit_logger_port(),
        S3Option::get_instance()->get_max_retry_count(),
        S3Option::get_instance()->get_audit_logger_rsyslog_msgid());
  }

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  return 0;
}

int S3AuditInfoLogger::save_msg(std::string const& cur_request_id,
                                std::string const& audit_logging_msg) {
  if (audit_info_logger) {
    audit_info_logger->save_msg(cur_request_id, audit_logging_msg);
  }
  return 0;
}

void S3AuditInfoLogger::finalize() {
  s3_log(S3_LOG_DEBUG, "", "Entering\n");
  delete audit_info_logger;
  audit_info_logger = nullptr;
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}
