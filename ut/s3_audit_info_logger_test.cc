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

#include <gtest/gtest.h>

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/dns.h>
#include <event2/util.h>

#include "s3_option.h"
#include "s3_audit_info_logger.h"

class OptionsWrapper {
 private:
  std::string option_file;
  std::string logger_policy;
  evbase_t *event_base;
  evbase_t *new_base;

 public:
  OptionsWrapper(std::string const &log_pol)
      : option_file(S3Option::get_instance()->get_option_file()),
        logger_policy(S3Option::get_instance()->get_audit_logger_policy()),
        event_base(S3Option::get_instance()->get_eventbase()),
        new_base(nullptr) {
    S3Option::get_instance()->set_option_file("s3config-test.yaml");
    S3Option::get_instance()->set_audit_logger_policy(log_pol);
  }

  ~OptionsWrapper() {
    S3Option::get_instance()->set_option_file(option_file);
    S3Option::get_instance()->set_audit_logger_policy(logger_policy);
    S3Option::get_instance()->set_eventbase(event_base);

    if (new_base) {
      event_base_free(new_base);
    }
  }

  void upd_base() {
    new_base = event_base_new();
    S3Option::get_instance()->set_eventbase(new_base);
  }
};

TEST(S3AuditInfoLoggerTest, PolicyDisabled) {
  OptionsWrapper ow("disabled");

  EXPECT_EQ(0, S3AuditInfoLogger::init());
  EXPECT_EQ(nullptr, S3AuditInfoLogger::audit_info_logger);
  EXPECT_EQ(false, S3AuditInfoLogger::audit_info_logger_enabled);
  EXPECT_EQ(false, S3AuditInfoLogger::is_enabled());
  EXPECT_EQ(1, S3AuditInfoLogger::save_msg("test_req_id", "test_audit_msg"));

  S3AuditInfoLogger::finalize();
  EXPECT_EQ(nullptr, S3AuditInfoLogger::audit_info_logger);
  EXPECT_EQ(false, S3AuditInfoLogger::audit_info_logger_enabled);
  EXPECT_EQ(false, S3AuditInfoLogger::is_enabled());
  EXPECT_EQ(1, S3AuditInfoLogger::save_msg("test_req_id", "test_audit_msg"));
}

TEST(S3AuditInfoLoggerTest, PolicyInvalid) {
  OptionsWrapper ow("bad policy");

  EXPECT_EQ(-1, S3AuditInfoLogger::init());
  EXPECT_EQ(nullptr, S3AuditInfoLogger::audit_info_logger);
  EXPECT_EQ(false, S3AuditInfoLogger::audit_info_logger_enabled);
  EXPECT_EQ(false, S3AuditInfoLogger::is_enabled());
  EXPECT_EQ(1, S3AuditInfoLogger::save_msg("test_req_id", "test_audit_msg"));

  S3AuditInfoLogger::finalize();
  EXPECT_EQ(nullptr, S3AuditInfoLogger::audit_info_logger);
  EXPECT_EQ(false, S3AuditInfoLogger::audit_info_logger_enabled);
  EXPECT_EQ(false, S3AuditInfoLogger::is_enabled());
  EXPECT_EQ(1, S3AuditInfoLogger::save_msg("test_req_id", "test_audit_msg"));
}

TEST(S3AuditInfoLoggerTest, MissedInit) {
  EXPECT_EQ(nullptr, S3AuditInfoLogger::audit_info_logger);
  EXPECT_EQ(false, S3AuditInfoLogger::audit_info_logger_enabled);
  EXPECT_EQ(false, S3AuditInfoLogger::is_enabled());
  EXPECT_EQ(1, S3AuditInfoLogger::save_msg("test_req_id", "test_audit_msg"));

  S3AuditInfoLogger::finalize();
  EXPECT_EQ(nullptr, S3AuditInfoLogger::audit_info_logger);
  EXPECT_EQ(false, S3AuditInfoLogger::audit_info_logger_enabled);
  EXPECT_EQ(false, S3AuditInfoLogger::is_enabled());
  EXPECT_EQ(1, S3AuditInfoLogger::save_msg("test_req_id", "test_audit_msg"));
}

TEST(S3AuditInfoLoggerTest, PolicyLog4cxx) {
  OptionsWrapper ow("log4cxx");

  EXPECT_EQ(0, S3AuditInfoLogger::init());
  EXPECT_NE(nullptr, S3AuditInfoLogger::audit_info_logger);
  EXPECT_EQ(true, S3AuditInfoLogger::audit_info_logger_enabled);
  EXPECT_EQ(true, S3AuditInfoLogger::is_enabled());
  EXPECT_EQ(0, S3AuditInfoLogger::save_msg("test_req_id", "test_audit_msg"));

  S3AuditInfoLogger::finalize();
  EXPECT_EQ(nullptr, S3AuditInfoLogger::audit_info_logger);
  EXPECT_EQ(false, S3AuditInfoLogger::audit_info_logger_enabled);
  EXPECT_EQ(false, S3AuditInfoLogger::is_enabled());
  EXPECT_EQ(1, S3AuditInfoLogger::save_msg("test_req_id", "test_audit_msg"));
}

TEST(S3AuditInfoLoggerTest, PolicySyslog) {
  OptionsWrapper ow("syslog");

  EXPECT_EQ(0, S3AuditInfoLogger::init());
  EXPECT_NE(nullptr, S3AuditInfoLogger::audit_info_logger);
  EXPECT_EQ(true, S3AuditInfoLogger::audit_info_logger_enabled);
  EXPECT_EQ(true, S3AuditInfoLogger::is_enabled());
  EXPECT_EQ(0, S3AuditInfoLogger::save_msg("test_req_id", "test_audit_msg"));

  S3AuditInfoLogger::finalize();
  EXPECT_EQ(nullptr, S3AuditInfoLogger::audit_info_logger);
  EXPECT_EQ(false, S3AuditInfoLogger::audit_info_logger_enabled);
  EXPECT_EQ(false, S3AuditInfoLogger::is_enabled());
  EXPECT_EQ(1, S3AuditInfoLogger::save_msg("test_req_id", "test_audit_msg"));
}

TEST(S3AuditInfoLoggerTest, PolicyRsyslogTcp) {
  OptionsWrapper ow("rsyslog-tcp");
  ow.upd_base();

  EXPECT_EQ(0, S3AuditInfoLogger::init());
  EXPECT_NE(nullptr, S3AuditInfoLogger::audit_info_logger);
  EXPECT_EQ(true, S3AuditInfoLogger::audit_info_logger_enabled);
  EXPECT_EQ(true, S3AuditInfoLogger::is_enabled());
  EXPECT_EQ(0, S3AuditInfoLogger::save_msg("test_req_id", "test_audit_msg"));

  S3AuditInfoLogger::finalize();
  EXPECT_EQ(nullptr, S3AuditInfoLogger::audit_info_logger);
  EXPECT_EQ(false, S3AuditInfoLogger::audit_info_logger_enabled);
  EXPECT_EQ(false, S3AuditInfoLogger::is_enabled());
  EXPECT_EQ(1, S3AuditInfoLogger::save_msg("test_req_id", "test_audit_msg"));
}

TEST(S3AuditInfoLoggerTest, PolicyRsyslogTcpBaseNULL) {
  OptionsWrapper ow("rsyslog-tcp");

  EXPECT_EQ(-1, S3AuditInfoLogger::init());
  EXPECT_EQ(nullptr, S3AuditInfoLogger::audit_info_logger);
  EXPECT_EQ(false, S3AuditInfoLogger::audit_info_logger_enabled);
  EXPECT_EQ(false, S3AuditInfoLogger::is_enabled());
  EXPECT_EQ(1, S3AuditInfoLogger::save_msg("test_req_id", "test_audit_msg"));

  S3AuditInfoLogger::finalize();
  EXPECT_EQ(nullptr, S3AuditInfoLogger::audit_info_logger);
  EXPECT_EQ(false, S3AuditInfoLogger::audit_info_logger_enabled);
  EXPECT_EQ(false, S3AuditInfoLogger::is_enabled());
  EXPECT_EQ(1, S3AuditInfoLogger::save_msg("test_req_id", "test_audit_msg"));
}

TEST(S3AuditInfoLoggerTest, PolicyKafkaWeb) {
  OptionsWrapper ow("kafka-web");

  EXPECT_EQ(0, S3AuditInfoLogger::init());
  EXPECT_NE(nullptr, S3AuditInfoLogger::audit_info_logger);
  EXPECT_EQ(true, S3AuditInfoLogger::audit_info_logger_enabled);
  EXPECT_EQ(true, S3AuditInfoLogger::is_enabled());
  EXPECT_EQ(0, S3AuditInfoLogger::save_msg("test_req_id", "{unit_tests=true}"));

  S3AuditInfoLogger::finalize();
  EXPECT_EQ(nullptr, S3AuditInfoLogger::audit_info_logger);
  EXPECT_EQ(false, S3AuditInfoLogger::audit_info_logger_enabled);
  EXPECT_EQ(false, S3AuditInfoLogger::is_enabled());
  EXPECT_EQ(1, S3AuditInfoLogger::save_msg("test_req_id", "{unit_tests=true}"));
}
