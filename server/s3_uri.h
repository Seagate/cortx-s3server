#pragma once

#ifndef __S3_SERVER_S3_URI_H__
#define __S3_SERVER_S3_URI_H__

#include <memory>
#include <string>

#include "s3_common.h"
#include "s3_request_object.h"

enum class S3UriType { path_style, virtual_host_style, unsupported };

class S3URI {
 protected:
  std::shared_ptr<S3RequestObject> request;

  S3OperationCode operation_code;
  S3ApiType s3_api_type;

  std::string bucket_name;
  std::string object_name;
  std::string request_id;

 private:
  void setup_operation_code();

 public:
  explicit S3URI(std::shared_ptr<S3RequestObject> req);
  virtual ~S3URI() {}

  virtual S3ApiType get_s3_api_type();

  virtual std::string& get_bucket_name();
  virtual std::string& get_object_name();  // Object key

  virtual S3OperationCode get_operation_code();
};

class S3PathStyleURI : public S3URI {
 public:
  explicit S3PathStyleURI(std::shared_ptr<S3RequestObject> req);
};

class S3VirtualHostStyleURI : public S3URI {
  std::string host_header;

  // helper
  void setup_bucket_name();

 public:
  explicit S3VirtualHostStyleURI(std::shared_ptr<S3RequestObject> req);
};

class S3UriFactory {
 public:
  virtual ~S3UriFactory() {}

  virtual S3URI* create_uri_object(S3UriType uri_type,
                                   std::shared_ptr<S3RequestObject> request);
};

#endif
