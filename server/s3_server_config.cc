
#include "s3_server_config.h"

S3Config* S3Config::instance = NULL;

S3Config::S3Config() {
  // Read config items from some file.
  default_endpoint = "s3.seagate.com";  // Default region endpoint (US)
  // TODO - load region endpoints from config
  std::string eps[] = {"s3-us.seagate.com", "s3-europe.seagate.com", "s3-asia.seagate.com"};
  for( int i = 0; i < 3; i++) {
    region_endpoints.insert(eps[i]);
  }
}

S3Config* S3Config::get_instance() {
  if(!instance){
    instance = new S3Config();
  }
  return instance;
}

std::string& S3Config::get_default_endpoint() {
    return default_endpoint;
}

std::set<std::string>& S3Config::get_region_endpoints() {
    return region_endpoints;
}
