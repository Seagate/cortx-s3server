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

#ifndef __S3_SERVER_MOTR_API_HANDLER_H__
#define __S3_SERVER_MOTR_API_HANDLER_H__

#include <memory>

#include "gtest/gtest_prod.h"

#include "action_base.h"
#include "s3_common.h"
#include "motr_request_object.h"

class MotrAPIHandler {
 protected:
  std::shared_ptr<MotrRequestObject> request;
  std::shared_ptr<Action> action;

  MotrOperationCode operation_code;

  // Only for Unit testing
  std::shared_ptr<MotrAPIHandler> _get_self_ref() { return self_ref; }
  std::shared_ptr<Action> _get_action() { return action; }
  std::string request_id;

 private:
  std::shared_ptr<MotrAPIHandler> self_ref;

 public:
  MotrAPIHandler(std::shared_ptr<MotrRequestObject> req,
                 MotrOperationCode op_code)
      : request(req), operation_code(op_code) {
    request_id = request->get_request_id();
  }
  virtual ~MotrAPIHandler() {}

  virtual void create_action() = 0;

  virtual void dispatch() {
    s3_log(S3_LOG_DEBUG, request_id, "Entering");

    if (action) {
      action->manage_self(action);
      action->start();
    } else {
      request->respond_unsupported_api();
    }
    i_am_done();

    s3_log(S3_LOG_DEBUG, "", "Exiting");
  }

  // Self destructing object.
  void manage_self(std::shared_ptr<MotrAPIHandler> ref) { self_ref = ref; }
  // This *MUST* be the last thing on object. Called @ end of dispatch.
  void i_am_done() { self_ref.reset(); }

  FRIEND_TEST(MotrAPIHandlerTest, ConstructorTest);
  FRIEND_TEST(MotrAPIHandlerTest, ManageSelfAndReset);
  FRIEND_TEST(MotrAPIHandlerTest, DispatchActionTest);
  FRIEND_TEST(MotrAPIHandlerTest, DispatchUnSupportedAction);
};

class MotrIndexAPIHandler : public MotrAPIHandler {
 public:
  MotrIndexAPIHandler(std::shared_ptr<MotrRequestObject> req,
                      MotrOperationCode op_code)
      : MotrAPIHandler(req, op_code) {}

  virtual void create_action();

  FRIEND_TEST(MotrIndexAPIHandlerTest, ConstructorTest);
  FRIEND_TEST(MotrIndexAPIHandlerTest, ManageSelfAndReset);
  FRIEND_TEST(MotrIndexAPIHandlerTest, ShouldCreateS3GetServiceAction);
  FRIEND_TEST(MotrIndexAPIHandlerTest, OperationNoneDefaultNoAction);
};

class MotrKeyValueAPIHandler : public MotrAPIHandler {
 public:
  MotrKeyValueAPIHandler(std::shared_ptr<MotrRequestObject> req,
                         MotrOperationCode op_code)
      : MotrAPIHandler(req, op_code) {}

  virtual void create_action();
};

class MotrObjectAPIHandler : public MotrAPIHandler {
 public:
  MotrObjectAPIHandler(std::shared_ptr<MotrRequestObject> req,
                       MotrOperationCode op_code)
      : MotrAPIHandler(req, op_code) {}

  virtual void create_action();
};

class MotrFaultinjectionAPIHandler : public MotrAPIHandler {
 public:
  MotrFaultinjectionAPIHandler(std::shared_ptr<MotrRequestObject> req,
                               MotrOperationCode op_code)
      : MotrAPIHandler(req, op_code) {}

  virtual void create_action();
};

class MotrAPIHandlerFactory {
 public:
  virtual ~MotrAPIHandlerFactory() {}

  virtual std::shared_ptr<MotrAPIHandler> create_api_handler(
      MotrApiType api_type, std::shared_ptr<MotrRequestObject> request,
      MotrOperationCode op_code);
};

#endif
