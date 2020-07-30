#pragma once

#ifndef __S3_SERVER_MERO_URI_H__
#define __S3_SERVER_MERO_URI_H__

#include <memory>
#include <string>

#include "s3_common.h"
#include "mero_request_object.h"

enum class MeroUriType {
  path_style,
  unsupported
};

class MeroURI {
 protected:
  std::shared_ptr<MeroRequestObject> request;

  std::string key_name;
  std::string index_id_lo;
  std::string index_id_hi;
  std::string object_oid_lo;
  std::string object_oid_hi;

  MeroOperationCode operation_code;
  MeroApiType mero_api_type;

  std::string request_id;

 private:
  void setup_operation_code();

 public:
  explicit MeroURI(std::shared_ptr<MeroRequestObject> req);
  virtual ~MeroURI() {}

  virtual MeroApiType get_mero_api_type();
  virtual std::string& get_key_name();
  virtual std::string& get_object_oid_lo();
  virtual std::string& get_object_oid_hi();
  virtual std::string& get_index_id_lo();
  virtual std::string& get_index_id_hi();
  virtual MeroOperationCode get_operation_code();
};

class MeroPathStyleURI : public MeroURI {
 public:
  explicit MeroPathStyleURI(std::shared_ptr<MeroRequestObject> req);
};

class MeroUriFactory {
 public:
  virtual ~MeroUriFactory() {}

  virtual MeroURI* create_uri_object(
      MeroUriType uri_type, std::shared_ptr<MeroRequestObject> request);
};

#endif
