
#include "s3_get_object_action.h"
#include "s3_error_codes.h"

S3GetObjectAction::S3GetObjectAction(std::shared_ptr<S3RequestObject> req) : S3Action(req) {
  setup_steps();
}

void S3GetObjectAction::setup_steps(){
  add_task(std::bind( &S3GetObjectAction::fetch_bucket_info, this ));
  add_task(std::bind( &S3GetObjectAction::fetch_object_info, this ));
  add_task(std::bind( &S3GetObjectAction::read_object, this ));
  add_task(std::bind( &S3GetObjectAction::send_response_to_s3_client, this ));
  // ...
}

void S3GetObjectAction::fetch_bucket_info() {
  printf("Called S3GetObjectAction::fetch_bucket_info\n");
  bucket_metadata = std::make_shared<S3BucketMetadata>(request);
  bucket_metadata->load(std::bind( &S3GetObjectAction::next, this), std::bind( &S3GetObjectAction::next, this));
}

void S3GetObjectAction::fetch_object_info() {
  printf("Called S3GetObjectAction::fetch_bucket_info\n");
  if (bucket_metadata->get_state() == S3BucketMetadataState::present) {
    object_metadata = std::make_shared<S3ObjectMetadata>(request);

    object_metadata->load(std::bind( &S3GetObjectAction::next, this), std::bind( &S3GetObjectAction::next, this));
  } else {
    send_response_to_s3_client();
  }
}

void S3GetObjectAction::read_object() {
  printf("Called S3GetObjectAction::read_object\n");
  if (object_metadata->get_state() == S3ObjectMetadataState::present) {
    size_t object_size = object_metadata->get_content_length();
    printf("Reading object of size %zu\n", object_size);

    clovis_reader = std::make_shared<S3ClovisReader>(request);
    clovis_reader->read_object(object_size, std::bind( &S3GetObjectAction::next, this), std::bind( &S3GetObjectAction::read_object_failed, this));
  } else {
    send_response_to_s3_client();
  }
}

void S3GetObjectAction::read_object_failed() {
  // TODO - do anything more for failure?
  printf("Called S3GetObjectAction::read_object_failed\n");
  send_response_to_s3_client();
}


void S3GetObjectAction::send_response_to_s3_client() {
  printf("Called S3GetObjectAction::send_response_to_s3_client\n");
  // Trigger metadata read async operation with callback
  if (bucket_metadata->get_state() == S3BucketMetadataState::missing) {
    // Invalid Bucket Name
    request->send_response(S3HttpFailed400);
  } else if (object_metadata->get_state() == S3ObjectMetadataState::missing) {
    request->send_response(S3HttpFailed404);
  } else if (clovis_reader->get_state() == S3ClovisReaderOpState::complete) {
    request->set_out_header_value("Last-Modified", object_metadata->get_last_modified());
    request->set_out_header_value("ETag", object_metadata->get_md5());
    request->set_out_header_value("Accept-Ranges", "bytes");
    request->set_out_header_value("Content-Length", object_metadata->get_content_length_str());

    char *data = NULL;
    int length = 0;
    request->send_reply_start(S3HttpSuccess200);

    length = clovis_reader->get_first_block(&data);
    while(length > 0)
    {
      request->send_reply_body(data, length);
      length = clovis_reader->get_next_block(&data);
    }
    request->send_reply_end();
  } else {
    // request->set_header_value(...)
    request->send_response(S3HttpFailed400);
  }
  done();
  i_am_done();  // self delete
}
