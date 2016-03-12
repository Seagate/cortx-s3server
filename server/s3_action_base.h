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

#ifndef __MERO_FE_S3_SERVER_S3_ACTION_BASE_H__
#define __MERO_FE_S3_SERVER_S3_ACTION_BASE_H__

#include <memory>
#include <functional>
#include <vector>

#include "s3_auth_client.h"
#include "s3_request_object.h"
#include "s3_log.h"

enum class S3ActionState {
  start,
  running,
  complete,
  paused,
  stopped,  // Aborted
};

// Derived Action Objects will have steps (member functions)
// required to complete the action.
// All member functions should perform an async operation as
// we do not want to block the main thread that manages the
// action. The async callbacks should ensure to call next or
// done/abort etc depending on the result of the operation.
class S3Action {
protected:
  std::shared_ptr<S3RequestObject> request;
  bool invalid_request;
  // Any action specific state should be managed by derived classes.
private:
  // Holds the member functions that will process the request.
  // member function signature should be void fn();
  std::vector<std::function<void()>> task_list;
  size_t task_iteration_index;

  std::shared_ptr<S3Action> self_ref;
  std::shared_ptr<S3AuthClient> auth_client;

  std::string error_message;
  S3ActionState state;

public:
  S3Action(std::shared_ptr<S3RequestObject> req);
  virtual ~S3Action();

  void get_error_message(std::string& message);

protected:
  void add_task(std::function<void()> task) {
    task_list.push_back(task);
  }

  void clear_tasks() {
    task_list.clear();
  }

  std::shared_ptr<S3AuthClient>& get_auth_client() {
    return auth_client;
  }

public:
  // Self destructing object.
  void manage_self(std::shared_ptr<S3Action> ref) {
      self_ref = ref;
  }
  // This *MUST* be the last thing on object. Called @ end of dispatch.
  void i_am_done() {
    self_ref.reset();
  }

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

  // Common steps for all Actions like Authenticate.
  void check_authentication();
  void check_authentication_successful();
  void check_authentication_failed();
  void start_chunk_authentication();

  void send_response_to_s3_client();
};

#endif
