
#include "s3_clovis_config.h"

ClovisConfig* ClovisConfig::instance = NULL;

ClovisConfig::ClovisConfig() {
  // Read config items from some file.
  clovis_block_size = 4096;
}

ClovisConfig* ClovisConfig::instance() {
  if(!instance){
    instance = new ClovisConfig();
  }
  return instance;
}

size_t ClovisConfig::clovis_block_size() {
    return clovis_block_size;
}
