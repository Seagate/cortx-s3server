/*
 * COPYRIGHT 2016 SEAGATE LLC
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
 * Original author:  Rajesh Nambiar        <rajesh.nambiar@seagate.com>
 * Original creation date: 31-March-2016
 */


#include <yaml-cpp/yaml.h>
#include <vector>
#include <inttypes.h>
#include "s3_option.h"
#include "s3_log.h"

S3Option* S3Option::option_instance = NULL;

bool S3Option::load_section(std::string section_name, bool force_override_from_config = false) {
  YAML::Node root_node = YAML::LoadFile(option_file);
  if (root_node.IsNull()) {
    return false; //File Not Found?
  }
  YAML::Node s3_option_node = root_node[section_name];
  if (s3_option_node.IsNull()) {
    return false;
  }
  if (force_override_from_config) {
    if (section_name == "S3_SERVER_CONFIG") {
      s3_daemon_dir = s3_option_node["S3_DAEMON_WORKING_DIR"].as<std::string>();
      s3_daemon_redirect = s3_option_node["S3_DAEMON_DO_REDIRECTION"].as<unsigned short>();
      s3_bind_port = s3_option_node["S3_SERVER_BIND_PORT"].as<unsigned short>();
      log_filename = s3_option_node["S3_LOG_FILENAME"].as<std::string>();
      log_level = s3_option_node["S3_LOG_MODE"].as<std::string>();
      s3_bind_addr = s3_option_node["S3_SERVER_BIND_ADDR"].as<std::string>();
      perf_enabled = s3_option_node["S3_ENABLE_PERF"].as<unsigned short>();
      read_ahead_multiple = s3_option_node["S3_READ_AHEAD_MULTIPLE"].as<int>();
      s3_default_endpoint = s3_option_node["S3_SERVER_DEFAULT_ENDPOINT"].as<std::string>();
      s3_region_endpoints.clear();
      for (unsigned short i = 0; i < s3_option_node["S3_SERVER_REGION_ENDPOINTS"].size(); ++i) {
        s3_region_endpoints.insert(s3_option_node["S3_SERVER_REGION_ENDPOINTS"][i].as<std::string>());
      }
      max_retry_count = s3_option_node["S3_MAX_RETRY_COUNT"].as<unsigned short>();
      retry_interval_millisec = s3_option_node["S3_RETRY_INTERVAL_MILLISEC"].as<unsigned short>();
    } else if (section_name == "S3_AUTH_CONFIG") {
      auth_port = s3_option_node["S3_AUTH_PORT"].as<unsigned short>();
      auth_ip_addr = s3_option_node["S3_AUTH_IP_ADDR"].as<std::string>();
    } else if (section_name == "S3_CLOVIS_CONFIG") {
      clovis_local_addr = s3_option_node["S3_CLOVIS_LOCAL_ADDR"].as<std::string>();
      clovis_confd_addr = s3_option_node["S3_CLOVIS_CONFD_ADDR"].as<std::string>();
      clovis_ha_addr = s3_option_node["S3_CLOVIS_HA_ADDR"].as<std::string>();
      clovis_profile = s3_option_node["S3_CLOVIS_PROF"].as<std::string>();
      clovis_layout_id = s3_option_node["S3_CLOVIS_LAYOUT_ID"].as<unsigned short>();
      clovis_block_size = s3_option_node["S3_CLOVIS_BLOCK_SIZE"].as<unsigned int>();
      clovis_factor = s3_option_node["S3_CLOVIS_MAX_BLOCKS_PER_REQUEST"].as<unsigned short>();
      clovis_idx_fetch_count = s3_option_node["S3_CLOVIS_MAX_IDX_FETCH_COUNT"].as<int>();
      clovis_is_oostore = s3_option_node["S3_CLOVIS_IS_OOSTORE"].as<bool>();
      clovis_is_read_verify =
          s3_option_node["S3_CLOVIS_IS_READ_VERIFY"].as<bool>();
      clovis_tm_recv_queue_min_len =
          s3_option_node["S3_CLOVIS_TM_RECV_QUEUE_MIN_LEN"].as<unsigned int>();
      clovis_max_rpc_msg_size =
          s3_option_node["S3_CLOVIS_MAX_RPC_MSG_SIZE"].as<unsigned int>();
      clovis_process_fid =
          s3_option_node["S3_CLOVIS_PROCESS_FID"].as<std::string>();
      clovis_idx_service_id =
          s3_option_node["S3_CLOVIS_IDX_SERVICE_ID"].as<int>();
      clovis_cass_cluster_ep =
          s3_option_node["S3_CLOVIS_CASS_CLUSTER_EP"].as<std::string>();
      clovis_cass_keyspace =
          s3_option_node["S3_CLOVIS_CASS_KEYSPACE"].as<std::string>();
      clovis_cass_max_column_family_num =
          s3_option_node["S3_CLOVIS_CASS_MAX_COL_FAMILY_NUM"].as<int>();
    }
  } else {
    if (section_name == "S3_SERVER_CONFIG") {
      if (!(cmd_opt_flag & S3_OPTION_BIND_PORT)) {
        s3_bind_port = s3_option_node["S3_SERVER_BIND_PORT"].as<unsigned short>();
      }
      if (!(cmd_opt_flag & S3_OPTION_LOG_FILE)) {
        log_filename = s3_option_node["S3_LOG_FILENAME"].as<std::string>();
      }
      if (!(cmd_opt_flag & S3_OPTION_PERF_LOG_FILE)) {
        perf_log_file = s3_option_node["S3_PERF_LOG_FILENAME"].as<std::string>();
      }
      if (!(cmd_opt_flag & S3_OPTION_LOG_MODE)) {
        log_level = s3_option_node["S3_LOG_MODE"].as<std::string>();
      }
      if (!(cmd_opt_flag & S3_OPTION_BIND_ADDR)) {
        s3_bind_addr = s3_option_node["S3_SERVER_BIND_ADDR"].as<std::string>();
      }
      s3_daemon_dir = s3_option_node["S3_DAEMON_WORKING_DIR"].as<std::string>();
      s3_daemon_redirect = s3_option_node["S3_DAEMON_DO_REDIRECTION"].as<unsigned short>();
      s3_default_endpoint = s3_option_node["S3_SERVER_DEFAULT_ENDPOINT"].as<std::string>();
      s3_region_endpoints.clear();
      for (unsigned short i = 0; i < s3_option_node["S3_SERVER_REGION_ENDPOINTS"].size(); ++i) {
        s3_region_endpoints.insert(s3_option_node["S3_SERVER_REGION_ENDPOINTS"][i].as<std::string>());
      }
      perf_enabled = s3_option_node["S3_ENABLE_PERF"].as<unsigned short>();
      read_ahead_multiple = s3_option_node["S3_READ_AHEAD_MULTIPLE"].as<int>();
      max_retry_count = s3_option_node["S3_MAX_RETRY_COUNT"].as<unsigned short>();
      retry_interval_millisec = s3_option_node["S3_RETRY_INTERVAL_MILLISEC"].as<unsigned short>();
    } else if (section_name == "S3_AUTH_CONFIG") {
      if (!(cmd_opt_flag & S3_OPTION_AUTH_PORT)) {
        auth_port = s3_option_node["S3_AUTH_PORT"].as<unsigned short>();
      }
      if (!(cmd_opt_flag & S3_OPTION_AUTH_IP_ADDR)) {
        auth_ip_addr = s3_option_node["S3_AUTH_IP_ADDR"].as<std::string>();
      }
    } else if (section_name == "S3_CLOVIS_CONFIG") {
      if (!(cmd_opt_flag & S3_OPTION_CLOVIS_LOCAL_ADDR)) {
        clovis_local_addr = s3_option_node["S3_CLOVIS_LOCAL_ADDR"].as<std::string>();
      }
      if (!(cmd_opt_flag & S3_OPTION_CLOVIS_CONFD_ADDR)) {
        clovis_confd_addr = s3_option_node["S3_CLOVIS_CONFD_ADDR"].as<std::string>();
      }
      if (!(cmd_opt_flag & S3_OPTION_CLOVIS_HA_ADDR)) {
        clovis_ha_addr = s3_option_node["S3_CLOVIS_HA_ADDR"].as<std::string>();
      }
      if (!(cmd_opt_flag & S3_CLOVIS_LAYOUT_ID)) {
        clovis_layout_id = s3_option_node["S3_CLOVIS_LAYOUT_ID"].as<unsigned short>();
      }
      clovis_profile = s3_option_node["S3_CLOVIS_PROF"].as<std::string>();
      clovis_block_size = s3_option_node["S3_CLOVIS_BLOCK_SIZE"].as<unsigned int>();
      clovis_factor = s3_option_node["S3_CLOVIS_MAX_BLOCKS_PER_REQUEST"].as<unsigned short>();
      clovis_idx_fetch_count = s3_option_node["S3_CLOVIS_MAX_IDX_FETCH_COUNT"].as<int>();
      clovis_is_oostore = s3_option_node["S3_CLOVIS_IS_OOSTORE"].as<bool>();
      clovis_is_read_verify =
          s3_option_node["S3_CLOVIS_IS_READ_VERIFY"].as<bool>();
      clovis_tm_recv_queue_min_len =
          s3_option_node["S3_CLOVIS_TM_RECV_QUEUE_MIN_LEN"].as<unsigned int>();
      clovis_max_rpc_msg_size =
          s3_option_node["S3_CLOVIS_MAX_RPC_MSG_SIZE"].as<unsigned int>();
      clovis_process_fid =
          s3_option_node["S3_CLOVIS_PROCESS_FID"].as<std::string>();
      clovis_idx_service_id =
          s3_option_node["S3_CLOVIS_IDX_SERVICE_ID"].as<int>();
      clovis_cass_cluster_ep =
          s3_option_node["S3_CLOVIS_CASS_CLUSTER_EP"].as<std::string>();
      clovis_cass_keyspace =
          s3_option_node["S3_CLOVIS_CASS_KEYSPACE"].as<std::string>();
      clovis_cass_max_column_family_num =
          s3_option_node["S3_CLOVIS_CASS_MAX_COL_FAMILY_NUM"].as<int>();
    }
  }
  return true;
}

bool S3Option::load_all_sections(bool force_override_from_config=false) {
  std::string s3_section;
  try {
    YAML::Node root_node = YAML::LoadFile(option_file);
    if (root_node.IsNull()) {
       return false; //File Not Found?
    }
    for (unsigned short i = 0; i < root_node["S3Config_Sections"].size(); ++i) {
      load_section(root_node["S3Config_Sections"][i].as<std::string>(), force_override_from_config);
    }
  } catch (const YAML::RepresentationException &e) {
    s3_log(S3_LOG_FATAL, "YAML::RepresentationException caught: %s\n", e.what());
    s3_log(S3_LOG_FATAL, "Yaml file %s is incorrect\n", option_file.c_str());
    return false;
  } catch (const YAML::ParserException &e) {
    s3_log(S3_LOG_FATAL, "YAML::ParserException caught: %s\n", e.what());
    s3_log(S3_LOG_FATAL, "Parsing Error in yaml file %s\n", option_file.c_str());
    return false;
  } catch (const YAML::EmitterException &e) {
    s3_log(S3_LOG_FATAL, "YAML::EmitterException caught: %s\n", e.what());
    return false;
  } catch (YAML::Exception& e) {
    s3_log(S3_LOG_FATAL, "YAML::Exception caught: %s\n", e.what());
    return false;
  }
  return true;
}

void S3Option::set_cmdline_option(int option_flag, const char *optarg) {
  if (option_flag & S3_OPTION_BIND_ADDR) {
    s3_bind_addr = optarg;
  } else if (option_flag & S3_OPTION_BIND_PORT) {
    s3_bind_port = atoi(optarg);
  } else if (option_flag & S3_OPTION_CLOVIS_LOCAL_ADDR) {
    clovis_local_addr = optarg;
  } else if (option_flag & S3_OPTION_CLOVIS_CONFD_ADDR) {
    clovis_confd_addr = optarg;
  } else if (option_flag & S3_OPTION_CLOVIS_HA_ADDR) {
    clovis_ha_addr = optarg;
  } else if (option_flag & S3_OPTION_AUTH_IP_ADDR) {
    auth_ip_addr = optarg;
  } else if (option_flag & S3_OPTION_AUTH_PORT) {
    auth_port = atoi(optarg);
  } else if (option_flag & S3_CLOVIS_LAYOUT_ID) {
    clovis_layout_id = atoi(optarg);
  } else if (option_flag & S3_OPTION_LOG_FILE) {
    log_filename = optarg;
  } else if (option_flag & S3_OPTION_PERF_LOG_FILE) {
    perf_log_file = optarg;
  } else if (option_flag & S3_OPTION_LOG_MODE) {
    log_level = optarg;
  }
  cmd_opt_flag |= option_flag;
  return;
}

int S3Option::get_cmd_opt_flag() {
  return cmd_opt_flag;
}

void S3Option::dump_options() {
  s3_log(S3_LOG_INFO, "S3_DAEMON_WORKING_DIR = %s\n", s3_daemon_dir.c_str());
  s3_log(S3_LOG_INFO, "S3_DAEMON_DO_REDIRECTION = %d\n", s3_daemon_redirect);
  s3_log(S3_LOG_INFO, "S3_LOG_FILENAME = %s\n", log_filename.c_str());
  s3_log(S3_LOG_INFO, "S3_LOG_MODE = %s\n", log_level.c_str());
  s3_log(S3_LOG_INFO, "S3_SERVER_BIND_ADDR = %s\n", s3_bind_addr.c_str());
  s3_log(S3_LOG_INFO, "S3_SERVER_BIND_PORT = %d\n", s3_bind_port);
  s3_log(S3_LOG_INFO, "S3_ENABLE_PERF = %d\n", perf_enabled);
  s3_log(S3_LOG_INFO, "S3_READ_AHEAD_MULTIPLE = %d\n", read_ahead_multiple);
  s3_log(S3_LOG_INFO, "S3_PERF_LOG_FILENAME = %s\n", perf_log_file.c_str());
  s3_log(S3_LOG_INFO, "S3_SERVER_DEFAULT_ENDPOINT = %s\n", s3_default_endpoint.c_str());
  for (auto endpoint : s3_region_endpoints) {
    s3_log(S3_LOG_INFO, "S3 Server region endpoint = %s\n", endpoint.c_str());
  }

  s3_log(S3_LOG_INFO, "S3_AUTH_IP_ADDR = %s\n", auth_ip_addr.c_str());
  s3_log(S3_LOG_INFO, "S3_AUTH_PORT = %d\n", auth_port);

  s3_log(S3_LOG_INFO, "S3_CLOVIS_LOCAL_ADDR = %s\n", clovis_local_addr.c_str());
  s3_log(S3_LOG_INFO, "S3_CLOVIS_CONFD_ADDR = %s\n", clovis_confd_addr.c_str());
  s3_log(S3_LOG_INFO, "S3_CLOVIS_HA_ADDR =  %s\n", clovis_ha_addr.c_str());
  s3_log(S3_LOG_INFO, "S3_CLOVIS_PROF = %s\n", clovis_profile.c_str());
  s3_log(S3_LOG_INFO, "S3_CLOVIS_LAYOUT_ID = %d\n", clovis_layout_id);
  s3_log(S3_LOG_INFO, "S3_CLOVIS_BLOCK_SIZE = %d\n", clovis_block_size);
  s3_log(S3_LOG_INFO, "S3_CLOVIS_MAX_BLOCKS_PER_REQUEST = %d\n", clovis_factor);
  s3_log(S3_LOG_INFO, "S3_CLOVIS_MAX_IDX_FETCH_COUNT = %d\n", clovis_idx_fetch_count);
  s3_log(S3_LOG_INFO, "S3_CLOVIS_IS_OOSTORE = %s\n",
         (clovis_is_oostore ? "true" : "false"));
  s3_log(S3_LOG_INFO, "S3_CLOVIS_IS_READ_VERIFY = %s\n",
         (clovis_is_read_verify ? "true" : "false"));
  s3_log(S3_LOG_INFO, "S3_CLOVIS_TM_RECV_QUEUE_MIN_LEN = %d\n",
         clovis_tm_recv_queue_min_len);
  s3_log(S3_LOG_INFO, "S3_CLOVIS_MAX_RPC_MSG_SIZE = %d\n",
         clovis_max_rpc_msg_size);
  s3_log(S3_LOG_INFO, "S3_CLOVIS_PROCESS_FID = %s\n",
         clovis_process_fid.c_str());
  s3_log(S3_LOG_INFO, "S3_CLOVIS_IDX_SERVICE_ID = %d\n", clovis_idx_service_id);
  s3_log(S3_LOG_INFO, "S3_CLOVIS_CASS_CLUSTER_EP = %s\n",
         clovis_cass_cluster_ep.c_str());
  s3_log(S3_LOG_INFO, "S3_CLOVIS_CASS_KEYSPACE = %s\n",
         clovis_cass_keyspace.c_str());
  s3_log(S3_LOG_INFO, "S3_CLOVIS_CASS_MAX_COL_FAMILY_NUM = %d\n",
         clovis_cass_max_column_family_num);

  s3_log(S3_LOG_INFO, "FLAGS_fake_clovis_createobj = %d\n", FLAGS_fake_clovis_createobj);
  s3_log(S3_LOG_INFO, "FLAGS_fake_clovis_writeobj = %d\n", FLAGS_fake_clovis_writeobj);
  s3_log(S3_LOG_INFO, "FLAGS_fake_clovis_deleteobj = %d\n", FLAGS_fake_clovis_deleteobj);
  s3_log(S3_LOG_INFO, "FLAGS_fake_clovis_createidx = %d\n", FLAGS_fake_clovis_createidx);
  s3_log(S3_LOG_INFO, "FLAGS_fake_clovis_deleteidx = %d\n", FLAGS_fake_clovis_deleteidx);
  s3_log(S3_LOG_INFO, "FLAGS_fake_clovis_getkv = %d\n", FLAGS_fake_clovis_getkv);
  s3_log(S3_LOG_INFO, "FLAGS_fake_clovis_putkv = %d\n", FLAGS_fake_clovis_putkv);
  s3_log(S3_LOG_INFO, "FLAGS_fake_clovis_deletekv = %d\n", FLAGS_fake_clovis_deletekv);
  s3_log(S3_LOG_INFO, "FLAGS_disable_auth = %d\n", FLAGS_disable_auth);
  return;
}

unsigned short S3Option::get_s3_bind_port() {
  return s3_bind_port;
}

unsigned short S3Option::get_auth_port() {
  return auth_port;
}

unsigned short S3Option::get_clovis_layout_id() {
  return clovis_layout_id;
}

std::string S3Option::get_option_file() {
  return option_file;
}

std::string S3Option::get_daemon_dir() {
  return s3_daemon_dir;
}

unsigned short S3Option::do_redirection() {
  return s3_daemon_redirect;
}

void S3Option::set_option_file(std::string filename) {
  option_file = filename;
}

void S3Option::set_daemon_dir(std::string path) {
  s3_daemon_dir = path;
}

void S3Option::set_redirection(unsigned short redirect) {
  s3_daemon_redirect = redirect;
}

std::string S3Option::get_log_filename() {
  return log_filename;
}

std::string S3Option::get_log_level() {
  return log_level;
}

std::string S3Option::get_perf_log_filename() {
  return perf_log_file;
}

int S3Option::get_read_ahead_multiple() {
  return read_ahead_multiple;
}

std::string S3Option::get_bind_addr() {
  return s3_bind_addr;
}

std::string S3Option::get_default_endpoint() {
    return s3_default_endpoint;
}

std::set<std::string>& S3Option::get_region_endpoints() {
    return s3_region_endpoints;
}

std::string S3Option::get_clovis_local_addr() {
  return clovis_local_addr;
}

std::string S3Option::get_clovis_confd_addr() {
  return clovis_confd_addr;
}

std::string S3Option::get_clovis_ha_addr() {
  return clovis_ha_addr;
}

std::string S3Option::get_clovis_prof() {
  return clovis_profile;
}

unsigned int S3Option::get_clovis_block_size() {
  return clovis_block_size;
}

unsigned short S3Option::get_clovis_factor() {
  return clovis_factor;
}

int S3Option::get_clovis_idx_fetch_count() {
  return clovis_idx_fetch_count;
}

unsigned int S3Option::get_clovis_write_payload_size() {
  return clovis_block_size * clovis_factor;
}

unsigned int S3Option::get_clovis_read_payload_size() {
  return clovis_block_size * clovis_factor;
}

bool S3Option::get_clovis_is_oostore() { return clovis_is_oostore; }

bool S3Option::get_clovis_is_read_verify() { return clovis_is_read_verify; }

unsigned int S3Option::get_clovis_tm_recv_queue_min_len() {
  return clovis_tm_recv_queue_min_len;
}

unsigned int S3Option::get_clovis_max_rpc_msg_size() {
  return clovis_max_rpc_msg_size;
}

std::string S3Option::get_clovis_process_fid() { return clovis_process_fid; }

int S3Option::get_clovis_idx_service_id() { return clovis_idx_service_id; }

std::string S3Option::get_clovis_cass_cluster_ep() {
  return clovis_cass_cluster_ep;
}

std::string S3Option::get_clovis_cass_keyspace() {
  return clovis_cass_keyspace;
}

int S3Option::get_clovis_cass_max_column_family_num() {
  return clovis_cass_max_column_family_num;
}

std::string S3Option::get_auth_ip_addr() {
  return auth_ip_addr;
}

void S3Option::disable_auth() {
  FLAGS_disable_auth = true;
}

bool S3Option::is_auth_disabled() {
  return FLAGS_disable_auth;
}

unsigned short S3Option::s3_performance_enabled() {
  return perf_enabled;
}

bool S3Option::is_fake_clovis_createobj() {
  return FLAGS_fake_clovis_createobj;
}

bool S3Option::is_fake_clovis_writeobj() {
  return FLAGS_fake_clovis_writeobj;
}

bool S3Option::is_fake_clovis_deleteobj() {
  return FLAGS_fake_clovis_deleteobj;
}

bool S3Option::is_fake_clovis_createidx() {
  return FLAGS_fake_clovis_createidx;
}

bool S3Option::is_fake_clovis_deleteidx() {
  return FLAGS_fake_clovis_deleteidx;
}

bool S3Option::is_fake_clovis_getkv() {
  return FLAGS_fake_clovis_getkv;
}

bool S3Option::is_fake_clovis_putkv() {
  return FLAGS_fake_clovis_putkv;
}

bool S3Option::is_fake_clovis_deletekv() {
  return FLAGS_fake_clovis_deletekv;
}

unsigned short S3Option::get_max_retry_count() {
  return max_retry_count;
}

unsigned short S3Option::get_retry_interval_in_millisec() {
  return retry_interval_millisec;
}

void S3Option::set_eventbase(evbase_t* base) {
  eventbase = base;
}

evbase_t* S3Option::get_eventbase() {
  return eventbase;
}

void S3Option::enable_fault_injection() { FLAGS_fault_injection = true; }

bool S3Option::is_fi_enabled() { return FLAGS_fault_injection; }
