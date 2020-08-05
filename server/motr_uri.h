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

#ifndef __S3_SERVER_MOTR_URI_H__
#define __S3_SERVER_MOTR_URI_H__

#include <memory>
#include <string>

#include "s3_common.h"
#include "motr_request_object.h"

enum class MotrUriType {
  path_style,
  unsupported
};

class MotrURI {
 protected:
  std::shared_ptr<MotrRequestObject> request;

  std::string key_name;
  std::string index_id_lo;
  std::string index_id_hi;
  std::string object_oid_lo;
  std::string object_oid_hi;

  MotrOperationCode operation_code;
  MotrApiType motr_api_type;

  std::string request_id;

 private:
  void setup_operation_code();

 public:
  explicit MotrURI(std::shared_ptr<MotrRequestObject> req);
  virtual ~MotrURI() {}

  virtual MotrApiType get_motr_api_type();
  virtual std::string& get_key_name();
  virtual std::string& get_object_oid_lo();
  virtual std::string& get_object_oid_hi();
  virtual std::string& get_index_id_lo();
  virtual std::string& get_index_id_hi();
  virtual MotrOperationCode get_operation_code();
};

class MotrPathStyleURI : public MotrURI {
 public:
  explicit MotrPathStyleURI(std::shared_ptr<MotrRequestObject> req);
};

class MotrUriFactory {
 public:
  virtual ~MotrUriFactory() {}

  virtual MotrURI* create_uri_object(
      MotrUriType uri_type, std::shared_ptr<MotrRequestObject> request);
};

#endif
