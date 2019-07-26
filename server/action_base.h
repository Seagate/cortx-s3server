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
 * Original author:  Kaustubh Deorukhkar   <kaustubh.deorukhkar@seagate.com>
 * Original author:  Prashanth Vanaparthy   <prashanth.vanaparthy@seagate.com>
 * Original creation date: 1-June-2019
 */

#pragma once

#ifndef __S3_SERVER_ACTION_BASE_H__
#define __S3_SERVER_ACTION_BASE_H__

#include <functional>
#include <memory>
#include <vector>

#include "s3_auth_client.h"
#include "s3_factory.h"
#include "s3_fi_common.h"
#include "s3_log.h"
#include "s3_memory_profile.h"
#include "request_object.h"

#ifdef ENABLE_FAULT_INJECTION

#define S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL(tag)                        \
  do {                                                                  \
    if (s3_fi_is_enabled(tag)) {                                        \
      s3_log(S3_LOG_DEBUG, "", "set shutdown signal for testing...\n"); \
      set_is_fi_hit(true);                                              \
      S3Option::get_instance()->set_is_s3_shutting_down(true);          \
    }                                                                   \
  } while (0)

#define S3_RESET_SHUTDOWN_SIGNAL                                \
  do {                                                          \
    if (get_is_fi_hit()) {                                      \
      s3_log(S3_LOG_DEBUG, "", "reset shutdown signal ...\n");  \
      S3Option::get_instance()->set_is_s3_shutting_down(false); \
      set_is_fi_hit(false);                                     \
    }                                                           \
  } while (0)

#else  // ENABLE_FAULT_INJECTION

#define S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL(tag)
#define S3_RESET_SHUTDOWN_SIGNAL

#endif  // ENABLE_FAULT_INJECTION

enum class ActionState {
  start,
  running,
  complete,
  paused,
  stopped,  // Aborted
  error,
};

// Derived Action Objects will have steps (member functions)
// required to complete the action.
// All member functions should perform an async operation as
// we do not want to block the main thread that manages the
// action. The async callbacks should ensure to call next or
// done/abort etc depending on the result of the operation.
class Action {
 private:
  std::shared_ptr<RequestObject> base_request;

  // Holds the member functions that will process the request.
  // member function signature should be void fn();
  std::vector<std::function<void()>> task_list;
  size_t task_iteration_index;

  // Hold member functions that will rollback
  // changes in event of error
  std::deque<std::function<void()>> rollback_list;
  size_t rollback_index;

  std::shared_ptr<Action> self_ref;

  std::string s3_error_code;
  ActionState state;
  ActionState rollback_state;

  // If this flag is set then check_shutdown_and_rollback() is called
  // from start() & next() methods.
  bool check_shutdown_signal;
  // In case of shutdown, this flag indicates that a error response
  // is already scheduled.
  bool is_response_scheduled;

  // For shutdown related system tests, if FI gets hit then set this flag
  bool is_fi_hit;

 protected:
  std::string request_id;
  bool invalid_request;
  // Allow class object instiantiation without support for authentication
  bool skip_auth;

  std::shared_ptr<S3AuthClient> auth_client;

  std::shared_ptr<S3MemoryProfile> mem_profile;
  std::shared_ptr<S3AuthClientFactory> auth_client_factory;

 public:
  Action(std::shared_ptr<RequestObject> req, bool check_shutdown = true,
         std::shared_ptr<S3AuthClientFactory> auth_factory = nullptr,
         bool skip_auth = false);
  virtual ~Action();

  void set_s3_error(std::string code);
  std::string& get_s3_error_code();
  bool is_error_state();
  void client_read_timeout_callback();

 protected:
  void add_task(std::function<void()> task) { task_list.push_back(task); }

  void clear_tasks() {
    task_list.clear();
    task_iteration_index = 0;
  }

  std::shared_ptr<S3AuthClient>& get_auth_client() { return auth_client; }

  // Add tasks to list after each successful operation that needs rollback.
  // This list can be used to rollback changes in the event of error
  // during any stage of operation.
  void add_task_rollback(std::function<void()> task) {
    // Insert task at start of list for rollback
    rollback_list.push_front(task);
  }

  void clear_tasks_rollback() { rollback_list.clear(); }

  int number_of_rollback_tasks() { return rollback_list.size(); }

  ActionState get_rollback_state() { return rollback_state; }

  // Checks whether S3 is shutting down. If yes then triggers rollback and
  // schedules a response.
  // Return value: true, in case of shutdown.
  virtual bool check_shutdown_and_rollback(bool check_auth_op_aborted = false);

  bool reject_if_shutting_down() { return is_response_scheduled; }

  // If param 'check_signal' is true then check_shutdown_and_rollback() method
  // will be invoked for next tasks in the queue.
  void check_shutdown_signal_for_next_task(bool check_signal) {
    check_shutdown_signal = check_signal;
  }

  void set_is_fi_hit(bool hit) { is_fi_hit = hit; }

  bool get_is_fi_hit() { return is_fi_hit; }

 public:
  // Self destructing object.
  void manage_self(std::shared_ptr<Action> ref) { self_ref = ref; }

  int number_of_tasks() { return task_list.size(); }

  // This *MUST* be the last thing on object. Called @ end of dispatch.
  void i_am_done() { self_ref.reset(); }

  // Register all the member functions required to complete the action.
  // This can register the function as
  // task_list.push_back(std::bind( &S3SomeDerivedAction::step1, this ));
  // Ensure you call this in Derived class constructor.
  virtual void setup_steps();

  // Common Actions.
  void start();
  // Step to next async step.
  void next();
  void done();
  void pause();
  void resume();
  void abort();

  // rollback async steps.
  void rollback_start();
  void rollback_next();
  void rollback_done();
  // Default will call the last task in task_list after exhausting all tasks in
  // rollback_list. It expects last task in task_list to be
  // send_response_to_s3_client;
  virtual void rollback_exit();

  // Common steps for all Actions like Authenticate.
  void check_authentication();
  void check_authentication_successful();
  void check_authentication_failed();

  virtual void send_response_to_s3_client() = 0;
  virtual void send_retry_error_to_s3_client(int retry_after_in_secs = 1);

  FRIEND_TEST(ActionTest, Constructor);
  FRIEND_TEST(ActionTest, ClientReadTimeoutCallBackRollback);
  FRIEND_TEST(ActionTest, ClientReadTimeoutCallBack);
  FRIEND_TEST(ActionTest, AddTask);
  FRIEND_TEST(ActionTest, AddTaskRollback);
  FRIEND_TEST(ActionTest, TasklistRun);
  FRIEND_TEST(ActionTest, RollbacklistRun);
  FRIEND_TEST(ActionTest, SkipAuthTest);
  FRIEND_TEST(ActionTest, EnableAuthTest);
  FRIEND_TEST(ActionTest, SetSkipAuthFlagAndSetS3OptionDisableAuthFlag);
  FRIEND_TEST(S3APIHandlerTest, DispatchActionTest);
  FRIEND_TEST(MeroAPIHandlerTest, DispatchActionTest);
};

#endif
