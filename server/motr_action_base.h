/*
 * COPYRIGHT 2019 SEAGATE LLC
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
 * Original author:  Prashanth Vanaparthy   <prashanth.vanaparthy@seagate.com>
 * Original creation date: 1-JUNE-2019
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
