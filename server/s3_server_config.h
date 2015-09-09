#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_SERVER_CONFIG_H__
#define __MERO_FE_S3_SERVER_S3_SERVER_CONFIG_H__

#include <string>
#include <set>

// We should load this from config file.
// Not thread-safe, but we are single threaded.
class S3Config {
private:
  static S3Config* instance;
  S3Config();
  // S3Config(std::string config_file);

  // Config items.
  std::string default_endpoint;
  std::set<std::string> region_endpoints;

public:
  static S3Config* get_instance();

  // Config items
  std::string& get_default_endpoint();
  std::set<std::string>& get_region_endpoints();
};

#endif
