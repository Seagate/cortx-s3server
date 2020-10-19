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
#include <inttypes.h>
#include <yaml-cpp/yaml.h>
#include <vector>
#include "s3_motr_layout.h"
#include "s3_log.h"
#include "s3_common_utilities.h"

S3Option* S3Option::option_instance = NULL;

bool S3Option::load_section(std::string section_name,
                            bool force_override_from_config = false) {
  YAML::Node root_node = YAML::LoadFile(option_file);
  if (root_node.IsNull()) {
    return false;  // File Not Found?
  }
  YAML::Node s3_option_node = root_node[section_name];
  if (s3_option_node.IsNull()) {
    return false;
  }
  if (force_override_from_config) {
    if (section_name == "S3_SERVER_CONFIG") {
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_DAEMON_WORKING_DIR");
      s3_daemon_dir = s3_option_node["S3_DAEMON_WORKING_DIR"].as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_DAEMON_DO_REDIRECTION");
      s3_daemon_redirect =
          s3_option_node["S3_DAEMON_DO_REDIRECTION"].as<unsigned short>();
      s3_enable_auth_ssl = s3_option_node["S3_ENABLE_AUTH_SSL"].as<bool>();
      s3_reuseport = s3_option_node["S3_REUSEPORT"].as<bool>();
      motr_http_reuseport = s3_option_node["S3_MOTR_HTTP_REUSEPORT"].as<bool>();
      s3_iam_cert_file = s3_option_node["S3_IAM_CERT_FILE"].as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_SERVER_CERT_FILE");
      s3server_ssl_cert_file =
          s3_option_node["S3_SERVER_CERT_FILE"].as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_SERVER_PEM_FILE");
      s3server_ssl_pem_file =
          s3_option_node["S3_SERVER_PEM_FILE"].as<std::string>();
      s3server_ssl_session_timeout_in_sec =
          s3_option_node["S3_SERVER_SSL_SESSION_TIMEOUT"].as<int>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_SERVER_BIND_PORT");
      s3_bind_port = s3_option_node["S3_SERVER_BIND_PORT"].as<unsigned short>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_SERVER_BIND_PORT");
      motr_http_bind_port =
          s3_option_node["S3_SERVER_MOTR_HTTP_BIND_PORT"].as<unsigned short>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node,
                               "S3_SERVER_SHUTDOWN_GRACE_PERIOD");
      s3_grace_period_sec = s3_option_node["S3_SERVER_SHUTDOWN_GRACE_PERIOD"]
                                .as<unsigned short>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_LOG_DIR");
      log_dir = s3_option_node["S3_LOG_DIR"].as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_LOG_MODE");
      log_level = s3_option_node["S3_LOG_MODE"].as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_AUDIT_LOG_DIR");
      audit_log_dir = s3_option_node["S3_AUDIT_LOG_DIR"].as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_AUDIT_LOG_CONFIG");
      audit_log_conf_file =
          s3_option_node["S3_AUDIT_LOG_CONFIG"].as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_AUDIT_LOG_FORMAT_TYPE");
      std::string audit_log_format_str =
          s3_option_node["S3_AUDIT_LOG_FORMAT_TYPE"].as<std::string>();
      if (!audit_log_format_str.compare("JSON")) {
        audit_log_format = AuditFormatType::JSON;
      } else if (!audit_log_format_str.compare("S3_FORMAT")) {
        audit_log_format = AuditFormatType::S3_FORMAT;
      } else {
        S3_OPTION_VALUE_INVALID_AND_RET("S3_AUDIT_LOG_FORMAT_TYPE",
                                        audit_log_format_str);
      }
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_AUDIT_LOGGER_POLICY");
      audit_logger_policy =
          s3_option_node["S3_AUDIT_LOGGER_POLICY"].as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_AUDIT_LOGGER_HOST");
      audit_logger_host =
          s3_option_node["S3_AUDIT_LOGGER_HOST"].as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_AUDIT_LOGGER_PORT");
      audit_logger_port = s3_option_node["S3_AUDIT_LOGGER_PORT"].as<int>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_AUDIT_LOGGER_RSYSLOG_MSGID");
      audit_logger_rsyslog_msgid =
          s3_option_node["S3_AUDIT_LOGGER_RSYSLOG_MSGID"].as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node,
                               "S3_AUDIT_LOGGER_KAFKA_WEB_PATH");
      audit_logger_kafka_web_path =
          s3_option_node["S3_AUDIT_LOGGER_KAFKA_WEB_PATH"].as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_LOG_FILE_MAX_SIZE");
      log_file_max_size_mb = s3_option_node["S3_LOG_FILE_MAX_SIZE"].as<int>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_LOG_ENABLE_BUFFERING");
      log_buffering_enable =
          s3_option_node["S3_LOG_ENABLE_BUFFERING"].as<bool>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_ENABLE_MURMURHASH_OID");
      s3_enable_murmurhash_oid =
          s3_option_node["S3_ENABLE_MURMURHASH_OID"].as<bool>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_LOG_FLUSH_FREQUENCY");
      log_flush_frequency_sec =
          s3_option_node["S3_LOG_FLUSH_FREQUENCY"].as<int>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_SERVER_IPV4_BIND_ADDR");
      s3_ipv4_bind_addr =
          s3_option_node["S3_SERVER_IPV4_BIND_ADDR"].as<std::string>();
      // '~' means empty or null
      if (S3CommonUtilities::is_yaml_value_null(s3_ipv4_bind_addr)) {
        s3_ipv4_bind_addr = "";
      }
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_SERVER_IPV6_BIND_ADDR");
      s3_ipv6_bind_addr =
          s3_option_node["S3_SERVER_IPV6_BIND_ADDR"].as<std::string>();
      // '~' means empty or null
      if (S3CommonUtilities::is_yaml_value_null(s3_ipv6_bind_addr)) {
        s3_ipv6_bind_addr = "";
      }
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_SERVER_MOTR_HTTP_BIND_ADDR");
      motr_http_bind_addr =
          s3_option_node["S3_SERVER_MOTR_HTTP_BIND_ADDR"].as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_ENABLE_PERF");
      perf_enabled = s3_option_node["S3_ENABLE_PERF"].as<unsigned short>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_SERVER_SSL_ENABLE");
      s3server_ssl_enabled = s3_option_node["S3_SERVER_SSL_ENABLE"].as<bool>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node,
                               "S3_SERVER_ENABLE_OBJECT_LEAK_TRACKING");
      s3server_objectleak_tracking_enabled =
          s3_option_node["S3_SERVER_ENABLE_OBJECT_LEAK_TRACKING"].as<bool>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_READ_AHEAD_MULTIPLE");
      read_ahead_multiple = s3_option_node["S3_READ_AHEAD_MULTIPLE"].as<int>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_SERVER_DEFAULT_ENDPOINT");
      s3_default_endpoint =
          s3_option_node["S3_SERVER_DEFAULT_ENDPOINT"].as<std::string>();
      s3_region_endpoints.clear();
      for (unsigned short i = 0;
           i < s3_option_node["S3_SERVER_REGION_ENDPOINTS"].size(); ++i) {
        s3_region_endpoints.insert(
            s3_option_node["S3_SERVER_REGION_ENDPOINTS"][i].as<std::string>());
      }
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_MAX_RETRY_COUNT");
      max_retry_count =
          s3_option_node["S3_MAX_RETRY_COUNT"].as<unsigned short>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_AUDIT_MAX_RETRY_COUNT");
      max_audit_retry_count =
          s3_option_node["S3_AUDIT_MAX_RETRY_COUNT"].as<unsigned short>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_RETRY_INTERVAL_MILLISEC");
      retry_interval_millisec =
          s3_option_node["S3_RETRY_INTERVAL_MILLISEC"].as<unsigned short>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node,
                               "S3_CLIENT_REQ_READ_TIMEOUT_SECS");
      s3_client_req_read_timeout_secs =
          s3_option_node["S3_CLIENT_REQ_READ_TIMEOUT_SECS"]
              .as<unsigned short>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_ENABLE_STATS");
      stats_enable = s3_option_node["S3_ENABLE_STATS"].as<bool>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_STATSD_PORT");
      statsd_port = s3_option_node["S3_STATSD_PORT"].as<unsigned short>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_STATSD_IP_ADDR");
      statsd_ip_addr = s3_option_node["S3_STATSD_IP_ADDR"].as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_STATSD_MAX_SEND_RETRY");
      statsd_max_send_retry =
          s3_option_node["S3_STATSD_MAX_SEND_RETRY"].as<unsigned short>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_STATS_ALLOWLIST_FILENAME");
      stats_allowlist_filename =
          s3_option_node["S3_STATS_ALLOWLIST_FILENAME"].as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node,
                               "S3_PERF_STATS_INOUT_BYTES_INTERVAL_MSEC");
      perf_stats_inout_bytes_interval_msec =
          s3_option_node["S3_PERF_STATS_INOUT_BYTES_INTERVAL_MSEC"]
              .as<uint32_t>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_REDIS_SERVER_ADDRESS");
      redis_srv_addr =
          s3_option_node["S3_REDIS_SERVER_ADDRESS"].as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_REDIS_SERVER_PORT");
      redis_srv_port =
          s3_option_node["S3_REDIS_SERVER_PORT"].as<unsigned short>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node,
                               "S3_SERVER_MOTR_ETIMEDOUT_MAX_THRESHOLD");
      motr_etimedout_max_threshold =
          s3_option_node["S3_SERVER_MOTR_ETIMEDOUT_MAX_THRESHOLD"].as<uint>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node,
                               "S3_SERVER_MOTR_ETIMEDOUT_WINDOW_SEC");
      motr_etimedout_window_sec =
          s3_option_node["S3_SERVER_MOTR_ETIMEDOUT_WINDOW_SEC"].as<uint>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_SERVER_ENABLE_ADDB_DUMP");
      FLAGS_addb = s3_option_node["S3_SERVER_ENABLE_ADDB_DUMP"].as<bool>();
    } else if (section_name == "S3_AUTH_CONFIG") {
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_AUTH_PORT");
      auth_port = s3_option_node["S3_AUTH_PORT"].as<unsigned short>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_AUTH_IP_ADDR");
      auth_ip_addr = s3_option_node["S3_AUTH_IP_ADDR"].as<std::string>();
    } else if (section_name == "S3_MOTR_CONFIG") {
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_MOTR_LOCAL_ADDR");
      motr_local_addr = s3_option_node["S3_MOTR_LOCAL_ADDR"].as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_MOTR_HA_ADDR");
      motr_ha_addr = s3_option_node["S3_MOTR_HA_ADDR"].as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_MOTR_PROF");
      motr_profile = s3_option_node["S3_MOTR_PROF"].as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_MOTR_LAYOUT_ID");
      motr_layout_id = s3_option_node["S3_MOTR_LAYOUT_ID"].as<unsigned short>();

      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_UNIT_SIZES_FOR_MEMORY_POOL");
      YAML::Node unit_sizes = s3_option_node["S3_UNIT_SIZES_FOR_MEMORY_POOL"];
      for (std::size_t i = 0; i < unit_sizes.size(); i++) {
        motr_unit_sizes_for_mem_pool.push_back(unit_sizes[i].as<int>());
      }

      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_MOTR_MAX_UNITS_PER_REQUEST");
      motr_units_per_request =
          s3_option_node["S3_MOTR_MAX_UNITS_PER_REQUEST"].as<unsigned short>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_MOTR_MAX_IDX_FETCH_COUNT");
      motr_idx_fetch_count =
          s3_option_node["S3_MOTR_MAX_IDX_FETCH_COUNT"].as<int>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_MOTR_IS_OOSTORE");
      motr_is_oostore = s3_option_node["S3_MOTR_IS_OOSTORE"].as<bool>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_MOTR_IS_READ_VERIFY");
      motr_is_read_verify = s3_option_node["S3_MOTR_IS_READ_VERIFY"].as<bool>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_MOTR_TM_RECV_QUEUE_MIN_LEN");
      motr_tm_recv_queue_min_len =
          s3_option_node["S3_MOTR_TM_RECV_QUEUE_MIN_LEN"].as<unsigned int>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_MOTR_MAX_RPC_MSG_SIZE");
      motr_max_rpc_msg_size =
          s3_option_node["S3_MOTR_MAX_RPC_MSG_SIZE"].as<unsigned int>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_MOTR_PROCESS_FID");
      motr_process_fid =
          s3_option_node["S3_MOTR_PROCESS_FID"].as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_MOTR_IDX_SERVICE_ID");
      motr_idx_service_id = s3_option_node["S3_MOTR_IDX_SERVICE_ID"].as<int>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_MOTR_CASS_CLUSTER_EP");
      motr_cass_cluster_ep =
          s3_option_node["S3_MOTR_CASS_CLUSTER_EP"].as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_MOTR_CASS_KEYSPACE");
      motr_cass_keyspace =
          s3_option_node["S3_MOTR_CASS_KEYSPACE"].as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node,
                               "S3_MOTR_CASS_MAX_COL_FAMILY_NUM");
      motr_cass_max_column_family_num =
          s3_option_node["S3_MOTR_CASS_MAX_COL_FAMILY_NUM"].as<int>();

      S3_OPTION_ASSERT_AND_RET(s3_option_node,
                               "S3_MOTR_READ_MEMPOOL_ZERO_BUFFER");
      motr_read_mempool_zeroed_buffer =
          s3_option_node["S3_MOTR_READ_MEMPOOL_ZERO_BUFFER"].as<bool>();

      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_MOTR_OPERATION_WAIT_PERIOD");
      motr_op_wait_period =
          s3_option_node["S3_MOTR_OPERATION_WAIT_PERIOD"].as<unsigned int>();

      std::string motr_read_pool_initial_buffer_count_str;
      std::string motr_read_pool_expandable_count_str;
      std::string motr_read_pool_max_threshold_str;
      S3_OPTION_ASSERT_AND_RET(s3_option_node,
                               "S3_MOTR_READ_POOL_INITIAL_BUFFER_COUNT");
      motr_read_pool_initial_buffer_count_str =
          s3_option_node["S3_MOTR_READ_POOL_INITIAL_BUFFER_COUNT"]
              .as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node,
                               "S3_MOTR_READ_POOL_EXPANDABLE_COUNT");
      motr_read_pool_expandable_count_str =
          s3_option_node["S3_MOTR_READ_POOL_EXPANDABLE_COUNT"]
              .as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node,
                               "S3_MOTR_READ_POOL_MAX_THRESHOLD");
      motr_read_pool_max_threshold_str =
          s3_option_node["S3_MOTR_READ_POOL_MAX_THRESHOLD"].as<std::string>();
      sscanf(motr_read_pool_initial_buffer_count_str.c_str(), "%zu",
             &motr_read_pool_initial_buffer_count);
      sscanf(motr_read_pool_expandable_count_str.c_str(), "%zu",
             &motr_read_pool_expandable_count);
      sscanf(motr_read_pool_max_threshold_str.c_str(), "%zu",
             &motr_read_pool_max_threshold);

    } else if (section_name == "S3_THIRDPARTY_CONFIG") {
      std::string libevent_pool_initial_size_str;
      std::string libevent_pool_expandable_size_str;
      std::string libevent_pool_max_threshold_str;
      std::string libevent_pool_buffer_size_str;
      std::string libevent_max_read_size_str;

      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_LIBEVENT_POOL_INITIAL_SIZE");
      libevent_pool_initial_size_str =
          s3_option_node["S3_LIBEVENT_POOL_INITIAL_SIZE"].as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node,
                               "S3_LIBEVENT_POOL_EXPANDABLE_SIZE");
      libevent_pool_expandable_size_str =
          s3_option_node["S3_LIBEVENT_POOL_EXPANDABLE_SIZE"].as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node,
                               "S3_LIBEVENT_POOL_MAX_THRESHOLD");
      libevent_pool_max_threshold_str =
          s3_option_node["S3_LIBEVENT_POOL_MAX_THRESHOLD"].as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_LIBEVENT_POOL_BUFFER_SIZE");
      libevent_pool_buffer_size_str =
          s3_option_node["S3_LIBEVENT_POOL_BUFFER_SIZE"].as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_LIBEVENT_MAX_READ_SIZE");
      libevent_max_read_size_str =
          s3_option_node["S3_LIBEVENT_MAX_READ_SIZE"].as<std::string>();
      sscanf(libevent_pool_initial_size_str.c_str(), "%zu",
             &libevent_pool_initial_size);
      sscanf(libevent_pool_expandable_size_str.c_str(), "%zu",
             &libevent_pool_expandable_size);
      sscanf(libevent_pool_max_threshold_str.c_str(), "%zu",
             &libevent_pool_max_threshold);
      sscanf(libevent_pool_buffer_size_str.c_str(), "%zu",
             &libevent_pool_buffer_size);
      sscanf(libevent_max_read_size_str.c_str(), "%zu",
             &libevent_max_read_size);
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_LIBEVENT_POOL_RESERVE_SIZE");
      libevent_pool_reserve_size =
          s3_option_node["S3_LIBEVENT_POOL_RESERVE_SIZE"].as<size_t>();

      S3_OPTION_ASSERT_AND_RET(s3_option_node,
                               "S3_LIBEVENT_MEMPOOL_ZERO_BUFFER");
      libevent_mempool_zeroed_buffer =
          s3_option_node["S3_LIBEVENT_MEMPOOL_ZERO_BUFFER"].as<bool>();

      S3_OPTION_ASSERT_AND_RET(s3_option_node,
                               "S3_LIBEVENT_POOL_RESERVE_PERCENT");
      libevent_pool_reserve_percent =
          s3_option_node["S3_LIBEVENT_POOL_RESERVE_PERCENT"].as<unsigned>();
    } else if (section_name == "S3_VERSION_CONFIG") {
      s3_version = s3_option_node["VERSION"].as<std::string>();
    }
  } else {
    if (section_name == "S3_SERVER_CONFIG") {
      if (!(cmd_opt_flag & S3_OPTION_BIND_PORT)) {
        S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_SERVER_BIND_PORT");
        s3_bind_port =
            s3_option_node["S3_SERVER_BIND_PORT"].as<unsigned short>();
      }
      if (!(cmd_opt_flag & S3_OPTION_MOTR_BIND_PORT)) {
        S3_OPTION_ASSERT_AND_RET(s3_option_node,
                                 "S3_SERVER_MOTR_HTTP_BIND_PORT");
        motr_http_bind_port = s3_option_node["S3_SERVER_MOTR_HTTP_BIND_PORT"]
                                  .as<unsigned short>();
      }
      if (!(cmd_opt_flag & S3_OPTION_LOG_DIR)) {
        S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_LOG_DIR");
        log_dir = s3_option_node["S3_LOG_DIR"].as<std::string>();
      }
      if (!(cmd_opt_flag & S3_OPTION_PERF_LOG_FILE)) {
        S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_PERF_LOG_FILENAME");
        perf_log_file =
            s3_option_node["S3_PERF_LOG_FILENAME"].as<std::string>();
      }
      if (!(cmd_opt_flag & S3_OPTION_LOG_MODE)) {
        S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_LOG_MODE");
        log_level = s3_option_node["S3_LOG_MODE"].as<std::string>();
      }
      if (!(cmd_opt_flag & S3_OPTION_AUDIT_LOG_DIR)) {
        S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_AUDIT_LOG_DIR");
        audit_log_dir = s3_option_node["S3_AUDIT_LOG_DIR"].as<std::string>();
      }
      if (!(cmd_opt_flag & S3_OPTION_AUDIT_CONFIG)) {
        S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_AUDIT_LOG_CONFIG");
        audit_log_conf_file =
            s3_option_node["S3_AUDIT_LOG_CONFIG"].as<std::string>();
      }
      if (!(cmd_opt_flag & S3_OPTION_LOG_FILE_MAX_SIZE)) {
        S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_LOG_FILE_MAX_SIZE");
        log_file_max_size_mb = s3_option_node["S3_LOG_FILE_MAX_SIZE"].as<int>();
      }
      if (!(cmd_opt_flag & S3_OPTION_IPV4_BIND_ADDR)) {
        S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_SERVER_IPV4_BIND_ADDR");
        s3_ipv4_bind_addr =
            s3_option_node["S3_SERVER_IPV4_BIND_ADDR"].as<std::string>();
      }
      if (!(cmd_opt_flag & S3_OPTION_IPV6_BIND_ADDR)) {
        S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_SERVER_IPV6_BIND_ADDR");
        s3_ipv6_bind_addr =
            s3_option_node["S3_SERVER_IPV6_BIND_ADDR"].as<std::string>();
      }
      if (!(cmd_opt_flag & S3_OPTION_MOTR_BIND_ADDR)) {
        S3_OPTION_ASSERT_AND_RET(s3_option_node,
                                 "S3_SERVER_MOTR_HTTP_BIND_ADDR");
        motr_http_bind_addr =
            s3_option_node["S3_SERVER_MOTR_HTTP_BIND_ADDR"].as<std::string>();
      }
      if (!(cmd_opt_flag & S3_OPTION_STATSD_PORT)) {
        S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_STATSD_PORT");
        statsd_port = s3_option_node["S3_STATSD_PORT"].as<unsigned short>();
      }
      if (!(cmd_opt_flag & S3_OPTION_STATSD_IP_ADDR)) {
        S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_STATSD_IP_ADDR");
        statsd_ip_addr = s3_option_node["S3_STATSD_IP_ADDR"].as<std::string>();
      }
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_DAEMON_WORKING_DIR");
      s3_daemon_dir = s3_option_node["S3_DAEMON_WORKING_DIR"].as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_DAEMON_DO_REDIRECTION");
      s3_daemon_redirect =
          s3_option_node["S3_DAEMON_DO_REDIRECTION"].as<unsigned short>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node,
                               "S3_SERVER_SHUTDOWN_GRACE_PERIOD");
      s3_grace_period_sec = s3_option_node["S3_SERVER_SHUTDOWN_GRACE_PERIOD"]
                                .as<unsigned short>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_SERVER_DEFAULT_ENDPOINT");
      s3_default_endpoint =
          s3_option_node["S3_SERVER_DEFAULT_ENDPOINT"].as<std::string>();
      s3_region_endpoints.clear();
      for (unsigned short i = 0;
           i < s3_option_node["S3_SERVER_REGION_ENDPOINTS"].size(); ++i) {
        s3_region_endpoints.insert(
            s3_option_node["S3_SERVER_REGION_ENDPOINTS"][i].as<std::string>());
      }
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_ENABLE_AUTH_SSL");
      s3_enable_auth_ssl = s3_option_node["S3_ENABLE_AUTH_SSL"].as<bool>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_REUSEPORT");
      s3_reuseport = s3_option_node["S3_REUSEPORT"].as<bool>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_MOTR_HTTP_REUSEPORT");
      motr_http_reuseport = s3_option_node["S3_MOTR_HTTP_REUSEPORT"].as<bool>();
      s3_iam_cert_file = s3_option_node["S3_IAM_CERT_FILE"].as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_SERVER_CERT_FILE");
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_SERVER_PEM_FILE");
      s3server_ssl_cert_file =
          s3_option_node["S3_SERVER_CERT_FILE"].as<std::string>();
      s3server_ssl_pem_file =
          s3_option_node["S3_SERVER_PEM_FILE"].as<std::string>();
      s3server_ssl_session_timeout_in_sec =
          s3_option_node["S3_SERVER_SSL_SESSION_TIMEOUT"].as<int>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_ENABLE_PERF");
      perf_enabled = s3_option_node["S3_ENABLE_PERF"].as<unsigned short>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_SERVER_SSL_ENABLE");
      s3server_ssl_enabled = s3_option_node["S3_SERVER_SSL_ENABLE"].as<bool>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node,
                               "S3_SERVER_ENABLE_OBJECT_LEAK_TRACKING");
      s3server_objectleak_tracking_enabled =
          s3_option_node["S3_SERVER_ENABLE_OBJECT_LEAK_TRACKING"].as<bool>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_READ_AHEAD_MULTIPLE");
      read_ahead_multiple = s3_option_node["S3_READ_AHEAD_MULTIPLE"].as<int>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_MAX_RETRY_COUNT");
      max_retry_count =
          s3_option_node["S3_MAX_RETRY_COUNT"].as<unsigned short>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_AUDIT_MAX_RETRY_COUNT");
      max_audit_retry_count =
          s3_option_node["S3_AUDIT_MAX_RETRY_COUNT"].as<unsigned short>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_RETRY_INTERVAL_MILLISEC");
      retry_interval_millisec =
          s3_option_node["S3_RETRY_INTERVAL_MILLISEC"].as<unsigned short>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node,
                               "S3_CLIENT_REQ_READ_TIMEOUT_SECS");
      s3_client_req_read_timeout_secs =
          s3_option_node["S3_CLIENT_REQ_READ_TIMEOUT_SECS"]
              .as<unsigned short>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_LOG_ENABLE_BUFFERING");
      log_buffering_enable =
          s3_option_node["S3_LOG_ENABLE_BUFFERING"].as<bool>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_ENABLE_MURMURHASH_OID");
      s3_enable_murmurhash_oid =
          s3_option_node["S3_ENABLE_MURMURHASH_OID"].as<bool>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_LOG_FLUSH_FREQUENCY");
      log_flush_frequency_sec =
          s3_option_node["S3_LOG_FLUSH_FREQUENCY"].as<int>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_ENABLE_STATS");
      stats_enable = s3_option_node["S3_ENABLE_STATS"].as<bool>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_STATSD_MAX_SEND_RETRY");
      statsd_max_send_retry =
          s3_option_node["S3_STATSD_MAX_SEND_RETRY"].as<unsigned short>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_STATS_ALLOWLIST_FILENAME");
      stats_allowlist_filename =
          s3_option_node["S3_STATS_ALLOWLIST_FILENAME"].as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_AUDIT_LOGGER_POLICY");
      audit_logger_policy =
          s3_option_node["S3_AUDIT_LOGGER_POLICY"].as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_AUDIT_LOGGER_HOST");
      audit_logger_host =
          s3_option_node["S3_AUDIT_LOGGER_HOST"].as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_AUDIT_LOGGER_PORT");
      audit_logger_port = s3_option_node["S3_AUDIT_LOGGER_PORT"].as<int>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_AUDIT_LOGGER_RSYSLOG_MSGID");
      audit_logger_rsyslog_msgid =
          s3_option_node["S3_AUDIT_LOGGER_RSYSLOG_MSGID"].as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node,
                               "S3_AUDIT_LOGGER_KAFKA_WEB_PATH");
      audit_logger_kafka_web_path =
          s3_option_node["S3_AUDIT_LOGGER_KAFKA_WEB_PATH"].as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_REDIS_SERVER_ADDRESS");
      redis_srv_addr =
          s3_option_node["S3_REDIS_SERVER_ADDRESS"].as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_REDIS_SERVER_PORT");
      redis_srv_port =
          s3_option_node["S3_REDIS_SERVER_PORT"].as<unsigned short>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node,
                               "S3_SERVER_MOTR_ETIMEDOUT_MAX_THRESHOLD");
      motr_etimedout_max_threshold =
          s3_option_node["S3_SERVER_MOTR_ETIMEDOUT_MAX_THRESHOLD"]
              .as<unsigned>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node,
                               "S3_SERVER_MOTR_ETIMEDOUT_WINDOW_SEC");
      motr_etimedout_window_sec =
          s3_option_node["S3_SERVER_MOTR_ETIMEDOUT_WINDOW_SEC"].as<unsigned>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_SERVER_ENABLE_ADDB_DUMP");
      FLAGS_addb = s3_option_node["S3_SERVER_ENABLE_ADDB_DUMP"].as<bool>();
    } else if (section_name == "S3_AUTH_CONFIG") {
      if (!(cmd_opt_flag & S3_OPTION_AUTH_PORT)) {
        S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_AUTH_PORT");
        auth_port = s3_option_node["S3_AUTH_PORT"].as<unsigned short>();
      }
      if (!(cmd_opt_flag & S3_OPTION_AUTH_IP_ADDR)) {
        S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_AUTH_IP_ADDR");
        auth_ip_addr = s3_option_node["S3_AUTH_IP_ADDR"].as<std::string>();
      }
    } else if (section_name == "S3_MOTR_CONFIG") {
      if (!(cmd_opt_flag & S3_OPTION_MOTR_LOCAL_ADDR)) {
        S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_MOTR_LOCAL_ADDR");
        motr_local_addr =
            s3_option_node["S3_MOTR_LOCAL_ADDR"].as<std::string>();
      }
      if (!(cmd_opt_flag & S3_OPTION_MOTR_HA_ADDR)) {
        S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_MOTR_HA_ADDR");
        motr_ha_addr = s3_option_node["S3_MOTR_HA_ADDR"].as<std::string>();
      }
      if (!(cmd_opt_flag & S3_MOTR_LAYOUT_ID)) {
        S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_MOTR_LAYOUT_ID");
        motr_layout_id =
            s3_option_node["S3_MOTR_LAYOUT_ID"].as<unsigned short>();
      }
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_MOTR_PROF");
      motr_profile = s3_option_node["S3_MOTR_PROF"].as<std::string>();

      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_UNIT_SIZES_FOR_MEMORY_POOL");
      YAML::Node unit_sizes = s3_option_node["S3_UNIT_SIZES_FOR_MEMORY_POOL"];
      for (std::size_t i = 0; i < unit_sizes.size(); i++) {
        motr_unit_sizes_for_mem_pool.push_back(unit_sizes[i].as<int>());
      }
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_MOTR_MAX_UNITS_PER_REQUEST");
      motr_units_per_request =
          s3_option_node["S3_MOTR_MAX_UNITS_PER_REQUEST"].as<unsigned short>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_MOTR_MAX_IDX_FETCH_COUNT");
      motr_idx_fetch_count =
          s3_option_node["S3_MOTR_MAX_IDX_FETCH_COUNT"].as<int>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_MOTR_IS_OOSTORE");
      motr_is_oostore = s3_option_node["S3_MOTR_IS_OOSTORE"].as<bool>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_MOTR_IS_READ_VERIFY");
      motr_is_read_verify = s3_option_node["S3_MOTR_IS_READ_VERIFY"].as<bool>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_MOTR_TM_RECV_QUEUE_MIN_LEN");
      motr_tm_recv_queue_min_len =
          s3_option_node["S3_MOTR_TM_RECV_QUEUE_MIN_LEN"].as<unsigned int>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_MOTR_MAX_RPC_MSG_SIZE");
      motr_max_rpc_msg_size =
          s3_option_node["S3_MOTR_MAX_RPC_MSG_SIZE"].as<unsigned int>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_MOTR_PROCESS_FID");
      motr_process_fid =
          s3_option_node["S3_MOTR_PROCESS_FID"].as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_MOTR_IDX_SERVICE_ID");
      motr_idx_service_id = s3_option_node["S3_MOTR_IDX_SERVICE_ID"].as<int>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_MOTR_CASS_CLUSTER_EP");
      motr_cass_cluster_ep =
          s3_option_node["S3_MOTR_CASS_CLUSTER_EP"].as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_MOTR_CASS_KEYSPACE");
      motr_cass_keyspace =
          s3_option_node["S3_MOTR_CASS_KEYSPACE"].as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node,
                               "S3_MOTR_CASS_MAX_COL_FAMILY_NUM");
      motr_cass_max_column_family_num =
          s3_option_node["S3_MOTR_CASS_MAX_COL_FAMILY_NUM"].as<int>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_MOTR_OPERATION_WAIT_PERIOD");
      motr_op_wait_period =
          s3_option_node["S3_MOTR_OPERATION_WAIT_PERIOD"].as<unsigned>();

      std::string motr_read_pool_initial_buffer_count_str;
      std::string motr_read_pool_expandable_count_str;
      std::string motr_read_pool_max_threshold_str;
      S3_OPTION_ASSERT_AND_RET(s3_option_node,
                               "S3_MOTR_READ_POOL_INITIAL_BUFFER_COUNT");
      motr_read_pool_initial_buffer_count_str =
          s3_option_node["S3_MOTR_READ_POOL_INITIAL_BUFFER_COUNT"]
              .as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node,
                               "S3_MOTR_READ_POOL_EXPANDABLE_COUNT");
      motr_read_pool_expandable_count_str =
          s3_option_node["S3_MOTR_READ_POOL_EXPANDABLE_COUNT"]
              .as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node,
                               "S3_MOTR_READ_POOL_MAX_THRESHOLD");
      motr_read_pool_max_threshold_str =
          s3_option_node["S3_MOTR_READ_POOL_MAX_THRESHOLD"].as<std::string>();
      sscanf(motr_read_pool_initial_buffer_count_str.c_str(), "%zu",
             &motr_read_pool_initial_buffer_count);
      sscanf(motr_read_pool_expandable_count_str.c_str(), "%zu",
             &motr_read_pool_expandable_count);
      sscanf(motr_read_pool_max_threshold_str.c_str(), "%zu",
             &motr_read_pool_max_threshold);

    } else if (section_name == "S3_THIRDPARTY_CONFIG") {
      std::string libevent_pool_initial_size_str;
      std::string libevent_pool_expandable_size_str;
      std::string libevent_pool_max_threshold_str;
      std::string libevent_pool_buffer_size_str;
      std::string libevent_max_read_size_str;

      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_LIBEVENT_POOL_INITIAL_SIZE");
      libevent_pool_initial_size_str =
          s3_option_node["S3_LIBEVENT_POOL_INITIAL_SIZE"].as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node,
                               "S3_LIBEVENT_POOL_EXPANDABLE_SIZE");
      libevent_pool_expandable_size_str =
          s3_option_node["S3_LIBEVENT_POOL_EXPANDABLE_SIZE"].as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node,
                               "S3_LIBEVENT_POOL_MAX_THRESHOLD");
      libevent_pool_max_threshold_str =
          s3_option_node["S3_LIBEVENT_POOL_MAX_THRESHOLD"].as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_LIBEVENT_POOL_BUFFER_SIZE");
      libevent_pool_buffer_size_str =
          s3_option_node["S3_LIBEVENT_POOL_BUFFER_SIZE"].as<std::string>();
      S3_OPTION_ASSERT_AND_RET(s3_option_node, "S3_LIBEVENT_MAX_READ_SIZE");
      libevent_max_read_size_str =
          s3_option_node["S3_LIBEVENT_MAX_READ_SIZE"].as<std::string>();
      sscanf(libevent_pool_initial_size_str.c_str(), "%zu",
             &libevent_pool_initial_size);
      sscanf(libevent_pool_expandable_size_str.c_str(), "%zu",
             &libevent_pool_expandable_size);
      sscanf(libevent_pool_max_threshold_str.c_str(), "%zu",
             &libevent_pool_max_threshold);
      sscanf(libevent_pool_buffer_size_str.c_str(), "%zu",
             &libevent_pool_buffer_size);
      sscanf(libevent_max_read_size_str.c_str(), "%zu",
             &libevent_max_read_size);
    } else if (section_name == "S3_VERSION_CONFIG") {
      s3_version = s3_option_node["VERSION"].as<std::string>();
    }
  }
  return true;
}

bool S3Option::load_all_sections(bool force_override_from_config = false) {
  std::string s3_section;
  try {
    YAML::Node root_node = YAML::LoadFile(option_file);
    if (root_node.IsNull()) {
      return false;  // File Not Found?
    }
    for (unsigned short i = 0; i < root_node["S3Config_Sections"].size(); ++i) {
      if (!load_section(root_node["S3Config_Sections"][i].as<std::string>(),
                        force_override_from_config)) {
        return false;
      }
    }
  }
  catch (const YAML::RepresentationException& e) {
    fprintf(stderr, "%s:%d:YAML::RepresentationException caught: %s\n",
            __FILE__, __LINE__, e.what());
    fprintf(stderr, "%s:%d:Yaml file %s is incorrect\n", __FILE__, __LINE__,
            option_file.c_str());
    return false;
  }
  catch (const YAML::ParserException& e) {
    fprintf(stderr, "%s:%d:YAML::ParserException caught: %s\n", __FILE__,
            __LINE__, e.what());
    fprintf(stderr, "%s:%d:Parsing Error in yaml file %s\n", __FILE__, __LINE__,
            option_file.c_str());
    return false;
  }
  catch (const YAML::EmitterException& e) {
    fprintf(stderr, "%s:%d:YAML::EmitterException caught: %s\n", __FILE__,
            __LINE__, e.what());
    return false;
  }
  catch (YAML::Exception& e) {
    fprintf(stderr, "%s:%d:YAML::Exception caught: %s\n", __FILE__, __LINE__,
            e.what());
    return false;
  }
  return true;
}

void S3Option::set_cmdline_option(int option_flag, const char* optarg) {
  if (option_flag & S3_OPTION_IPV4_BIND_ADDR) {
    s3_ipv4_bind_addr = optarg;
  } else if (option_flag & S3_OPTION_IPV6_BIND_ADDR) {
    s3_ipv6_bind_addr = optarg;
  } else if (option_flag & S3_OPTION_MOTR_BIND_ADDR) {
    motr_http_bind_addr = optarg;
  } else if (option_flag & S3_OPTION_BIND_PORT) {
    s3_bind_port = atoi(optarg);
  } else if (option_flag & S3_OPTION_MOTR_BIND_PORT) {
    motr_http_bind_port = atoi(optarg);
  } else if (option_flag & S3_OPTION_MOTR_LOCAL_ADDR) {
    motr_local_addr = optarg;
  } else if (option_flag & S3_OPTION_MOTR_HA_ADDR) {
    motr_ha_addr = optarg;
  } else if (option_flag & S3_OPTION_AUTH_IP_ADDR) {
    auth_ip_addr = optarg;
  } else if (option_flag & S3_OPTION_AUTH_PORT) {
    auth_port = atoi(optarg);
  } else if (option_flag & S3_MOTR_LAYOUT_ID) {
    motr_layout_id = atoi(optarg);
  } else if (option_flag & S3_OPTION_LOG_DIR) {
    log_dir = optarg;
  } else if (option_flag & S3_OPTION_PERF_LOG_FILE) {
    perf_log_file = optarg;
  } else if (option_flag & S3_OPTION_LOG_MODE) {
    log_level = optarg;
  } else if (option_flag & S3_OPTION_AUDIT_LOG_DIR) {
    audit_log_dir = optarg;
  } else if (option_flag & S3_OPTION_AUDIT_CONFIG) {
    audit_log_conf_file = optarg;
  } else if (option_flag & S3_OPTION_LOG_FILE_MAX_SIZE) {
    log_file_max_size_mb = atoi(optarg);
  } else if (option_flag & S3_OPTION_PIDFILE) {
    s3_pidfile = optarg;
  } else if (option_flag & S3_OPTION_STATSD_IP_ADDR) {
    statsd_ip_addr = optarg;
  } else if (option_flag & S3_OPTION_STATSD_PORT) {
    statsd_port = atoi(optarg);
  } else if (option_flag & S3_OPTION_MOTR_PROF) {
    motr_profile = optarg;
  } else if (option_flag & S3_OPTION_MOTR_PROCESS_FID) {
    motr_process_fid = optarg;
  } else if (option_flag & S3_OPTION_REUSEPORT) {
    if (strcmp(optarg, "true") == 0) {
      s3_reuseport = true;
    } else {
      s3_reuseport = false;
    }
  } else if (option_flag & S3_OPTION_MOTR_HTTP_REUSEPORT) {
    if (strcmp(optarg, "true") == 0) {
      motr_http_reuseport = true;
    } else {
      motr_http_reuseport = false;
    }
  }
  cmd_opt_flag |= option_flag;
  return;
}

int S3Option::get_cmd_opt_flag() { return cmd_opt_flag; }

void S3Option::dump_options() {
  s3_log(S3_LOG_INFO, "", "option_file = %s\n", option_file.c_str());
  s3_log(S3_LOG_INFO, "", "layout_recommendation_file = %s\n",
         layout_recommendation_file.c_str());
  s3_log(S3_LOG_INFO, "", "S3_DAEMON_WORKING_DIR = %s\n",
         s3_daemon_dir.c_str());
  s3_log(S3_LOG_INFO, "", "S3_DAEMON_DO_REDIRECTION = %d\n",
         s3_daemon_redirect);
  s3_log(S3_LOG_INFO, "", "S3_PIDFILENAME = %s\n", s3_pidfile.c_str());
  s3_log(S3_LOG_INFO, "", "S3_LOG_DIR = %s\n", log_dir.c_str());
  s3_log(S3_LOG_INFO, "", "S3_LOG_MODE = %s\n", log_level.c_str());
  s3_log(S3_LOG_INFO, "", "S3_LOG_FILE_MAX_SIZE = %d\n", log_file_max_size_mb);
  s3_log(S3_LOG_INFO, "", "S3_LOG_ENABLE_BUFFERING = %s\n",
         (log_buffering_enable ? "true" : "false"));
  s3_log(S3_LOG_INFO, "", "S3_AUDIT_LOG_DIR = %s\n", audit_log_dir.c_str());
  s3_log(S3_LOG_INFO, "", "S3_AUDIT_LOG_CONFIG = %s\n",
         audit_log_conf_file.c_str());
  s3_log(S3_LOG_INFO, "", "S3_AUDIT_LOG_FORMAT_TYPE = %s\n",
         audit_format_type_to_string(audit_log_format).c_str());
  s3_log(S3_LOG_INFO, "", "S3_AUDIT_LOGGER_POLICY = %s\n",
         audit_logger_policy.c_str());
  s3_log(S3_LOG_INFO, "", "S3_AUDIT_LOGGER_HOST = %s\n",
         audit_logger_host.c_str());
  s3_log(S3_LOG_INFO, "", "S3_AUDIT_LOGGER_PORT = %d\n", audit_logger_port);
  s3_log(S3_LOG_INFO, "", "S3_AUDIT_LOGGER_RSYSLOG_MSGID = %s\n",
         audit_logger_rsyslog_msgid.c_str());
  s3_log(S3_LOG_INFO, "", "S3_AUDIT_LOGGER_KAFKA_WEB_PATH = %s\n",
         audit_logger_kafka_web_path.c_str());
  s3_log(S3_LOG_INFO, "", "S3_ENABLE_MURMURHASH_OID = %s\n",
         (s3_enable_murmurhash_oid ? "true" : "false"));
  s3_log(S3_LOG_INFO, "", "S3_LOG_FLUSH_FREQUENCY = %d\n",
         log_flush_frequency_sec);
  s3_log(S3_LOG_INFO, "", "S3_ENABLE_AUTH_SSL = %s\n",
         (s3_enable_auth_ssl) ? "true" : "false");
  s3_log(S3_LOG_INFO, "", "S3_REUSEPORT = %s\n",
         (s3_reuseport) ? "true" : "false");
  s3_log(S3_LOG_INFO, "", "S3_MOTR_HTTP_REUSEPORT = %s\n",
         (motr_http_reuseport) ? "true" : "false");
  s3_log(S3_LOG_INFO, "", "S3_IAM_CERT_FILE = %s\n", s3_iam_cert_file.c_str());
  s3_log(S3_LOG_INFO, "", "S3_SERVER_IPV4_BIND_ADDR = %s\n",
         s3_ipv4_bind_addr.c_str());
  s3_log(S3_LOG_INFO, "", "S3_SERVER_IPV6_BIND_ADDR = %s\n",
         s3_ipv6_bind_addr.c_str());
  s3_log(S3_LOG_INFO, "", "S3_SERVER_MOTR_HTTP_BIND_ADDR = %s\n",
         motr_http_bind_addr.c_str());
  s3_log(S3_LOG_INFO, "", "S3_SERVER_BIND_PORT = %d\n", s3_bind_port);
  s3_log(S3_LOG_INFO, "", "S3_SERVER_MOTR_HTTP_BIND_PORT = %d\n",
         motr_http_bind_port);
  s3_log(S3_LOG_INFO, "", "S3_SERVER_SHUTDOWN_GRACE_PERIOD = %d\n",
         s3_grace_period_sec);
  s3_log(S3_LOG_INFO, "", "S3_ENABLE_PERF = %d\n", perf_enabled);
  s3_log(S3_LOG_INFO, "", "S3_SERVER_SSL_ENABLE = %d\n", s3server_ssl_enabled);
  s3_log(S3_LOG_INFO, "", "S3_SERVER_ENABLE_OBJECT_LEAK_TRACKING = %d\n",
         s3server_objectleak_tracking_enabled);
  s3_log(S3_LOG_INFO, "", "S3_SERVER_CERT_FILE = %s\n",
         s3server_ssl_cert_file.c_str());
  s3_log(S3_LOG_INFO, "", "S3_SERVER_PEM_FILE = %s\n",
         s3server_ssl_pem_file.c_str());
  s3_log(S3_LOG_INFO, "", "S3_SERVER_SSL_SESSION_TIMEOUT = %d\n",
         s3server_ssl_session_timeout_in_sec);
  s3_log(S3_LOG_INFO, "", "S3_READ_AHEAD_MULTIPLE = %d\n", read_ahead_multiple);
  s3_log(S3_LOG_INFO, "", "S3_PERF_LOG_FILENAME = %s\n", perf_log_file.c_str());
  s3_log(S3_LOG_INFO, "", "S3_SERVER_DEFAULT_ENDPOINT = %s\n",
         s3_default_endpoint.c_str());
  for (auto endpoint : s3_region_endpoints) {
    s3_log(S3_LOG_INFO, "", "S3 Server region endpoint = %s\n",
           endpoint.c_str());
  }

  s3_log(S3_LOG_INFO, "", "S3_AUTH_IP_ADDR = %s\n", auth_ip_addr.c_str());
  s3_log(S3_LOG_INFO, "", "S3_AUTH_PORT = %d\n", auth_port);
  s3_log(S3_LOG_INFO, "", "S3_version = %s\n", s3_version.c_str());

  s3_log(S3_LOG_INFO, "", "S3_MOTR_LOCAL_ADDR = %s\n", motr_local_addr.c_str());
  s3_log(S3_LOG_INFO, "", "S3_MOTR_HA_ADDR =  %s\n", motr_ha_addr.c_str());
  s3_log(S3_LOG_INFO, "", "S3_MOTR_PROF = %s\n", motr_profile.c_str());
  s3_log(S3_LOG_INFO, "", "S3_MOTR_LAYOUT_ID = %d\n", motr_layout_id);

  std::string unit_sizes = "";
  for (auto unit_size : motr_unit_sizes_for_mem_pool) {
    unit_sizes += std::to_string(unit_size) + ", ";
  }
  s3_log(S3_LOG_INFO, "", "S3_UNIT_SIZES_FOR_MEMORY_POOL = %s\n",
         unit_sizes.c_str());
  s3_log(S3_LOG_INFO, "", "S3_MOTR_MAX_UNITS_PER_REQUEST = %d\n",
         motr_units_per_request);
  s3_log(S3_LOG_INFO, "", "S3_MOTR_MAX_IDX_FETCH_COUNT = %d\n",
         motr_idx_fetch_count);
  s3_log(S3_LOG_INFO, "", "S3_MOTR_IS_OOSTORE = %s\n",
         (motr_is_oostore ? "true" : "false"));
  s3_log(S3_LOG_INFO, "", "S3_MOTR_IS_READ_VERIFY = %s\n",
         (motr_is_read_verify ? "true" : "false"));
  s3_log(S3_LOG_INFO, "", "S3_MOTR_TM_RECV_QUEUE_MIN_LEN = %d\n",
         motr_tm_recv_queue_min_len);
  s3_log(S3_LOG_INFO, "", "S3_MOTR_MAX_RPC_MSG_SIZE = %d\n",
         motr_max_rpc_msg_size);
  s3_log(S3_LOG_INFO, "", "S3_MOTR_PROCESS_FID = %s\n",
         motr_process_fid.c_str());
  s3_log(S3_LOG_INFO, "", "S3_MOTR_IDX_SERVICE_ID = %d\n", motr_idx_service_id);
  s3_log(S3_LOG_INFO, "", "S3_MOTR_CASS_CLUSTER_EP = %s\n",
         motr_cass_cluster_ep.c_str());
  s3_log(S3_LOG_INFO, "", "S3_MOTR_CASS_KEYSPACE = %s\n",
         motr_cass_keyspace.c_str());
  s3_log(S3_LOG_INFO, "", "S3_MOTR_CASS_MAX_COL_FAMILY_NUM = %d\n",
         motr_cass_max_column_family_num);
  s3_log(S3_LOG_INFO, "", "S3_MOTR_OPERATION_WAIT_PERIOD = %d\n",
         motr_op_wait_period);

  s3_log(S3_LOG_INFO, "", "S3_MOTR_READ_POOL_INITIAL_BUFFER_COUNT = %zu\n",
         motr_read_pool_initial_buffer_count);
  s3_log(S3_LOG_INFO, "", "S3_MOTR_READ_POOL_EXPANDABLE_COUNT = %zu\n",
         motr_read_pool_expandable_count);
  s3_log(S3_LOG_INFO, "", "S3_MOTR_READ_POOL_MAX_THRESHOLD = %zu\n",
         motr_read_pool_max_threshold);

  s3_log(S3_LOG_INFO, "", "S3_LIBEVENT_POOL_INITIAL_SIZE = %zu\n",
         libevent_pool_initial_size);
  s3_log(S3_LOG_INFO, "", "S3_LIBEVENT_POOL_EXPANDABLE_SIZE = %zu\n",
         libevent_pool_expandable_size);
  s3_log(S3_LOG_INFO, "", "S3_LIBEVENT_POOL_MAX_THRESHOLD = %zu\n",
         libevent_pool_max_threshold);
  s3_log(S3_LOG_INFO, "", "S3_LIBEVENT_POOL_RESERVE_SIZE = %zu\n",
         libevent_pool_reserve_size);
  s3_log(S3_LOG_INFO, "", "S3_LIBEVENT_POOL_RESERVE_PERCENT = %u\n",
         libevent_pool_reserve_percent);

  s3_log(S3_LOG_INFO, "", "FLAGS_fake_motr_createobj = %d\n",
         FLAGS_fake_motr_createobj);
  s3_log(S3_LOG_INFO, "", "FLAGS_fake_motr_writeobj = %d\n",
         FLAGS_fake_motr_writeobj);
  s3_log(S3_LOG_INFO, "", "FLAGS_fake_motr_readobj = %d\n",
         FLAGS_fake_motr_readobj);
  s3_log(S3_LOG_INFO, "", "FLAGS_fake_motr_deleteobj = %d\n",
         FLAGS_fake_motr_deleteobj);
  s3_log(S3_LOG_INFO, "", "FLAGS_fake_motr_createidx = %d\n",
         FLAGS_fake_motr_createidx);
  s3_log(S3_LOG_INFO, "", "FLAGS_fake_motr_deleteidx = %d\n",
         FLAGS_fake_motr_deleteidx);
  s3_log(S3_LOG_INFO, "", "FLAGS_fake_motr_getkv = %d\n",
         FLAGS_fake_motr_getkv);
  s3_log(S3_LOG_INFO, "", "FLAGS_fake_motr_putkv = %d\n",
         FLAGS_fake_motr_putkv);
  s3_log(S3_LOG_INFO, "", "FLAGS_fake_motr_deletekv = %d\n",
         FLAGS_fake_motr_deletekv);
  s3_log(S3_LOG_INFO, "", "FLAGS_fake_motr_redis_kvs = %d\n",
         FLAGS_fake_motr_redis_kvs);
  s3_log(S3_LOG_INFO, "", "FLAGS_disable_auth = %d\n", FLAGS_disable_auth);

  s3_log(S3_LOG_INFO, "", "S3_ENABLE_STATS = %s\n",
         (stats_enable ? "true" : "false"));
  s3_log(S3_LOG_INFO, "", "S3_STATSD_IP_ADDR = %s\n", statsd_ip_addr.c_str());
  s3_log(S3_LOG_INFO, "", "S3_STATSD_PORT = %d\n", statsd_port);
  s3_log(S3_LOG_INFO, "", "S3_STATSD_MAX_SEND_RETRY = %d\n",
         statsd_max_send_retry);
  s3_log(S3_LOG_INFO, "", "S3_STATS_ALLOWLIST_FILENAME = %s\n",
         stats_allowlist_filename.c_str());
  s3_log(S3_LOG_INFO, "",
         "S3_PERF_STATS_INOUT_BYTES_INTERVAL_MSEC = %" PRIu32 "\n",
         perf_stats_inout_bytes_interval_msec);

  s3_log(S3_LOG_INFO, "", "S3_REDIS_SERVER_PORT = %d\n", (int)redis_srv_port);
  s3_log(S3_LOG_INFO, "", "S3_REDIS_SERVER_ADDRESS = %s\n",
         redis_srv_addr.c_str());

  s3_log(S3_LOG_INFO, "", "S3_SERVER_MOTR_ETIMEDOUT_MAX_THRESHOLD = %u\n",
         motr_etimedout_max_threshold);
  s3_log(S3_LOG_INFO, "", "S3_SERVER_MOTR_ETIMEDOUT_WINDOW_SEC = %u\n",
         motr_etimedout_window_sec);

  s3_log(S3_LOG_INFO, "", "S3_SERVER_ENABLE_ADDB_DUMP = %s\n",
         is_s3server_addb_dump_enabled() ? "true" : "false");
  s3_log(S3_LOG_INFO, "", "S3_MOTR_READ_MEMPOOL_ZERO_BUFFER=%s\n",
         motr_read_mempool_zeroed_buffer ? "true" : "false");
  s3_log(S3_LOG_INFO, "", "S3_LIBEVENT_MEMPOOL_ZERO_BUFFER=%s\n",
         libevent_mempool_zeroed_buffer ? "true" : "false");

  return;
}

std::string S3Option::get_s3_nodename() { return s3_nodename; }

unsigned short S3Option::get_s3_bind_port() { return s3_bind_port; }

unsigned short S3Option::get_motr_http_bind_port() {
  return motr_http_bind_port;
}

std::string S3Option::get_s3_pidfile() { return s3_pidfile; }

std::string S3Option::get_s3_audit_config() { return audit_log_conf_file; }

AuditFormatType S3Option::get_s3_audit_format_type() {
  return audit_log_format;
}

std::string S3Option::get_audit_logger_policy() { return audit_logger_policy; }

void S3Option::set_audit_logger_policy(std::string const& policy) {
  audit_logger_policy = policy;
}

std::string S3Option::get_audit_logger_host() { return audit_logger_host; }

int S3Option::get_audit_logger_port() { return audit_logger_port; }

std::string S3Option::get_audit_logger_rsyslog_msgid() {
  return audit_logger_rsyslog_msgid;
}

std::string S3Option::get_audit_logger_kafka_web_path() {
  return audit_logger_kafka_web_path;
}

unsigned short S3Option::get_s3_grace_period_sec() {
  return s3_grace_period_sec;
}

bool S3Option::get_is_s3_shutting_down() { return is_s3_shutting_down; }

void S3Option::set_is_s3_shutting_down(bool is_shutting_down) {
  is_s3_shutting_down = is_shutting_down;
}

unsigned short S3Option::get_auth_port() { return auth_port; }

std::string S3Option::get_s3_version() { return s3_version; }

unsigned short S3Option::get_motr_layout_id() { return motr_layout_id; }

std::string S3Option::get_option_file() { return option_file; }

std::string S3Option::get_layout_recommendation_file() {
  return layout_recommendation_file;
}

std::string S3Option::get_daemon_dir() { return s3_daemon_dir; }

size_t S3Option::get_motr_read_pool_initial_buffer_count() {
  return motr_read_pool_initial_buffer_count;
}

size_t S3Option::get_motr_read_pool_expandable_count() {
  return motr_read_pool_expandable_count;
}

size_t S3Option::get_motr_read_pool_max_threshold() {
  return motr_read_pool_max_threshold;
}

size_t S3Option::get_libevent_pool_initial_size() {
  return libevent_pool_initial_size;
}

size_t S3Option::get_libevent_max_read_size() { return libevent_max_read_size; }

size_t S3Option::get_libevent_pool_buffer_size() {
  return libevent_pool_buffer_size;
}

size_t S3Option::get_libevent_pool_expandable_size() {
  return libevent_pool_expandable_size;
}

size_t S3Option::get_libevent_pool_max_threshold() {
  return libevent_pool_max_threshold;
}

size_t S3Option::get_libevent_pool_reserve_size() const {
  return libevent_pool_reserve_size;
}

unsigned S3Option::get_libevent_pool_reserve_percent() const {
  return libevent_pool_reserve_percent;
}

unsigned short S3Option::do_redirection() { return s3_daemon_redirect; }

void S3Option::set_option_file(std::string filename) { option_file = filename; }

void S3Option::set_layout_recommendation_file(std::string filename) {
  layout_recommendation_file = filename;
}

void S3Option::set_daemon_dir(std::string path) { s3_daemon_dir = path; }

void S3Option::set_redirection(unsigned short redirect) {
  s3_daemon_redirect = redirect;
}

std::string S3Option::get_log_dir() { return log_dir; }

std::string S3Option::get_audit_log_dir() { return audit_log_dir; }

std::string S3Option::get_log_level() { return log_level; }

int S3Option::get_log_file_max_size_in_mb() { return log_file_max_size_mb; }

int S3Option::get_log_flush_frequency_in_sec() {
  return log_flush_frequency_sec;
}

bool S3Option::is_log_buffering_enabled() { return log_buffering_enable; }

bool S3Option::is_murmurhash_oid_enabled() { return s3_enable_murmurhash_oid; }

bool S3Option::is_s3_ssl_auth_enabled() { return s3_enable_auth_ssl; }

const char* S3Option::get_iam_cert_file() { return s3_iam_cert_file.c_str(); }

const char* S3Option::get_s3server_ssl_cert_file() {
  return s3server_ssl_cert_file.c_str();
}

const char* S3Option::get_s3server_ssl_pem_file() {
  return s3server_ssl_pem_file.c_str();
}

int S3Option::get_s3server_ssl_session_timeout() {
  return s3server_ssl_session_timeout_in_sec;
}

std::string S3Option::get_perf_log_filename() { return perf_log_file; }

int S3Option::get_read_ahead_multiple() { return read_ahead_multiple; }

std::string S3Option::get_ipv4_bind_addr() { return s3_ipv4_bind_addr; }

std::string S3Option::get_ipv6_bind_addr() { return s3_ipv6_bind_addr; }

std::string S3Option::get_motr_http_bind_addr() { return motr_http_bind_addr; }

std::string S3Option::get_default_endpoint() { return s3_default_endpoint; }

std::set<std::string>& S3Option::get_region_endpoints() {
  return s3_region_endpoints;
}

std::string S3Option::get_motr_local_addr() { return motr_local_addr; }

std::string S3Option::get_motr_ha_addr() { return motr_ha_addr; }

std::string S3Option::get_motr_prof() { return motr_profile; }

std::vector<int> S3Option::get_motr_unit_sizes_for_mem_pool() {
  return motr_unit_sizes_for_mem_pool;
}

unsigned short S3Option::get_motr_units_per_request() {
  return motr_units_per_request;
}

unsigned short S3Option::get_motr_op_wait_period() {
  return motr_op_wait_period;
}

int S3Option::get_motr_idx_fetch_count() { return motr_idx_fetch_count; }

void S3Option::set_motr_idx_fetch_count(short count) {
  motr_idx_fetch_count = count;
}

unsigned short S3Option::get_client_req_read_timeout_secs() {
  return s3_client_req_read_timeout_secs;
}

unsigned int S3Option::get_motr_write_payload_size(int layoutid) {
  return S3MotrLayoutMap::get_instance()->get_unit_size_for_layout(layoutid) *
         motr_units_per_request;
}

unsigned int S3Option::get_motr_read_payload_size(int layoutid) {
  return S3MotrLayoutMap::get_instance()->get_unit_size_for_layout(layoutid) *
         motr_units_per_request;
}

bool S3Option::get_motr_is_oostore() { return motr_is_oostore; }

bool S3Option::get_motr_is_read_verify() { return motr_is_read_verify; }

unsigned int S3Option::get_motr_tm_recv_queue_min_len() {
  return motr_tm_recv_queue_min_len;
}

unsigned int S3Option::get_motr_max_rpc_msg_size() {
  return motr_max_rpc_msg_size;
}

std::string S3Option::get_motr_process_fid() { return motr_process_fid; }

int S3Option::get_motr_idx_service_id() { return motr_idx_service_id; }

std::string S3Option::get_motr_cass_cluster_ep() {
  return motr_cass_cluster_ep;
}

std::string S3Option::get_motr_cass_keyspace() { return motr_cass_keyspace; }

int S3Option::get_motr_cass_max_column_family_num() {
  return motr_cass_max_column_family_num;
}

std::string S3Option::get_auth_ip_addr() { return auth_ip_addr; }

void S3Option::disable_auth() { FLAGS_disable_auth = true; }

void S3Option::enable_auth() { FLAGS_disable_auth = false; }

bool S3Option::is_auth_disabled() { return FLAGS_disable_auth; }

bool S3Option::is_s3server_addb_dump_enabled() { return FLAGS_addb; }

unsigned short S3Option::s3_performance_enabled() { return perf_enabled; }

bool S3Option::is_s3server_ssl_enabled() { return s3server_ssl_enabled; }

bool S3Option::is_s3server_objectleak_tracking_enabled() {
  return s3server_objectleak_tracking_enabled;
}

void S3Option::set_s3server_objectleak_tracking_enabled(const bool& flag) {
  s3server_objectleak_tracking_enabled = flag;
}

bool S3Option::is_fake_motr_createobj() { return FLAGS_fake_motr_createobj; }

bool S3Option::is_fake_motr_writeobj() { return FLAGS_fake_motr_writeobj; }

bool S3Option::is_fake_motr_readobj() { return FLAGS_fake_motr_readobj; }

bool S3Option::is_fake_motr_deleteobj() { return FLAGS_fake_motr_deleteobj; }

bool S3Option::is_fake_motr_createidx() { return FLAGS_fake_motr_createidx; }

bool S3Option::is_fake_motr_deleteidx() { return FLAGS_fake_motr_deleteidx; }

bool S3Option::is_fake_motr_getkv() { return FLAGS_fake_motr_getkv; }

bool S3Option::is_fake_motr_putkv() { return FLAGS_fake_motr_putkv; }

bool S3Option::is_fake_motr_deletekv() { return FLAGS_fake_motr_deletekv; }

bool S3Option::is_fake_motr_redis_kvs() { return FLAGS_fake_motr_redis_kvs; }

/* For the moment sync kvs operation for fake kvs is not supported */
bool S3Option::is_sync_kvs_allowed() {
  return !(FLAGS_fake_motr_redis_kvs || FLAGS_fake_motr_putkv);
}

unsigned short S3Option::get_max_retry_count() { return max_retry_count; }

unsigned short S3Option::get_audit_max_retry_count() {
  return max_audit_retry_count;
}

unsigned short S3Option::get_retry_interval_in_millisec() {
  return retry_interval_millisec;
}

void S3Option::set_eventbase(evbase_t* base) { eventbase = base; }

bool S3Option::is_stats_enabled() { return stats_enable; }

void S3Option::set_stats_enable(bool enable) { stats_enable = enable; }

std::string S3Option::get_statsd_ip_addr() { return statsd_ip_addr; }

unsigned short S3Option::get_statsd_port() { return statsd_port; }

unsigned short S3Option::get_statsd_max_send_retry() {
  return statsd_max_send_retry;
}

std::string S3Option::get_stats_allowlist_filename() {
  return stats_allowlist_filename;
}

uint32_t S3Option::get_perf_stats_inout_bytes_interval_msec() {
  return perf_stats_inout_bytes_interval_msec;
}

void S3Option::set_stats_allowlist_filename(std::string filename) {
  stats_allowlist_filename = filename;
}

evbase_t* S3Option::get_eventbase() { return eventbase; }

void S3Option::enable_fault_injection() { FLAGS_fault_injection = true; }

void S3Option::enable_get_oid() { FLAGS_getoid = true; }

void S3Option::enable_murmurhash_oid() { s3_enable_murmurhash_oid = true; }

void S3Option::disable_murmurhash_oid() { s3_enable_murmurhash_oid = false; }

void S3Option::enable_reuseport() { FLAGS_reuseport = true; }

bool S3Option::is_s3_reuseport_enabled() { return s3_reuseport; }

bool S3Option::is_motr_http_reuseport_enabled() { return motr_http_reuseport; }

bool S3Option::is_fi_enabled() { return FLAGS_fault_injection; }

bool S3Option::is_getoid_enabled() { return FLAGS_getoid; }

std::string S3Option::get_redis_srv_addr() { return redis_srv_addr; }

unsigned short S3Option::get_redis_srv_port() { return redis_srv_port; }

unsigned S3Option::get_motr_etimedout_max_threshold() {
  return motr_etimedout_max_threshold;
}

unsigned S3Option::get_motr_etimedout_window_sec() {
  return motr_etimedout_window_sec;
}

bool S3Option::get_motr_read_mempool_zeroed_buffer() {
  return motr_read_mempool_zeroed_buffer;
}

bool S3Option::get_libevent_mempool_zeroed_buffer() {
  return libevent_mempool_zeroed_buffer;
}
