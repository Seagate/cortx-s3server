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

#ifndef __S3_SERVER_S3_ASYNCOP_CONTEXT_BASE_H__
#define __S3_SERVER_S3_ASYNCOP_CONTEXT_BASE_H__

#include <gtest/gtest_prod.h>
#include <functional>
#include <memory>
#include <string>

#include "s3_motr_wrapper.h"
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
  int error_code;  // this is same as Motr/Motr errors
};

class S3AsyncOpContextBase {
 protected:
  std::shared_ptr<RequestObject> request;
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
  // Used for mocking motr return calls.
  std::shared_ptr<MotrAPI> s3_motr_api;

  std::string request_id;

 public:
  S3AsyncOpContextBase(std::shared_ptr<RequestObject> req,
                       std::function<void(void)> success,
                       std::function<void(void)> failed, int ops_cnt = 1,
                       std::shared_ptr<MotrAPI> motr_api = nullptr);
  virtual ~S3AsyncOpContextBase() {}

  std::shared_ptr<RequestObject> get_request();

  std::function<void(void)> on_success_handler();
  std::function<void(void)> on_failed_handler();
  void reset_callbacks(std::function<void(void)> success,
                       std::function<void(void)> failed);

  S3AsyncOpStatus get_op_status_for(int op_idx);
  virtual int get_errno_for(int op_idx);

  virtual void set_op_status_for(int op_idx, S3AsyncOpStatus opstatus,
                                 std::string message);
  virtual void set_op_errno_for(int op_idx, int err);

  std::string& get_error_message_for(int op_idx);

  int incr_response_count() { return ++response_received_count; }

  int get_ops_count() { return ops_count; }

  bool is_at_least_one_op_successful() { return at_least_one_success; }
  // virtual void consume(char* chars, size_t length) = 0;

  void start_timer_for(std::string op_key);
  void stop_timer(bool success = true);  // arg indicates success/failed metric
  // Call the logging always on main thread, so we dont need synchronisation of
  // log file.
  void log_timer();
  std::shared_ptr<MotrAPI> get_motr_api();
  // Google tests
  FRIEND_TEST(S3MotrReadWriteCommonTest, MotrOpDoneOnMainThreadOnSuccess);
  FRIEND_TEST(S3MotrReadWriteCommonTest, S3MotrOpStable);
  FRIEND_TEST(S3MotrReadWriteCommonTest,
              S3MotrOpStableResponseCountSameAsOpCount);
  FRIEND_TEST(S3MotrReadWriteCommonTest,
              S3MotrOpStableResponseCountNotSameAsOpCount);
  FRIEND_TEST(S3MotrReadWriteCommonTest,
              S3MotrOpFailedResponseCountSameAsOpCount);
  FRIEND_TEST(S3MotrReadWriteCommonTest,
              S3MotrOpFailedResponseCountNotSameAsOpCount);
  FRIEND_TEST(S3MotrKVSWritterTest, SyncIndexFailedMissingMetadata);
  FRIEND_TEST(S3MotrKVSWritterTest, SyncIndexFailedFailedMetadata);
  FRIEND_TEST(S3MotrReaderTest, ReadObjectDataFailed);
  FRIEND_TEST(S3MotrReaderTest, ReadObjectDataFailedMissing);
  FRIEND_TEST(S3MotrKVSWritterTest, DelIndexFailed);
  FRIEND_TEST(S3MotrReaderTest, OpenObjectMissingTest);
  FRIEND_TEST(S3MotrReaderTest, OpenObjectErrFailedTest);
};

#endif
