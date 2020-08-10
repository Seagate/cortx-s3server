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

#ifndef __S3_SERVER_MERO_ACTION_BASE_H__
#define __S3_SERVER_MERO_ACTION_BASE_H__

#include <functional>
#include <memory>
#include <vector>

#include "action_base.h"
#include "s3_factory.h"
#include "s3_fi_common.h"
#include "s3_log.h"
#include "mero_request_object.h"

#define BACKGROUND_STALE_OBJECT_DELETE_ACCOUNT "s3-background-delete-svc"
#define S3RECOVERY_ACCOUNT "s3-recovery-svc"

// Derived Action Objects will have steps (member functions)
// required to complete the action.
// All member functions should perform an async operation as
// we do not want to block the main thread that manages the
// action. The async callbacks should ensure to call next or
// done/abort etc depending on the result of the operation.
class MeroAction : public Action {
 protected:
  std::shared_ptr<MeroRequestObject> request;

 public:
  MeroAction(std::shared_ptr<MeroRequestObject> req, bool check_shutdown = true,
             std::shared_ptr<S3AuthClientFactory> auth_factory = nullptr,
             bool skip_auth = false);
  virtual ~MeroAction();

  // Register all the member functions required to complete the action.
  // This can register the function as
  // task_list.push_back(std::bind( &S3SomeDerivedAction::step1, this ));
  // Ensure you call this in Derived class constructor.
  virtual void setup_steps();

  // Common steps for all Actions like Authorization.
  void check_authorization();

  FRIEND_TEST(MeroActionTest, Constructor);
  FRIEND_TEST(MeroActionTest, ClientReadTimeoutCallBackRollback);
  FRIEND_TEST(MeroActionTest, ClientReadTimeoutCallBack);
  FRIEND_TEST(MeroActionTest, AddTask);
  FRIEND_TEST(MeroActionTest, AddTaskRollback);
  FRIEND_TEST(MeroActionTest, TasklistRun);
  FRIEND_TEST(MeroActionTest, RollbacklistRun);
  FRIEND_TEST(MeroActionTest, SkipAuthTest);
  FRIEND_TEST(MeroActionTest, EnableAuthTest);
  FRIEND_TEST(MeroActionTest, SetSkipAuthFlagAndSetS3OptionDisableAuthFlag);
  FRIEND_TEST(S3APIHandlerTest, DispatchActionTest);
};

#endif
