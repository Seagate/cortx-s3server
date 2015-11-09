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

#include "s3_asyncop_context_base.h"

S3AsyncOpContextBase::S3AsyncOpContextBase(std::shared_ptr<S3RequestObject> req, std::function<void(void)> success, std::function<void(void)> failed) : request(req), on_success(success), on_failed(failed), status(S3AsyncOpStatus::unknown), error_message(""), error_code(0) {
}

std::shared_ptr<S3RequestObject> S3AsyncOpContextBase::get_request() {
  return request;
}

std::function<void(void)> S3AsyncOpContextBase::on_success_handler() {
  return on_success;
}

std::function<void(void)> S3AsyncOpContextBase::on_failed_handler() {
  return on_failed;
}

S3AsyncOpStatus S3AsyncOpContextBase::get_op_status() {
  return status;
}

std::string& S3AsyncOpContextBase::get_error_message() {
  return error_message;
}

void S3AsyncOpContextBase::set_op_status(S3AsyncOpStatus opstatus, std::string message) {
  status = opstatus;
  error_message = message;
}

int S3AsyncOpContextBase::get_errno() {
  return error_code;
}

void S3AsyncOpContextBase::set_op_errno(int err) {
  error_code = err;
}
