
#include <string>

#include "s3_uri.h"
#include "s3_server_config.h"

S3URI::S3URI(std::shared_ptr<S3RequestObject> req) : request(req), operation_code(S3OperationCode::none), bucket_name(""), object_name(""), service_api(false), bucket_api(false), object_api(false) {
  setup_operation_code();
}

bool S3URI::is_service_api() {
  return service_api;
}

bool S3URI::is_bucket_api() {
  return bucket_api;
}

bool S3URI::is_object_api() {
  return object_api;
}

std::string& S3URI::get_bucket_name() {
  return bucket_name;
}

std::string& S3URI::get_object_name() {
  return object_name;
}

S3OperationCode S3URI::get_operation_code() {
  return operation_code;
}

void S3URI::setup_operation_code() {
  if (request->has_query_param_key("acl")) {
    operation_code = S3OperationCode::acl;
  } else if (request->has_query_param_key("location")) {
    operation_code = S3OperationCode::location;
  }
  // Other operations - todo
}

S3PathStyleURI::S3PathStyleURI(std::shared_ptr<S3RequestObject> req) : S3URI(req) {
  std::string full_path(request->c_get_full_path());
  printf("full_path = %s\n", full_path.c_str());
  // Regex is better, but lets live with raw parsing. regex = >gcc 4.9.0
  if (full_path.compare("/") == 0) {
    service_api = true;
  } else {
    // Find the second forward slash.
    std::size_t pos = full_path.find("/", 1);  // ignoring the first forward slash
    if (pos == std::string::npos) {
      // no second slash, means only bucket name.
      bucket_name = std::string(full_path.c_str() + 1);
      bucket_api = true;
    } else if (pos == full_path.length() - 1) {
      // second slash and its last char, means only bucket name.
      bucket_name = std::string(full_path.c_str() + 1, full_path.length() - 2);
      bucket_api = true;
    } else  {
      // Its an object api.
      object_api = true;
      bucket_name = std::string(full_path.c_str() + 1, pos - 1);
      object_name = std::string(full_path.c_str() + pos + 1);
      if (object_name.back() == '/') {
        object_name.pop_back();  // ignore last slash
      }
    }
  }
}

S3VirtualHostStyleURI::S3VirtualHostStyleURI(std::shared_ptr<S3RequestObject> req) : S3URI(req) {
    host_header = request->get_host_header();
    setup_bucket_name();

    std::string full_path(request->c_get_full_path());
    if (full_path.compare("/") == 0) {
      bucket_api = true;
    } else {
      object_api = true;
      object_name = std::string(full_path.c_str() + 1);  // ignore first slash
      if (object_name.back() == '/') {
        object_name.pop_back();  // ignore last slash
      }
    }
}

void S3VirtualHostStyleURI::setup_bucket_name() {
  if (host_header.find(S3Config::get_instance()->get_default_endpoint()) != std::string::npos) {
    bucket_name = host_header.substr(0, (host_header.length() - S3Config::get_instance()->get_default_endpoint().length() - 1));
  }
  for (std::set<std::string>::iterator it = S3Config::get_instance()->get_region_endpoints().begin(); it != S3Config::get_instance()->get_region_endpoints().end(); ++it) {
    if (host_header.find(*it) != std::string::npos) {
      bucket_name = host_header.substr(0, (host_header.length() - (*it).length() - 1));
    }
  }
}
