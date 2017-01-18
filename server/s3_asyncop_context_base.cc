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
#include "s3_perf_logger.h"
#include "s3_stats.h"

S3AsyncOpContextBase::S3AsyncOpContextBase(std::shared_ptr<S3RequestObject> req,
                                           std::function<void(void)> success,
                                           std::function<void(void)> failed,
                                           int ops_cnt)
    : request(req),
      on_success(success),
      on_failed(failed),
      ops_count(ops_cnt),
      response_received_count(0),
      at_least_one_success(false) {
  ops_response.resize(ops_count);
}

void S3AsyncOpContextBase::reset_callbacks(std::function<void(void)> success,
                                           std::function<void(void)> failed) {
  on_success = success;
  on_failed = failed;
  return;
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

S3AsyncOpStatus S3AsyncOpContextBase::get_op_status_for(int op_idx) {
  return ops_response[op_idx].status;
}

std::string& S3AsyncOpContextBase::get_error_message_for(int op_idx) {
  return ops_response[op_idx].error_message;
}

void S3AsyncOpContextBase::set_op_status_for(int op_idx,
                                             S3AsyncOpStatus opstatus,
                                             std::string message) {
  ops_response[op_idx].status = opstatus;
  ops_response[op_idx].error_message = message;
}

int S3AsyncOpContextBase::get_errno_for(int op_idx) {
  return ops_response[op_idx].error_code;
}

void S3AsyncOpContextBase::set_op_errno_for(int op_idx, int err) {
  ops_response[op_idx].error_code = err;
  if (err == 0) {
    at_least_one_success = true;
  }
}

void S3AsyncOpContextBase::start_timer_for(std::string op_key) {
  operation_key = op_key;
  timer.start();
}

void S3AsyncOpContextBase::stop_timer(bool success) {
  timer.stop();
  if (operation_key.empty()) {
    return;
  }
  if (success) {
    operation_key += "_success";
  } else {
    operation_key += "_failed";
  }
}

void S3AsyncOpContextBase::log_timer() {
  if (operation_key.empty()) {
    return;
  }
  LOG_PERF((operation_key + "_ms").c_str(), timer.elapsed_time_in_millisec());
  s3_stats_timing(operation_key, timer.elapsed_time_in_millisec());
}
