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

#ifndef __S3_SERVER_S3_ACTION_BASE_H__
#define __S3_SERVER_S3_ACTION_BASE_H__

#include <functional>
#include <memory>
#include <vector>

#include "action_base.h"
#include "s3_bucket_metadata.h"
#include "s3_factory.h"
#include "s3_fi_common.h"
#include "s3_log.h"
#include "s3_object_metadata.h"

#define MAX_OBJECT_KEY_LENGTH 1024
#define MAX_HEADER_SIZE 8192
#define MAX_USER_METADATA_SIZE 2048

// Derived Action Objects will have steps (member functions)
// required to complete the action.
// All member functions should perform an async operation as
// we do not want to block the main thread that manages the
// action. The async callbacks should ensure to call next or
// done/abort etc depending on the result of the operation.
class S3Action : public Action {
 protected:
  std::shared_ptr<S3RequestObject> request;
 private:
  bool skip_authorization;

 public:
  S3Action(std::shared_ptr<S3RequestObject> req, bool check_shutdown = true,
           std::shared_ptr<S3AuthClientFactory> auth_factory = nullptr,
           bool skip_auth = false, bool skip_authorize = false);
  virtual ~S3Action();

  // Register all the member functions required to complete the action.
  // This can register the function as
  // task_list.push_back(std::bind( &S3SomeDerivedAction::step1, this ));
  // Ensure you call this in Derived class constructor.
  virtual void setup_steps();

  // TODO This can be made pure virtual once its implemented for all action
  // class
  virtual void load_metadata();
  virtual void set_authorization_meta();

  // Common steps for all Actions like Authorization.
  void check_authorization();
  void check_authorization_successful();
  void check_authorization_failed();

  void fetch_acl_policies();
  void fetch_acl_bucket_policies_failed();
  void fetch_acl_object_policies_failed();

  FRIEND_TEST(S3ActionTest, Constructor);
  FRIEND_TEST(S3ActionTest, ClientReadTimeoutCallBackRollback);
  FRIEND_TEST(S3ActionTest, ClientReadTimeoutCallBack);
  FRIEND_TEST(S3ActionTest, AddTask);
  FRIEND_TEST(S3ActionTest, AddTaskRollback);
  FRIEND_TEST(S3ActionTest, TasklistRun);
  FRIEND_TEST(S3ActionTest, RollbacklistRun);
  FRIEND_TEST(S3ActionTest, SkipAuthTest);
  FRIEND_TEST(S3ActionTest, EnableAuthTest);
  FRIEND_TEST(S3ActionTest, SetSkipAuthFlagAndSetS3OptionDisableAuthFlag);
  FRIEND_TEST(S3APIHandlerTest, DispatchActionTest);
};

#endif
