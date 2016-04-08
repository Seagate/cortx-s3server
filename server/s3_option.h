/*
 * COPYRIGHT 2015 SEAGATE LLC
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
 * Original author:  Rajesh Nambiar   <rajesh.nambiar@seagate.com>
 * Original creation date: 16-March-2016
 */
#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_OPTION_H__
#define __MERO_FE_S3_SERVER_S3_OPTION_H__

#define S3_OPTION_BIND_ADDR 0x0001
#define S3_OPTION_BIND_PORT 0x0002
#define S3_OPTION_CLOVIS_LOCAL_ADDR 0x0004
#define S3_OPTION_CLOVIS_CONFD_ADDR 0x0008
#define S3_OPTION_CLOVIS_HA_ADDR 0x0010
#define S3_OPTION_AUTH_IP_ADDR 0x0020
#define S3_OPTION_AUTH_PORT 0x0040
#define S3_CLOVIS_LAYOUT_ID 0x0080
#define S3_OPTION_LOG_FILE 0x0100
#define S3_OPTION_LOG_MODE 0x0200

#include <string>
#include <set>

class S3Option {
  int s3command_option;
  unsigned short s3config_bind_port;
  unsigned short s3config_auth_port;
  std::string s3config_default_endpoint;
  std::set<std::string> s3config_region_endpoints;
  unsigned short s3config_clovis_layout;
  unsigned short s3config_performance_enabled;
  unsigned short s3config_factor;
  int s3config_clovis_block_size;
  int s3config_clovis_idx_fetch_count;
  std::string option_file;
  std::string s3config_log_filename;
  std::string s3config_log_level;
  std::string s3config_bind_addr;
  std::string s3config_clovis_local_addr;
  std::string s3config_clovis_confd_addr;
  std::string s3config_clovis_ha_addr;
  std::string s3config_clovis_prof;
  std::string s3config_auth_ip_addr;
  static S3Option* option_instance;
public:
  S3Option() {
    s3config_log_filename = "/var/log/seagate/s3/s3server.log";
    s3config_log_level = "INFO";
    s3config_bind_addr = "0.0.0.0";
    s3config_default_endpoint = "s3.seagate.com";
    s3config_auth_ip_addr = "127.0.0.1";
    s3config_clovis_layout = 9;
    s3config_clovis_local_addr = s3config_clovis_confd_addr = "<ipaddress>@tcp:12345:33:100";
    s3config_clovis_ha_addr = "<ipaddress>@tcp:12345:34:1";
    s3config_clovis_prof = "<0x7000000000000001:0>";
    s3config_bind_port = 8081;
    s3config_auth_port = 8085;
    s3command_option = 0;
    option_file = "/opt/seagate/s3conf/s3config.yaml";
    s3config_performance_enabled = 0;
    s3config_clovis_block_size = 1048576; // One MB
    s3config_factor = 1;
    s3config_clovis_idx_fetch_count = 100;
  }

  bool load_section(std::string section_name, bool selective_load);
  bool load_all_sections(bool selective_load);
  unsigned short get_s3_bind_port();
  unsigned short get_auth_port();
  unsigned short get_clovis_layout();
  std::string get_option_file();
  std::string get_log_filename();
  std::string get_log_level();
  std::string get_bind_addr();
  std::string get_default_endpoint();
  std::set<std::string>& get_region_endpoints();
  std::string get_clovis_local_addr();
  std::string get_clovis_confd_addr();
  std::string get_clovis_ha_addr();
  std::string get_clovis_prof();
  int get_clovis_block_size();
  unsigned short get_clovis_factor();
  int get_clovis_write_payload_size();
  int get_clovis_read_payload_size();
  int get_clovis_idx_fetch_count();
  std::string get_auth_ip_addr();
  unsigned short s3_performance_enabled();
  void set_cmdline_option(int option_flag, char *option);
  int get_s3command_option();
  void dump_options();

  static S3Option *get_instance()  {
    if (!option_instance){
      option_instance = new S3Option();
    }
    return option_instance;
  }
};
#endif
