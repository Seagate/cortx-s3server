
#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_ACTION_BASE_H__
#define __MERO_FE_S3_SERVER_S3_ACTION_BASE_H__

#include <memory>
#include <functional>
#include <vector>

#include "s3_request_object.h"

// Derived Action Objects will have steps (member functions)
// required to complete the action.
// All member functions should perform an async operation as
// we do not want to block the main thread that manages the
// action. The async callbacks should ensure to call next or
// done/abort etc depending on the result of the operation.
class S3Action {
protected:
  std::shared_ptr<S3RequestObject> request;
  // Any action specific state should be managed by derived classes.
private:
  // Holds the member functions that will process the request.
  // member function signature should be void fn();
  std::vector<std::function<void()>> task_list;
  size_t task_iteration_index;

  std::shared_ptr<S3Action> self_ref;

  std::string error_message;

public:
  S3Action(std::shared_ptr<S3RequestObject> req);
  virtual ~S3Action();

  void get_error_message(std::string& message);

protected:
  void add_task(std::function<void()> task) {
    task_list.push_back(task);
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
  virtual void setup_steps() = 0;

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
  void send_response_to_s3_client();
};

#endif
