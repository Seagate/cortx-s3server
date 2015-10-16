
#include "s3_head_bucket_action.h"
#include "s3_error_codes.h"

S3HeadBucketAction::S3HeadBucketAction(std::shared_ptr<S3RequestObject> req) : S3Action(req) {
  setup_steps();
}

void S3HeadBucketAction::setup_steps(){
  add_task(std::bind( &S3HeadBucketAction::read_metadata, this ));
  add_task(std::bind( &S3HeadBucketAction::send_response_to_s3_client, this ));
  // ...
}

void S3HeadBucketAction::read_metadata() {
  printf("S3HeadBucketAction::read_metadata\n");

  // Trigger metadata read async operation with callback
  bucket_metadata = std::make_shared<S3BucketMetadata>(request);
  bucket_metadata->load(std::bind( &S3HeadBucketAction::next, this), std::bind( &S3HeadBucketAction::next, this));
}

void S3HeadBucketAction::send_response_to_s3_client() {
  printf("S3HeadBucketAction::send_response_to_s3_client\n");

  if (bucket_metadata->get_state() == S3BucketMetadataState::present) {
    request->send_response(S3HttpSuccess200);
  } else {
    S3Error error("NoSuchBucket", request->get_request_id(), request->get_bucket_name());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length", std::to_string(response_xml.length()));

    request->send_response(error.get_http_status_code(), response_xml);
  }
  done();
  i_am_done();  // self delete
}
