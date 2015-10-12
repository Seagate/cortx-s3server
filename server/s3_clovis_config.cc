
#include "s3_clovis_config.h"

S3ClovisConfig* S3ClovisConfig::instance = NULL;

S3ClovisConfig::S3ClovisConfig() {
  // Read config items from some file.
  clovis_block_size = 4096;

  clovis_idx_fetch_count = 100;
}

S3ClovisConfig* S3ClovisConfig::get_instance() {
  if(!instance){
    instance = new S3ClovisConfig();
  }
  return instance;
}

size_t S3ClovisConfig::get_clovis_block_size() {
    return clovis_block_size;
}

size_t S3ClovisConfig::get_clovis_idx_fetch_count() {
  return clovis_idx_fetch_count;
}
