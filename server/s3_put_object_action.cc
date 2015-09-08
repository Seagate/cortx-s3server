
#include "s3_put_bucket_action.h"

S3PutObjectAction::S3PutObjectAction(S3RequestObject* req) : S3Action(req) {
  setup_steps();
}

void S3PutObjectAction::setup_steps(){
  // add_task(std::bind( &S3PutObjectAction::read_metadata, this ));
  // add_task(std::bind( &S3PutObjectAction::write_metadata, this ));
  add_task(std::bind( &S3PutObjectAction::write_object, this ));
  add_task(std::bind( &S3PutObjectAction::send_response_to_s3_client, this ));
  // ...
}

void S3PutObjectAction::read_metadata() {
  // Trigger metadata read async operation with callback
  metadata = new S3ObjectMetadata(request);
  metadata->async_load(std::bind( &S3PutObjectAction::next, this));
}

void S3PutObjectAction::write_metadata() {
  // Trigger metadata write async operation with callback
  // XXX Check if last step was successful.
  if (metadata->state() == S3ObjectMetadataState::present) {
    // Report 409 bucket exists.
  } else {
    // xxx set attributes & save
    metadata->async_save(std::bind( &S3PutObjectAction::next, this));
  }
}

void S3PutObjectAction::write_object() {
  clovis_writer = new S3ClovisWriter(request);
  clovis_writer->save(std::bind( &S3PutObjectAction::next, this));
}

void S3PutObjectAction::send_response_to_s3_client() {
  // Trigger metadata read async operation with callback
  if (clovis_writer->state() == S3IOOpStatus::saved) {
    // request->set_header_value(...)
    // request->send_response(S3HttpSuccess200);
  } else {
    // request->set_header_value(...)
    request->send_response(S3HttpFailed400);
  }
}
