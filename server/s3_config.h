
#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_CONFIG_H__
#define __MERO_FE_S3_SERVER_S3_CONFIG_H__

#include <cstddef>

// We should load this from config file.
// Not thread-safe, but we are single threaded.
class S3Config {
private:
  static S3Config* instance;
  S3Config();
  // S3Config(std::string config_file);

  // Config items.
  size_t clovis_block_size;

  size_t clovis_idx_fetch_count;

public:
  static S3Config* get_instance();

  // Config items
  size_t get_clovis_block_size();

  // Number of rows to fetch from clovis idx in single call
  size_t get_clovis_idx_fetch_count();
};

#endif
