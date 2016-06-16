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
#define S3_OPTION_PERF_LOG_FILE 0x0400

#include <string>
#include <set>

#include "s3_cli_options.h"

class S3Option {
  int cmd_opt_flag;

  std::string s3_bind_addr;
  unsigned short s3_bind_port;

  std::string auth_ip_addr;
  unsigned short auth_port;

  std::string s3_default_endpoint;
  std::set<std::string> s3_region_endpoints;

  unsigned short perf_enabled;
  std::string perf_log_file;

  std::string option_file;
  std::string log_filename;
  int read_ahead_multiple;
  std::string log_level;

  unsigned short clovis_layout_id;
  unsigned short clovis_factor;
  unsigned int clovis_block_size;
  int clovis_idx_fetch_count;
  std::string clovis_local_addr;
  std::string clovis_confd_addr;
  std::string clovis_ha_addr;
  std::string clovis_profile;

  std::string s3_daemon_dir;
  unsigned short s3_daemon_redirect;

  static S3Option* option_instance;

public:
  S3Option() {
    cmd_opt_flag = 0;

    s3_bind_addr = FLAGS_s3host;
    s3_bind_port = FLAGS_s3port;

    read_ahead_multiple = 1;

    s3_default_endpoint = "s3.seagate.com";
    s3_region_endpoints.insert("s3-us.seagate.com");
    s3_region_endpoints.insert("s3-europe.seagate.com");
    s3_region_endpoints.insert("s3-asia.seagate.com");

    log_filename = FLAGS_s3logfile;
    log_level = FLAGS_s3loglevel;

    clovis_layout_id = FLAGS_clovislayoutid;
    clovis_local_addr = FLAGS_clovislocal;
    clovis_confd_addr = FLAGS_clovisconfd;
    clovis_ha_addr = FLAGS_clovisha;
    clovis_profile = FLAGS_clovisprofile;

    auth_ip_addr = FLAGS_authhost;
    auth_port = FLAGS_authport;

    option_file = "/opt/seagate/s3/conf/s3config.yaml";

    s3_daemon_dir = "/";
    s3_daemon_redirect = 1;

    perf_enabled = FLAGS_perfenable;
    perf_log_file = FLAGS_perflogfile;

    clovis_block_size = 1048576; // One MB
    clovis_factor = 1;
    clovis_idx_fetch_count = 100;
  }

  bool load_section(std::string section_name, bool force_override_from_config);
  bool load_all_sections(bool force_override_from_config);

  std::string get_bind_addr();
  unsigned short get_s3_bind_port();

  int get_read_ahead_multiple();
  std::string get_default_endpoint();
  std::set<std::string>& get_region_endpoints();

  std::string get_auth_ip_addr();
  unsigned short get_auth_port();
  void disable_auth();
  bool is_auth_disabled();

  std::string get_option_file();
  void set_option_file(std::string filename);

  std::string get_daemon_dir();
  unsigned short do_redirection();
  void set_daemon_dir(std::string path);
  void set_redirection(unsigned short redirect);

  std::string get_log_filename();
  std::string get_log_level();

  unsigned short s3_performance_enabled();
  std::string get_perf_log_filename();

  std::string get_clovis_local_addr();
  std::string get_clovis_confd_addr();
  std::string get_clovis_ha_addr();
  std::string get_clovis_prof();
  unsigned short get_clovis_layout_id();
  unsigned int get_clovis_block_size();
  unsigned short get_clovis_factor();
  unsigned int get_clovis_write_payload_size();
  unsigned int get_clovis_read_payload_size();
  int get_clovis_idx_fetch_count();

  void set_cmdline_option(int option_flag, const char *option);
  int get_cmd_opt_flag();

  // Check if any fake out options are provided.
  bool is_fake_clovis_createobj();
  bool is_fake_clovis_writeobj();
  bool is_fake_clovis_deleteobj();
  bool is_fake_clovis_createidx();
  bool is_fake_clovis_deleteidx();
  bool is_fake_clovis_getkv();
  bool is_fake_clovis_putkv();
  bool is_fake_clovis_deletekv();

  void dump_options();

  static S3Option *get_instance()  {
    if (!option_instance){
      option_instance = new S3Option();
    }
    return option_instance;
  }

  static void destroy_instance() {
    if (option_instance) {
      delete option_instance;
      option_instance = NULL;
    }
  }
};
#endif
