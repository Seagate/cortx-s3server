

#include "s3_action_base.h"

S3Action::S3Action(std::shared_ptr<S3RequestObject> req) : request(req) {
  task_iteration_index = 0;
  error_message = "";
}

S3Action::~S3Action() {}

void S3Action::get_error_message(std::string& message) {
    error_message = message;
}

void S3Action::setup_steps() {
  add_task(std::bind( &S3Action::check_authentication, this ));
}

void S3Action::start() {
  task_iteration_index = 0;
  task_list[0]();
}
// Step to next async step.
void S3Action::next() {
  if (task_iteration_index < task_list.size()) {
    task_list[++task_iteration_index]();  // Call the next step
  } else {
    done();
  }
}

void S3Action::done() {
  task_iteration_index = 0;
}

void S3Action::pause() {
  // Set state as Paused.
}

void S3Action::resume() {
  // Resume only if paused.
}

void S3Action::abort() {
  // Mark state as Aborted.
  task_iteration_index = 0;
}

void S3Action::check_authentication() {
  // TODO auth code
  this->next();  // For now assume success.
}
