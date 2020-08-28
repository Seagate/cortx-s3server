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

#include <cerrno>
#include "s3_asyncop_context_base.h"
#include "s3_perf_logger.h"
#include "s3_stats.h"
#include "s3_log.h"

S3AsyncOpContextBase::S3AsyncOpContextBase(std::shared_ptr<RequestObject> req,
                                           std::function<void(void)> success,
                                           std::function<void(void)> failed,
                                           int ops_cnt,
                                           std::shared_ptr<MotrAPI> motr_api)
    : request(std::move(req)),
      on_success(success),
      on_failed(failed),
      ops_count(ops_cnt),
      response_received_count(0),
      at_least_one_success(false),
      s3_motr_api(motr_api ? std::move(motr_api)
                           : std::make_shared<ConcreteMotrAPI>()) {
  request_id = request->get_request_id();
  ops_response.resize(ops_count);
}

void S3AsyncOpContextBase::reset_callbacks(std::function<void(void)> success,
                                           std::function<void(void)> failed) {
  on_success = success;
  on_failed = failed;
  return;
}

std::shared_ptr<RequestObject> S3AsyncOpContextBase::get_request() {
  return request;
}

std::shared_ptr<MotrAPI> S3AsyncOpContextBase::get_motr_api() {
  return s3_motr_api;
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
  if (op_idx < 0 || op_idx >= ops_count) {
    s3_log(S3_LOG_ERROR, request_id, "op_idx %d is out of range\n", op_idx);
    return -ENOENT;
  }
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
  LOG_PERF((operation_key + "_ms").c_str(), request_id.c_str(),
           timer.elapsed_time_in_millisec());

  s3_stats_timing(operation_key, timer.elapsed_time_in_millisec());
}
