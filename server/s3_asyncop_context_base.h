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
#include "s3_timer.h"

class S3AsyncOpResponse {
public:
  S3AsyncOpResponse() {
    status = S3AsyncOpStatus::unknown;
    error_message = "";
    error_code = 0;
  }

  S3AsyncOpStatus status;
  std::string error_message;
  int error_code;  // this is same as Mero/Clovis errors
};

class S3AsyncOpContextBase {
  std::shared_ptr<S3RequestObject> request;
  std::function<void(void)> on_success;
  std::function<void(void)> on_failed;

  // Operational details
  // When multiple operations are invoked in a single launch
  // this indicates the status in order with tuple
  // <return_code, message>
  std::vector<S3AsyncOpResponse> ops_response;

  int ops_count;
  int response_received_count;
  bool at_least_one_success;

  // To measure performance
  S3Timer timer;
  std::string operation_key;  // used to identify operation(metric) name
public:
  S3AsyncOpContextBase(std::shared_ptr<S3RequestObject> req, std::function<void(void)> success, std::function<void(void)> failed, int ops_cnt = 1);
  virtual ~S3AsyncOpContextBase() {}

  std::shared_ptr<S3RequestObject> get_request();

  std::function<void(void)> on_success_handler();
  std::function<void(void)> on_failed_handler();

  S3AsyncOpStatus get_op_status_for(int op_idx);
  int get_errno_for(int op_idx);

  void set_op_status_for(int op_idx, S3AsyncOpStatus opstatus, std::string message);
  void set_op_errno_for(int op_idx, int err);

  std::string& get_error_message_for(int op_idx);

  int incr_response_count() {
    return ++response_received_count;
  }

  int get_ops_count() {
    return ops_count;
  }

  bool is_at_least_one_op_successful() {
    return at_least_one_success;
  }
  // virtual void consume(char* chars, size_t length) = 0;

  void start_timer_for(std::string op_key);
  void stop_timer(bool success = true);  // arg indicates success/failed metric
  // Call the logging always on main thread, so we dont need synchronisation of log file.
  void log_timer();
};

#endif
