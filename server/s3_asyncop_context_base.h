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

#ifndef __MERO_FE_S3_SERVER_S3_ASYNCOP_CONTEXT_BASE_H__
#define __MERO_FE_S3_SERVER_S3_ASYNCOP_CONTEXT_BASE_H__

#include <functional>
#include <memory>
#include <string>

#include "s3_common.h"
#include "s3_request_object.h"

class S3AsyncOpContextBase {
  std::shared_ptr<S3RequestObject> request;
  std::function<void(void)> on_success;
  std::function<void(void)> on_failed;

  // Operational details
  S3AsyncOpStatus status;
  std::string error_message;
  int error_code;  // this is same as Mero/Clovis errors
public:
  S3AsyncOpContextBase(std::shared_ptr<S3RequestObject> req, std::function<void(void)> success, std::function<void(void)> failed);
  virtual ~S3AsyncOpContextBase() {}

  std::shared_ptr<S3RequestObject> get_request();

  std::function<void(void)> on_success_handler();
  std::function<void(void)> on_failed_handler();

  S3AsyncOpStatus get_op_status();
  int get_errno();

  void set_op_status(S3AsyncOpStatus opstatus, std::string message);
  void set_op_errno(int err);

  std::string& get_error_message();

  // virtual void consume(char* chars, size_t length) = 0;
};

#endif
