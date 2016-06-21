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

#ifndef __MERO_FE_S3_SERVER_S3_API_HANDLER_H__
#define __MERO_FE_S3_SERVER_S3_API_HANDLER_H__

#include <memory>

#include "s3_common.h"
#include "s3_request_object.h"

class S3APIHandler {
protected:
  std::shared_ptr<S3RequestObject> request;
  S3OperationCode operation_code;

private:
  std::shared_ptr<S3APIHandler> self_ref;

public:
  S3APIHandler(std::shared_ptr<S3RequestObject> req, S3OperationCode op_code) : request(req), operation_code(op_code) {}
  virtual ~S3APIHandler() {}

  virtual void dispatch() = 0;

  // Self destructing object.
  void manage_self(std::shared_ptr<S3APIHandler> ref) {
      self_ref = ref;
  }
  // This *MUST* be the last thing on object. Called @ end of dispatch.
  void i_am_done() {
    self_ref.reset();
  }
};

class S3ServiceAPIHandler : public S3APIHandler {
public:
  S3ServiceAPIHandler(std::shared_ptr<S3RequestObject> req, S3OperationCode op_code) : S3APIHandler(req, op_code) {}

  virtual void dispatch();
};

class S3BucketAPIHandler : public S3APIHandler {
public:
  S3BucketAPIHandler(std::shared_ptr<S3RequestObject> req, S3OperationCode op_code) : S3APIHandler(req, op_code) {}

  virtual void dispatch();
};

class S3ObjectAPIHandler : public S3APIHandler {
public:
  S3ObjectAPIHandler(std::shared_ptr<S3RequestObject> req, S3OperationCode op_code) : S3APIHandler(req, op_code) {}

  virtual void dispatch();
};

class S3FaultinjectionAPIHandler : public S3APIHandler {
 public:
  S3FaultinjectionAPIHandler(std::shared_ptr<S3RequestObject> req,
                             S3OperationCode op_code)
      : S3APIHandler(req, op_code) {}

  virtual void dispatch();
};

class S3APIHandlerFactory {
public:
  virtual ~S3APIHandlerFactory() {}

  virtual std::shared_ptr<S3APIHandler> create_api_handler(
        S3ApiType api_type,
        std::shared_ptr<S3RequestObject> request,
        S3OperationCode op_code);
};

#endif
