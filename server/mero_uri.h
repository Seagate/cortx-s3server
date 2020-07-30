/*
 * Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For any questions about this software or licensing,
 * please email opensource@seagate.com or cortx-questions@seagate.com.
 *
 */


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
