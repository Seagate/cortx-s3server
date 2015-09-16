
#include "s3_put_object_action.h"
#include "s3_error_codes.h"

S3PutObjectAction::S3PutObjectAction(std::shared_ptr<S3RequestObject> req) : S3Action(req) {
  setup_steps();
}

void S3PutObjectAction::setup_steps(){
  // add_task(std::bind( &S3PutObjectAction::check_metadata, this ));
  // add_task(std::bind( &S3PutObjectAction::write_metadata, this ));
  add_task(std::bind( &S3PutObjectAction::create_object, this ));
  add_task(std::bind( &S3PutObjectAction::write_object, this ));
  add_task(std::bind( &S3PutObjectAction::send_response_to_s3_client, this ));
  // ...
}
/*
void S3PutObjectAction::check_metadata() {
  // Trigger metadata read async operation with callback
  metadata = std::make_shared<S3ObjectMetadata> (request);

  metadata->async_load(std::bind( &S3PutObjectAction::next, this));
}

void S3PutObjectAction::write_metadata() {
  // Trigger metadata write async operation with callback
  // XXX Check if last step was successful.
  if (metadata->get_state() == S3ObjectMetadataState::present) {
    // Report 409 bucket exists.
  } else {
    // xxx set attributes & save
    metadata->async_save(std::bind( &S3PutObjectAction::next, this));
  }
}
*/
void S3PutObjectAction::create_object() {
  printf("Called S3PutObjectAction::create_object\n");
  clovis_writer = std::make_shared<S3ClovisWriter>(request);
  clovis_writer->create_object(std::bind( &S3PutObjectAction::next, this), std::bind( &S3PutObjectAction::create_object_failed, this));
}

void S3PutObjectAction::create_object_failed() {
  // TODO - do anything more for failure?
  printf("Called S3PutObjectAction::create_object_failed\n");
  send_response_to_s3_client();
}

void S3PutObjectAction::write_object() {
  printf("Called S3PutObjectAction::write_object\n");
  clovis_writer->write_content(std::bind( &S3PutObjectAction::next, this), std::bind( &S3PutObjectAction::write_object_failed, this));
}

void S3PutObjectAction::write_object_failed() {
  // TODO - do anything more for failure?
  printf("Called S3PutObjectAction::write_object_failed\n");
  send_response_to_s3_client();
}

void S3PutObjectAction::send_response_to_s3_client() {
  printf("Called S3PutObjectAction::send_response_to_s3_client\n");
  // Trigger metadata read async operation with callback
  if (clovis_writer->get_state() == S3ClovisWriterOpState::saved) {
    // request->set_header_value(...)
    request->send_response(S3HttpSuccess200);
  } else {
    // request->set_header_value(...)
    request->send_response(S3HttpFailed400);
  }
  done();
  i_am_done();  // self delete
}
