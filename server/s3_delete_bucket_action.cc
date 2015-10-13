
#include "s3_delete_bucket_action.h"
#include "s3_error_codes.h"

S3DeleteBucketAction::S3DeleteBucketAction(std::shared_ptr<S3RequestObject> req) : S3Action(req), last_key(""), is_bucket_empty(false), delete_successful(false) {
  setup_steps();
}

void S3DeleteBucketAction::setup_steps(){
  add_task(std::bind( &S3DeleteBucketAction::fetch_bucket_metadata, this ));
  add_task(std::bind( &S3DeleteBucketAction::fetch_first_object_metadata, this ));
  add_task(std::bind( &S3DeleteBucketAction::delete_bucket, this ));
  add_task(std::bind( &S3DeleteBucketAction::send_response_to_s3_client, this ));
  // ...
}

void S3DeleteBucketAction::fetch_bucket_metadata() {
  printf("S3DeleteBucketAction::fetch_bucket_metadata\n");

  // Trigger metadata read async operation with callback
  bucket_metadata = std::make_shared<S3BucketMetadata>(request);
  bucket_metadata->load(std::bind( &S3DeleteBucketAction::next, this), std::bind( &S3DeleteBucketAction::next, this));
}

void S3DeleteBucketAction::fetch_first_object_metadata() {
  printf("Called S3DeleteBucketAction::fetch_first_object_metadata\n");

  if (bucket_metadata->get_state() == S3BucketMetadataState::present) {
    clovis_kv_reader = std::make_shared<S3ClovisKVSReader>(request);
    // Try to fetch one object at least
    clovis_kv_reader->next_keyval(get_bucket_index_name(), last_key, 1, std::bind( &S3DeleteBucketAction::fetch_first_object_metadata_successful, this), std::bind( &S3DeleteBucketAction::fetch_first_object_metadata_failed, this));
  } else {
    send_response_to_s3_client();
  }
}

void S3DeleteBucketAction::fetch_first_object_metadata_successful() {
  printf("Called S3DeleteBucketAction::fetch_first_object_metadata_successful\n");
  is_bucket_empty = false;
  send_response_to_s3_client();
}

void S3DeleteBucketAction::fetch_first_object_metadata_failed() {
  printf("Called S3DeleteBucketAction::fetch_first_object_metadata_failed\n");
  if (clovis_kv_reader->get_state() == S3ClovisKVSReaderOpState::missing) {
    is_bucket_empty = true;
    next();
  } else {
    is_bucket_empty = false;
    send_response_to_s3_client();
  }
}

void S3DeleteBucketAction::delete_bucket() {
  printf("Called S3DeleteBucketAction::delete_bucket\n");
  bucket_metadata->remove(std::bind( &S3DeleteBucketAction::delete_bucket_successful, this), std::bind( &S3DeleteBucketAction::delete_bucket_failed, this));
}

void S3DeleteBucketAction::delete_bucket_successful() {
  printf("Called S3DeleteBucketAction::delete_bucket_successful\n");
  delete_successful = true;
  send_response_to_s3_client();
}

void S3DeleteBucketAction::delete_bucket_failed() {
  printf("Called S3DeleteBucketAction::delete_bucket_failed\n");
  delete_successful = false;
  send_response_to_s3_client();
}

void S3DeleteBucketAction::send_response_to_s3_client() {
  printf("Called S3DeleteBucketAction::send_response_to_s3_client\n");
  // Trigger metadata read async operation with callback
  if (bucket_metadata->get_state() == S3BucketMetadataState::missing) {
    request->send_response(S3HttpFailed404);  // TODO add error xml
  } else if (is_bucket_empty == false) {
    // Bucket not empty, cannot delete
    request->send_response(S3HttpFailed409);  // TODO add error xml
  } else if (delete_successful) {
    request->send_response(S3HttpSuccess204);
  } else {
    // request->set_header_value(...)
    request->send_response(S3HttpFailed400);
  }
  done();
  i_am_done();  // self delete
}
