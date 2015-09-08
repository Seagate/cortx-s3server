
#include "s3_put_bucket_action.h"

S3PutBucketAction::S3PutBucketAction(S3RequestObject* req) : S3Action(req) {
  setup_steps();
}

void S3PutBucketAction::setup_steps(){
  add_task(std::bind( &S3PutBucketAction::read_metadata, this ));
  add_task(std::bind( &S3PutBucketAction::write_metadata, this ));
  add_task(std::bind( &S3PutBucketAction::send_response_to_s3_client, this ));
  // ...
}

void S3PutBucketAction::read_metadata() {
  // Trigger metadata read async operation with callback
  metadata = new S3BucketMetadata(request);
  metadata->async_load(std::bind( &S3PutBucketAction::next, this));
}

void S3PutBucketAction::write_metadata() {
  // Trigger metadata write async operation with callback
  // XXX Check if last step was successful.
  if (metadata->state() == S3BucketMetadataState::present) {
    // Report 409 bucket exists.
  } else {
    // xxx set attributes & save
    metadata->async_save(std::bind( &S3PutBucketAction::next, this));
  }
}

void S3PutBucketAction::send_response_to_s3_client() {
  // Trigger metadata read async operation with callback
  if (metadata->state() == S3BucketMetadataState::saved) {
    // request->set_header_value(...)
    // request->send_response(S3HttpSuccess200);
  } else {
    // request->set_header_value(...)
    request->send_response(S3HttpFailed400);
  }
}
