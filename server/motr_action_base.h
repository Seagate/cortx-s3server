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

#ifndef __S3_SERVER_MOTR_ACTION_BASE_H__
#define __S3_SERVER_MOTR_ACTION_BASE_H__

#include <functional>
#include <memory>
#include <vector>

#include "action_base.h"
#include "s3_factory.h"
#include "s3_fi_common.h"
#include "s3_log.h"
#include "motr_request_object.h"

#define BACKGROUND_STALE_OBJECT_DELETE_ACCOUNT "s3-background-delete-svc"
#define S3RECOVERY_ACCOUNT "s3-recovery-svc"

// Derived Action Objects will have steps (member functions)
// required to complete the action.
// All member functions should perform an async operation as
// we do not want to block the main thread that manages the
// action. The async callbacks should ensure to call next or
// done/abort etc depending on the result of the operation.
class MotrAction : public Action {
 protected:
  std::shared_ptr<MotrRequestObject> request;

 public:
  MotrAction(std::shared_ptr<MotrRequestObject> req, bool check_shutdown = true,
             std::shared_ptr<S3AuthClientFactory> auth_factory = nullptr,
             bool skip_auth = false);
  virtual ~MotrAction();

  // Register all the member functions required to complete the action.
  // This can register the function as
  // task_list.push_back(std::bind( &S3SomeDerivedAction::step1, this ));
  // Ensure you call this in Derived class constructor.
  virtual void setup_steps();

  // Common steps for all Actions like Authorization.
  void check_authorization();

  FRIEND_TEST(MotrActionTest, Constructor);
  FRIEND_TEST(MotrActionTest, ClientReadTimeoutCallBackRollback);
  FRIEND_TEST(MotrActionTest, ClientReadTimeoutCallBack);
  FRIEND_TEST(MotrActionTest, AddTask);
  FRIEND_TEST(MotrActionTest, AddTaskRollback);
  FRIEND_TEST(MotrActionTest, TasklistRun);
  FRIEND_TEST(MotrActionTest, RollbacklistRun);
  FRIEND_TEST(MotrActionTest, SkipAuthTest);
  FRIEND_TEST(MotrActionTest, EnableAuthTest);
  FRIEND_TEST(MotrActionTest, SetSkipAuthFlagAndSetS3OptionDisableAuthFlag);
  FRIEND_TEST(S3APIHandlerTest, DispatchActionTest);
};

#endif
