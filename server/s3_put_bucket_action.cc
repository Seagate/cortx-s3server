
#include "s3_put_bucket_action.h"
#include "s3_put_bucket_body.h"
#include "s3_error_codes.h"

S3PutBucketAction::S3PutBucketAction(std::shared_ptr<S3RequestObject> req) : S3Action(req) {
  setup_steps();
}

void S3PutBucketAction::setup_steps(){
  add_task(std::bind( &S3PutBucketAction::validate_request, this ));
  add_task(std::bind( &S3PutBucketAction::read_metadata, this ));
  add_task(std::bind( &S3PutBucketAction::create_bucket, this ));
  add_task(std::bind( &S3PutBucketAction::send_response_to_s3_client, this ));
  // ...
}

void S3PutBucketAction::validate_request() {
  printf("S3PutBucketAction::validate_request\n");
  S3PutBucketBody bucket(request->get_full_body_content_as_string());
  if (bucket.isOK()) {
    location_constraint = bucket.get_location_constraint();
    next();
  } else {
    invalid_request = true;
    send_response_to_s3_client();
  }
}

void S3PutBucketAction::read_metadata() {
  printf("S3PutBucketAction::read_metadata\n");

  // Trigger metadata read async operation with callback
  bucket_metadata = std::make_shared<S3BucketMetadata>(request);
  bucket_metadata->load(std::bind( &S3PutBucketAction::next, this), std::bind( &S3PutBucketAction::next, this));
}

void S3PutBucketAction::create_bucket() {
  printf("S3PutBucketAction::create_bucket\n");

  // Trigger metadata write async operation with callback
  // XXX Check if last step was successful.
  if (bucket_metadata->get_state() == S3BucketMetadataState::present) {
    // Report 409 bucket exists.
    send_response_to_s3_client();
  } else {
    // xxx set attributes & save
    if (!location_constraint.empty()) {
      bucket_metadata->add_system_attribute("LocationConstraint", location_constraint);
    }
    bucket_metadata->save(std::bind( &S3PutBucketAction::next, this), std::bind( &S3PutBucketAction::next, this));
  }
}

void S3PutBucketAction::send_response_to_s3_client() {
  printf("S3PutBucketAction::send_response_to_s3_client\n");

  if (invalid_request) {
    request->send_response(S3HttpFailed400);
  } else if (bucket_metadata->get_state() == S3BucketMetadataState::present) {
    request->send_response(S3HttpFailed409);
  } else if (bucket_metadata->get_state() == S3BucketMetadataState::saved) {
    // request->set_header_value(...)
    request->send_response(S3HttpSuccess200);
  } else {
    // request->set_header_value(...)
    request->send_response(S3HttpFailed500);
  }
  done();
  i_am_done();  // self delete
}
