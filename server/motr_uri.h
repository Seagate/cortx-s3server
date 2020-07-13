/*
 * COPYRIGHT 2019 SEAGATE LLC
 *
 * THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
 * HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE TECHNOLOGY
 * LIMITED, ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
 * THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
 * BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
 * USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
 * EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.
 *
 * YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
 * THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
 * http://www.seagate.com/contact
 *
 * Original author:  Prashanth Vanaparthy   <prashanth.vanaparthy@seagate.com>
 * Original creation date: 1-Oct-2019
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
