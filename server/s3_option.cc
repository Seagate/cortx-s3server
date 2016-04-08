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

bool S3Option::load_section(std::string section_name, bool selective_load=false) {
  YAML::Node root_node = YAML::LoadFile(option_file);
  if (root_node.IsNull()) {
    return false; //File Not Found?
  }
  YAML::Node s3_option_node = root_node[section_name];
  if (s3_option_node.IsNull()) {
    return false;
  }
  if (!selective_load) {
    if (section_name == "S3_SERVER_CONFIG") {
      s3config_bind_port = s3_option_node["S3_SERVER_BIND_PORT"].as<unsigned short>();
      s3config_log_filename = s3_option_node["S3_LOG_FILENAME"].as<std::string>();
      s3config_log_level = s3_option_node["S3_LOG_MODE"].as<std::string>();
      s3config_bind_addr = s3_option_node["S3_SERVER_BIND_ADDR"].as<std::string>();
      s3config_performance_enabled = s3_option_node["S3_ENABLE_PERF"].as<unsigned short>();
      for (unsigned short i = 0; i < s3_option_node["S3_SERVER_REGION_ENDPOINTS"].size(); ++i) {
        s3config_region_endpoints.insert(s3_option_node["S3_SERVER_REGION_ENDPOINTS"][i].as<std::string>());
      }
    } else if (section_name == "S3_AUTH_CONFIG") {
      s3config_auth_port = s3_option_node["S3_AUTH_PORT"].as<unsigned short>();
      s3config_auth_ip_addr = s3_option_node["S3_AUTH_IP_ADDR"].as<std::string>();
    } else if (section_name == "S3_CLOVIS_CONFIG") {
      s3config_clovis_local_addr = s3_option_node["S3_CLOVIS_LOCAL_ADDR"].as<std::string>();
      s3config_clovis_confd_addr = s3_option_node["S3_CLOVIS_CONFD_ADDR"].as<std::string>();
      s3config_clovis_ha_addr = s3_option_node["S3_CLOVIS_HA_ADDR"].as<std::string>();
      s3config_clovis_prof = s3_option_node["S3_CLOVIS_PROF"].as<std::string>();
      s3config_clovis_layout = s3_option_node["S3_CLOVIS_LAYOUT_ID"].as<unsigned short>();
      s3config_clovis_block_size = s3_option_node["S3_CLOVIS_BLOCK_SIZE"].as<int>();
      s3config_factor = s3_option_node["S3_CLOVIS_MAX_BLOCKS_PER_REQUEST"].as<unsigned short>();
      s3config_clovis_idx_fetch_count = s3_option_node["S3_CLOVIS_MAX_IDX_FETCH_COUNT"].as<int>();
    }
  } else {
    if (section_name == "S3_SERVER_CONFIG") {
      if (!(s3command_option & S3_OPTION_BIND_PORT)) {
        s3config_bind_port = s3_option_node["S3_SERVER_BIND_PORT"].as<unsigned short>();
      }
      if (!(s3command_option & S3_OPTION_LOG_FILE)) {
        s3config_log_filename = s3_option_node["S3_LOG_FILENAME"].as<std::string>();
      }
      if (!(s3command_option & S3_OPTION_LOG_MODE)) {
        s3config_log_level = s3_option_node["S3_LOG_MODE"].as<std::string>();
      }
      if (!(s3command_option & S3_OPTION_BIND_ADDR)) {
        s3config_bind_addr = s3_option_node["S3_SERVER_BIND_ADDR"].as<std::string>();
      }
      for (unsigned short i = 0; i < s3_option_node["S3_SERVER_REGION_ENDPOINTS"].size(); ++i) {
        s3config_region_endpoints.insert(s3_option_node["S3_SERVER_REGION_ENDPOINTS"][i].as<std::string>());
      }
    } else if (section_name == "S3_AUTH_CONFIG") {
      if (!(s3command_option & S3_OPTION_AUTH_PORT)) {
        s3config_auth_port = s3_option_node["S3_AUTH_PORT"].as<unsigned short>();
      }
      if (!(s3command_option & S3_OPTION_AUTH_IP_ADDR)) {
        s3config_auth_ip_addr = s3_option_node["S3_AUTH_IP_ADDR"].as<std::string>();
      }
    } else if (section_name == "S3_CLOVIS_CONFIG") {
      if (!(s3command_option & S3_OPTION_CLOVIS_LOCAL_ADDR)) {
        s3config_clovis_local_addr = s3_option_node["S3_CLOVIS_LOCAL_ADDR"].as<std::string>();
      }
      if (!(s3command_option & S3_OPTION_CLOVIS_CONFD_ADDR)) {
        s3config_clovis_confd_addr = s3_option_node["S3_CLOVIS_CONFD_ADDR"].as<std::string>();
      }
      if (!(s3command_option & S3_OPTION_CLOVIS_HA_ADDR)) {
        s3config_clovis_ha_addr = s3_option_node["S3_CLOVIS_HA_ADDR"].as<std::string>();
      }
      if (!(s3command_option & S3_CLOVIS_LAYOUT_ID)) {
        s3config_clovis_layout = s3_option_node["S3_CLOVIS_LAYOUT_ID"].as<unsigned short>();
      }
      s3config_clovis_prof = s3_option_node["S3_CLOVIS_PROF"].as<std::string>();
      s3config_clovis_block_size = s3_option_node["S3_CLOVIS_BLOCK_SIZE"].as<int>();
      s3config_factor = s3_option_node["S3_CLOVIS_MAX_BLOCKS_PER_REQUEST"].as<unsigned short>();
      s3config_clovis_idx_fetch_count = s3_option_node["S3_CLOVIS_MAX_IDX_FETCH_COUNT"].as<int>();
    }
  }
 return true;
}

bool S3Option::load_all_sections(bool selective_load=false) {
  std::string s3_section;
  YAML::Node root_node = YAML::LoadFile(option_file);
  if (root_node.IsNull()) {
     return false; //File Not Found?
  }
  for (unsigned short i = 0; i < root_node["S3Config_Sections"].size(); ++i) {
    load_section(root_node["S3Config_Sections"][i].as<std::string>(), selective_load);
  }
  return true;
}

void S3Option::set_cmdline_option(int option_flag, char *optarg) {
  if (option_flag & S3_OPTION_BIND_ADDR) {
    s3config_bind_addr = optarg;
  } else if (option_flag & S3_OPTION_BIND_PORT) {
    s3config_bind_port = atoi(optarg);
  } else if (option_flag & S3_OPTION_CLOVIS_LOCAL_ADDR) {
    s3config_clovis_local_addr = optarg;
  } else if (option_flag & S3_OPTION_CLOVIS_CONFD_ADDR) {
    s3config_clovis_confd_addr = optarg;
  } else if (option_flag & S3_OPTION_CLOVIS_HA_ADDR) {
    s3config_clovis_ha_addr = optarg;
  } else if (option_flag & S3_OPTION_AUTH_IP_ADDR) {
    s3config_auth_ip_addr = optarg;
  } else if (option_flag & S3_OPTION_AUTH_PORT) {
    s3config_auth_port = atoi(optarg);
  } else if (option_flag & S3_CLOVIS_LAYOUT_ID) {
    s3config_clovis_layout = atoi(optarg);
  } else if (option_flag & S3_OPTION_LOG_FILE) {
    s3config_log_filename = optarg;
  } else if (option_flag & S3_OPTION_LOG_MODE) {
    s3config_log_level = optarg;
  }
  s3command_option |= option_flag;
  return;
}

int S3Option::get_s3command_option() {
  return s3command_option;
}

void S3Option::dump_options() {
  s3_log(S3_LOG_INFO, "S3_LOG_FILENAME = %s\n", s3config_log_filename.c_str());
  s3_log(S3_LOG_INFO, "S3_LOG_MODE = %s\n", s3config_log_level.c_str());
  s3_log(S3_LOG_INFO, "S3_SERVER_BIND_ADDR = %s\n", s3config_bind_addr.c_str());
  s3_log(S3_LOG_INFO, "S3_SERVER_BIND_PORT = %d\n", s3config_bind_port);
  s3_log(S3_LOG_INFO, "S3_ENABLE_PERF = %d\n", s3config_performance_enabled);
  s3_log(S3_LOG_INFO, "S3_AUTH_IP_ADDR = %s\n", s3config_auth_ip_addr.c_str());
  s3_log(S3_LOG_INFO, "S3_AUTH_PORT = %d\n", s3config_auth_port);
  s3_log(S3_LOG_INFO, "S3_CLOVIS_LOCAL_ADDR = %s\n", s3config_clovis_local_addr.c_str());
  s3_log(S3_LOG_INFO, "S3_CLOVIS_CONFD_ADDR = %s\n", s3config_clovis_confd_addr.c_str());
  s3_log(S3_LOG_INFO, "S3_CLOVIS_HA_ADDR =  %s\n", s3config_clovis_ha_addr.c_str());
  s3_log(S3_LOG_INFO, "S3_CLOVIS_PROF = %s\n", s3config_clovis_prof.c_str());
  s3_log(S3_LOG_INFO, "S3_CLOVIS_LAYOUT_ID = %d\n", s3config_clovis_layout);
  s3_log(S3_LOG_INFO, "S3_CLOVIS_BLOCK_SIZE = %d\n", s3config_clovis_block_size);
  s3_log(S3_LOG_INFO, "S3_CLOVIS_MAX_BLOCKS_PER_REQUEST = %d\n", s3config_factor);
  s3_log(S3_LOG_INFO, "S3_CLOVIS_MAX_IDX_FETCH_COUNT = %d\n", s3config_clovis_idx_fetch_count);
  return;
}

unsigned short S3Option::get_s3_bind_port() {
  return s3config_bind_port;
}

unsigned short S3Option::get_auth_port() {
  return s3config_auth_port;
}

unsigned short S3Option::get_clovis_layout() {
  return s3config_clovis_layout;
}

std::string S3Option::get_option_file() {
  return option_file;
}

std::string S3Option::get_log_filename() {
  return s3config_log_filename;
}

std::string S3Option::get_log_level() {
  return s3config_log_level;
}

std::string S3Option::get_bind_addr() {
  return s3config_bind_addr;
}

std::string S3Option::get_default_endpoint() {
    return s3config_default_endpoint;
}

std::set<std::string>& S3Option::get_region_endpoints() {
    return s3config_region_endpoints;
}

std::string S3Option::get_clovis_local_addr() {
  return s3config_clovis_local_addr;
}

std::string S3Option::get_clovis_confd_addr() {
  return s3config_clovis_confd_addr;
}

std::string S3Option::get_clovis_ha_addr() {
  return s3config_clovis_ha_addr;
}

std::string S3Option::get_clovis_prof() {
  return s3config_clovis_prof;
}

int S3Option::get_clovis_block_size() {
  return s3config_clovis_block_size;
}

unsigned short S3Option::get_clovis_factor() {
  return s3config_factor;
}

int S3Option::get_clovis_idx_fetch_count() {
  return s3config_clovis_idx_fetch_count;
}

int S3Option::get_clovis_write_payload_size() {
  return s3config_clovis_block_size * s3config_factor;
}

int S3Option::get_clovis_read_payload_size() {
  return s3config_clovis_block_size * s3config_factor;
}

std::string S3Option::get_auth_ip_addr() {
  return s3config_auth_ip_addr;
}

unsigned short S3Option::s3_performance_enabled() {
  return s3config_performance_enabled;
}
