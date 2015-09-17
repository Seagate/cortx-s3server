
#include "s3_get_object_action.h"
#include "s3_error_codes.h"

S3GetObjectAction::S3GetObjectAction(std::shared_ptr<S3RequestObject> req) : S3Action(req) {
  setup_steps();
}

void S3GetObjectAction::setup_steps(){
  // add_task(std::bind( &S3GetObjectAction::fetch_metadata, this ));
  add_task(std::bind( &S3GetObjectAction::read_object, this ));
  add_task(std::bind( &S3GetObjectAction::send_response_to_s3_client, this ));
  // ...
}
/*
void S3GetObjectAction::fetch_metadata() {
  // Trigger metadata read async operation with callback
  metadata = std::make_shared<S3ObjectMetadata> (request);

  metadata->async_load(std::bind( &S3GetObjectAction::next, this));
}
*/

void S3GetObjectAction::read_object() {
  printf("Called S3GetObjectAction::read_object\n");
  clovis_reader = std::make_shared<S3ClovisReader>(request);
  clovis_reader->read_object(std::bind( &S3GetObjectAction::next, this), std::bind( &S3GetObjectAction::read_object_failed, this));
}

void S3GetObjectAction::read_object_failed() {
  // TODO - do anything more for failure?
  printf("Called S3GetObjectAction::read_object_failed\n");
  send_response_to_s3_client();
}


void S3GetObjectAction::send_response_to_s3_client() {
  printf("Called S3GetObjectAction::send_response_to_s3_client\n");
  // Trigger metadata read async operation with callback
  if (clovis_reader->get_state() == S3ClovisReaderOpState::complete) {
    std::string etag_key("etag");
    std::string content_len_key("Content-Length");
    request->set_out_header_value(content_len_key, clovis_reader->get_content_length_str());
    request->set_out_header_value(etag_key, clovis_reader->get_content_md5());

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
