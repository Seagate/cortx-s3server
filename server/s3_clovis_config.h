
#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_CLOVIS_CONFIG_H__
#define __MERO_FE_S3_SERVER_S3_CLOVIS_CONFIG_H__

// We should load this from config file.
// Not thread-safe, but we are single threaded.
class ClovisConfig {
private:
  static ClovisConfig* instance;
  ClovisConfig();
  // ClovisConfig(std::string config_file);

  // Config items.
  size_t clovis_block_size;

public:
  static ClovisConfig* instance();

  // Config items
  size_t clovis_block_size();
};

#endif
