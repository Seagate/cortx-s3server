
#include "s3_get_bucket_location_action.h"
#include "s3_error_codes.h"

S3GetBucketlocationAction::S3GetBucketlocationAction(std::shared_ptr<S3RequestObject> req) : S3Action(req) {
  setup_steps();
}

void S3GetBucketlocationAction::setup_steps(){
  add_task(std::bind( &S3GetBucketlocationAction::get_metadata, this ));
  add_task(std::bind( &S3GetBucketlocationAction::send_response_to_s3_client, this ));
  // ...
}


void S3GetBucketlocationAction::get_metadata() {
  // Trigger metadata read async operation with callback
  // For now just call next.
  next();
}


void S3GetBucketlocationAction::send_response_to_s3_client() {
  printf("Called S3GetBucketlocationAction::send_response_to_s3_client\n");

  std::string response;
  response = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
  response += "<LocationConstraint xmlns=\"http://s3\">";
  response += "</LocationConstraint>";
  request->send_response(S3HttpSuccess200, response);

  done();
  i_am_done();  // self delete
}
