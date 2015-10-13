
#include "s3_delete_object_action.h"
#include "s3_error_codes.h"

S3DeleteObjectAction::S3DeleteObjectAction(std::shared_ptr<S3RequestObject> req) : S3Action(req) {
  setup_steps();
}

void S3DeleteObjectAction::setup_steps(){
  add_task(std::bind( &S3DeleteObjectAction::fetch_bucket_info, this ));
  add_task(std::bind( &S3DeleteObjectAction::fetch_object_info, this ));
  add_task(std::bind( &S3DeleteObjectAction::delete_object, this ));
  add_task(std::bind( &S3DeleteObjectAction::delete_metadata, this ));
  add_task(std::bind( &S3DeleteObjectAction::send_response_to_s3_client, this ));
  // ...
}

void S3DeleteObjectAction::fetch_bucket_info() {
  printf("Called S3DeleteObjectAction::fetch_bucket_info\n");
  bucket_metadata = std::make_shared<S3BucketMetadata>(request);
  bucket_metadata->load(std::bind( &S3DeleteObjectAction::next, this), std::bind( &S3DeleteObjectAction::next, this));
}

void S3DeleteObjectAction::fetch_object_info() {
  printf("Called S3DeleteObjectAction::fetch_bucket_info\n");
  if (bucket_metadata->get_state() == S3BucketMetadataState::present) {
    object_metadata = std::make_shared<S3ObjectMetadata>(request);

    object_metadata->load(std::bind( &S3DeleteObjectAction::next, this), std::bind( &S3DeleteObjectAction::next, this));
  } else {
    send_response_to_s3_client();
  }
}

void S3DeleteObjectAction::delete_object() {
  printf("Called S3DeleteObjectAction::delete_object\n");
  if (object_metadata->get_state() == S3ObjectMetadataState::present) {
    clovis_writer = std::make_shared<S3ClovisWriter>(request);
    clovis_writer->delete_object(std::bind( &S3DeleteObjectAction::next, this), std::bind( &S3DeleteObjectAction::delete_object_failed, this));
  } else {
    send_response_to_s3_client();
  }
}

void S3DeleteObjectAction::delete_object_failed() {
  // TODO - do anything more for failure?
  printf("Called S3DeleteObjectAction::delete_object_failed\n");
  // Any other error report failure.
  send_response_to_s3_client();
}

void S3DeleteObjectAction::delete_metadata() {
  object_metadata = std::make_shared<S3ObjectMetadata>(request);
  object_metadata->remove(std::bind( &S3DeleteObjectAction::next, this), std::bind( &S3DeleteObjectAction::next, this));
}

void S3DeleteObjectAction::send_response_to_s3_client() {
  printf("Called S3DeleteObjectAction::send_response_to_s3_client\n");
  if (bucket_metadata->get_state() == S3BucketMetadataState::missing) {
    // Invalid Bucket Name
    request->send_response(S3HttpFailed400);  // TODO error xml
  } else if (object_metadata->get_state() == S3ObjectMetadataState::missing) {
    request->send_response(S3HttpFailed404);
  }
  else if (object_metadata->get_state() == S3ObjectMetadataState::deleted) {
    request->send_response(S3HttpSuccess200);
  } else {
    // request->set_header_value(...)
    request->send_response(S3HttpFailed500);
  }
  done();
  i_am_done();  // self delete
}
