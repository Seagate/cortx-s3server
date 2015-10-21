

#include "s3_action_base.h"
#include "s3_error_codes.h"

S3Action::S3Action(std::shared_ptr<S3RequestObject> req) : request(req), invalid_request(false) {
  task_iteration_index = 0;
  error_message = "";
  setup_steps();
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
  check_auth = std::make_shared<S3AuthClient>(request);
  check_auth->check_authentication(std::bind( &S3Action::next, this), std::bind( &S3Action::check_authentication_failed, this));
}

void S3Action::check_authentication_failed() {
  printf("Called S3Action::check_authentication_failed\n");
  S3Error error("AccessDenied", request->get_request_id(), request->get_object_uri());
  std::string& response_xml = error.to_xml();
  request->set_out_header_value("Content-Type", "application/xml");
  request->set_out_header_value("Content-Length", std::to_string(response_xml.length()));

  request->send_response(error.get_http_status_code(), response_xml);

  done();
  i_am_done();
}
