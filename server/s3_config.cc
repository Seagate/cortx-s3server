
#include "s3_config.h"

S3Config* S3Config::instance = NULL;

S3Config::S3Config() {
  // Read config items from some file.
  clovis_block_size = 4096;

  clovis_idx_fetch_count = 100;
}

S3Config* S3Config::get_instance() {
  if(!instance){
    instance = new S3Config();
  }
  return instance;
}

size_t S3Config::get_clovis_block_size() {
    return clovis_block_size;
}

size_t S3Config::get_clovis_idx_fetch_count() {
  return clovis_idx_fetch_count;
}
