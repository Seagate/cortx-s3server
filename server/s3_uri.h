/*
 * COPYRIGHT 2015 SEAGATE LLC
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
 * Original author:  Kaustubh Deorukhkar   <kaustubh.deorukhkar@seagate.com>
 * Original creation date: 1-Oct-2015
 */

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
