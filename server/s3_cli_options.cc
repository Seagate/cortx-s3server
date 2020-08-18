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

#include "s3_cli_options.h"
#include "s3_option.h"

DEFINE_string(s3config, "/opt/seagate/cortx/s3/conf/s3config.yaml",
              "S3 server config file");

DEFINE_string(s3layoutmap,
              "/opt/seagate/cortx/s3/conf/s3_obj_layout_mapping.yaml",
              "S3 Motr layout mapping file for different object sizes");

DEFINE_string(s3hostv4, "", "S3 server ipv4 bind address");
DEFINE_string(s3hostv6, "", "S3 server ipv6 bind address");
DEFINE_string(motrhttpapihost, "", "S3 server motr http bind address");
DEFINE_int32(s3port, 8081, "S3 server bind port");
DEFINE_int32(motrhttpapiport, 7081, "motr http server bind port");
DEFINE_string(s3pidfile, "/var/run/s3server.pid", "S3 server pid file");

DEFINE_string(audit_config,
              "/opt/seagate/cortx/s3/conf/s3server_audit_log.properties",
              "S3 Audit log Config Path");
DEFINE_string(audit_log_dir, "/var/log/seagate/s3", "S3 Audit log Directory");
DEFINE_string(s3loglevel, "INFO",
              "options: DEBUG | INFO | WARN | ERROR | FATAL");

DEFINE_bool(perfenable, false, "Enable performance log");
DEFINE_bool(reuseport, false, "Enable reusing s3 server port");
DEFINE_string(perflogfile, "/var/log/seagate/s3/perf.log",
              "Performance log path");

DEFINE_string(motrlocal, "localhost@tcp:12345:33:100", "Motr local address");
DEFINE_string(motrha, "MOTR_DEFAULT_HA_ADDR", "Motr ha address");
DEFINE_int32(motrlayoutid, 9, "For options please see the readme");
DEFINE_string(motrprofilefid, "<0x7000000000000001:0>", "Motr profile FID");
DEFINE_string(motrprocessfid, "<0x7200000000000000:0>", "Motr process FID");

DEFINE_string(authhost, "ipv4:127.0.0.1", "Auth server host");
DEFINE_int32(authport, 8095, "Auth server port");
DEFINE_bool(disable_auth, false, "Disable authentication");
DEFINE_bool(getoid, false, "Enable getoid in S3 request for testing");

DEFINE_bool(fake_authenticate, false, "Fake out authenticate");
DEFINE_bool(fake_authorization, false, "Fake out authorization");

DEFINE_bool(fake_motr_createobj, false, "Fake out motr create object");
DEFINE_bool(fake_motr_writeobj, false, "Fake out motr write object data");
DEFINE_bool(fake_motr_readobj, false, "Fake out motr read object data");
DEFINE_bool(fake_motr_deleteobj, false, "Fake out motr delete object");
DEFINE_bool(fake_motr_createidx, false, "Fake out motr create index");
DEFINE_bool(fake_motr_deleteidx, false, "Fake out motr delete index");
DEFINE_bool(fake_motr_getkv, false, "Fake out motr get key-val");
DEFINE_bool(fake_motr_putkv, false, "Fake out motr put key-val");
DEFINE_bool(fake_motr_deletekv, false, "Fake out motr delete key-val");
DEFINE_bool(fake_motr_redis_kvs, false,
            "Fake out motr kvs with redis in-memory storage");
DEFINE_bool(fault_injection, false, "Enable fault Injection flag for testing");
DEFINE_bool(loading_indicators, false, "Enable logging load indicators");
DEFINE_bool(addb, false, "Enable logging via ADDB motr subsystem");

DEFINE_string(statsd_host, "127.0.0.1", "StatsD daemon host");
DEFINE_int32(statsd_port, 8125, "StatsD daemon port");

int parse_and_load_config_options(int argc, char **argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, false);

  // Create the initial options object with default values.
  S3Option *option_instance = S3Option::get_instance();

  // load the configurations from config file.
  option_instance->set_option_file(FLAGS_s3config);
  bool force_override_from_config = true;
  if (!option_instance->load_all_sections(force_override_from_config)) {
    return -1;
  }

  // Override with options set on command line
  gflags::CommandLineFlagInfo flag_info;

  gflags::GetCommandLineFlagInfo("s3hostv4", &flag_info);
  if (!flag_info.is_default) {
    option_instance->set_cmdline_option(S3_OPTION_IPV4_BIND_ADDR,
                                        flag_info.current_value.c_str());
  }

  gflags::GetCommandLineFlagInfo("s3hostv6", &flag_info);
  if (!flag_info.is_default) {
    option_instance->set_cmdline_option(S3_OPTION_IPV6_BIND_ADDR,
                                        flag_info.current_value.c_str());
  }

  gflags::GetCommandLineFlagInfo("motrhttpapihost", &flag_info);
  if (!flag_info.is_default) {
    option_instance->set_cmdline_option(S3_OPTION_MOTR_BIND_ADDR,
                                        flag_info.current_value.c_str());
  }

  gflags::GetCommandLineFlagInfo("s3port", &flag_info);
  if (!flag_info.is_default) {
    option_instance->set_cmdline_option(S3_OPTION_BIND_PORT,
                                        flag_info.current_value.c_str());
  }

  gflags::GetCommandLineFlagInfo("motrhttpapiport", &flag_info);
  if (!flag_info.is_default) {
    option_instance->set_cmdline_option(S3_OPTION_MOTR_BIND_PORT,
                                        flag_info.current_value.c_str());
  }

  gflags::GetCommandLineFlagInfo("s3pidfile", &flag_info);
  if (!flag_info.is_default) {
    option_instance->set_cmdline_option(S3_OPTION_PIDFILE,
                                        flag_info.current_value.c_str());
  }

  gflags::GetCommandLineFlagInfo("authhost", &flag_info);
  if (!flag_info.is_default) {
    option_instance->set_cmdline_option(S3_OPTION_AUTH_IP_ADDR,
                                        flag_info.current_value.c_str());
  }

  gflags::GetCommandLineFlagInfo("authport", &flag_info);
  if (!flag_info.is_default) {
    option_instance->set_cmdline_option(S3_OPTION_AUTH_PORT,
                                        flag_info.current_value.c_str());
  }

  gflags::GetCommandLineFlagInfo("perflogfile", &flag_info);
  if (!flag_info.is_default) {
    option_instance->set_cmdline_option(S3_OPTION_PERF_LOG_FILE,
                                        flag_info.current_value.c_str());
  }

  gflags::GetCommandLineFlagInfo("log_dir", &flag_info);
  if (!flag_info.is_default) {
    option_instance->set_cmdline_option(S3_OPTION_LOG_DIR,
                                        flag_info.current_value.c_str());
  }

  gflags::GetCommandLineFlagInfo("s3loglevel", &flag_info);
  if (!flag_info.is_default) {
    option_instance->set_cmdline_option(S3_OPTION_LOG_MODE,
                                        flag_info.current_value.c_str());
  }

  gflags::GetCommandLineFlagInfo("audit_config", &flag_info);
  if (!flag_info.is_default) {
    option_instance->set_cmdline_option(S3_OPTION_AUDIT_CONFIG,
                                        flag_info.current_value.c_str());
  }

  gflags::GetCommandLineFlagInfo("audit_log_dir", &flag_info);
  if (!flag_info.is_default) {
    option_instance->set_cmdline_option(S3_OPTION_AUDIT_LOG_DIR,
                                        flag_info.current_value.c_str());
  }

  gflags::GetCommandLineFlagInfo("reuseport", &flag_info);
  if (!flag_info.is_default) {
    option_instance->set_cmdline_option(S3_OPTION_REUSEPORT,
                                        flag_info.current_value.c_str());
  }

  gflags::GetCommandLineFlagInfo("max_log_size", &flag_info);
  if (!flag_info.is_default) {
    option_instance->set_cmdline_option(S3_OPTION_LOG_FILE_MAX_SIZE,
                                        flag_info.current_value.c_str());
  }

  gflags::GetCommandLineFlagInfo("motrlocal", &flag_info);
  if (!flag_info.is_default) {
    option_instance->set_cmdline_option(S3_OPTION_MOTR_LOCAL_ADDR,
                                        flag_info.current_value.c_str());
  }

  gflags::GetCommandLineFlagInfo("motrha", &flag_info);
  if (!flag_info.is_default) {
    option_instance->set_cmdline_option(S3_OPTION_MOTR_HA_ADDR,
                                        flag_info.current_value.c_str());
  }

  gflags::GetCommandLineFlagInfo("motrlayoutid", &flag_info);
  if (!flag_info.is_default) {
    option_instance->set_cmdline_option(S3_MOTR_LAYOUT_ID,
                                        flag_info.current_value.c_str());
  }

  gflags::GetCommandLineFlagInfo("statsd_host", &flag_info);
  if (!flag_info.is_default) {
    option_instance->set_cmdline_option(S3_OPTION_STATSD_IP_ADDR,
                                        flag_info.current_value.c_str());
  }

  gflags::GetCommandLineFlagInfo("statsd_port", &flag_info);
  if (!flag_info.is_default) {
    option_instance->set_cmdline_option(S3_OPTION_STATSD_PORT,
                                        flag_info.current_value.c_str());
  }

  gflags::GetCommandLineFlagInfo("motrprofilefid", &flag_info);
  if (!flag_info.is_default) {
    option_instance->set_cmdline_option(S3_OPTION_MOTR_PROF,
                                        flag_info.current_value.c_str());
  }

  gflags::GetCommandLineFlagInfo("motrprocessfid", &flag_info);
  if (!flag_info.is_default) {
    option_instance->set_cmdline_option(S3_OPTION_MOTR_PROCESS_FID,
                                        flag_info.current_value.c_str());
  }
  return 0;
}

void finalize_cli_options() { gflags::ShutDownCommandLineFlags(); }
