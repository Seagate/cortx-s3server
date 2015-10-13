
#include "s3_head_object_action.h"
#include "s3_error_codes.h"

S3HeadObjectAction::S3HeadObjectAction(std::shared_ptr<S3RequestObject> req) : S3Action(req) {
  setup_steps();
}

void S3HeadObjectAction::setup_steps(){
  add_task(std::bind( &S3HeadObjectAction::fetch_bucket_info, this ));
  add_task(std::bind( &S3HeadObjectAction::fetch_object_info, this ));
  add_task(std::bind( &S3HeadObjectAction::send_response_to_s3_client, this ));
  // ...
}

void S3HeadObjectAction::fetch_bucket_info() {
  printf("Called S3HeadObjectAction::fetch_bucket_info\n");
  bucket_metadata = std::make_shared<S3BucketMetadata>(request);
  bucket_metadata->load(std::bind( &S3HeadObjectAction::next, this), std::bind( &S3HeadObjectAction::next, this));
}

void S3HeadObjectAction::fetch_object_info() {
  printf("Called S3HeadObjectAction::fetch_bucket_info\n");
  if (bucket_metadata->get_state() == S3BucketMetadataState::present) {
    object_metadata = std::make_shared<S3ObjectMetadata>(request);

    object_metadata->load(std::bind( &S3HeadObjectAction::next, this), std::bind( &S3HeadObjectAction::next, this));
  } else {
    send_response_to_s3_client();
  }
}

void S3HeadObjectAction::send_response_to_s3_client() {
  printf("Called S3HeadObjectAction::send_response_to_s3_client\n");
  // Trigger metadata read async operation with callback
  if (bucket_metadata->get_state() == S3BucketMetadataState::missing) {
    // Invalid Bucket Name
    request->send_response(S3HttpFailed400);
  } else if (object_metadata->get_state() == S3ObjectMetadataState::missing) {
    request->send_response(S3HttpFailed404);
  } else if (object_metadata->get_state() == S3ObjectMetadataState::present) {
    request->set_out_header_value("Last-Modified", object_metadata->get_last_modified());
    request->set_out_header_value("ETag", object_metadata->get_md5());
    request->set_out_header_value("Accept-Ranges", "bytes");
    request->set_out_header_value("Content-Length", object_metadata->get_content_length_str());

    request->send_response(S3HttpSuccess200);
  } else {
    // request->set_header_value(...)
    request->send_response(S3HttpFailed400);
  }
  done();
  i_am_done();  // self delete
}
