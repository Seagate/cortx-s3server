
#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_URI_H__
#define __MERO_FE_S3_SERVER_S3_URI_H__

#include <string>
#include <memory>

#include "s3_request_object.h"
#include "s3_common.h"


class S3URI {

protected:
  std::shared_ptr<S3RequestObject> request;

  S3OperationCode operation_code;

  std::string bucket_name;
  std::string object_name;
  bool service_api;
  bool bucket_api;
  bool object_api;

private:
  void setup_operation_code();

public:
  S3URI(std::shared_ptr<S3RequestObject> req);

  bool is_service_api();
  bool is_bucket_api();
  bool is_object_api();

  std::string& get_bucket_name();
  std::string& get_object_name();  // Object key

  S3OperationCode get_operation_code();

};

class S3PathStyleURI : public S3URI {
public:
  S3PathStyleURI(std::shared_ptr<S3RequestObject> req);
};

class S3VirtualHostStyleURI : public S3URI {
  std::string host_header;

  // helper
  void setup_bucket_name();
public:
  S3VirtualHostStyleURI(std::shared_ptr<S3RequestObject> req);
};

#endif
