#include "s3_get_bucket_tagging_action.h"
#include "s3_error_codes.h"
#include "s3_log.h"

S3GetBucketTaggingAction::S3GetBucketTaggingAction(
    std::shared_ptr<S3RequestObject> req,
    std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory)
    : S3BucketAction(std::move(req), std::move(bucket_meta_factory), false) {
  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");

  s3_log(S3_LOG_INFO, request_id, "S3 API: Get Bucket Tagging. Bucket[%s]\n",
         request->get_bucket_name().c_str());

  setup_steps();
}

void S3GetBucketTaggingAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setting up the action\n");
  ACTION_TASK_ADD(S3GetBucketTaggingAction::check_metadata_missing_status,
                  this);
  ACTION_TASK_ADD(S3GetBucketTaggingAction::send_response_to_s3_client, this);
  // ...
}

void S3GetBucketTaggingAction::check_metadata_missing_status() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (!(bucket_metadata->check_bucket_tags_exists())) {
    set_s3_error("NoSuchTagSetError");
  }
  next();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3GetBucketTaggingAction::fetch_bucket_info_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering get_metadata_failed\n");
  if (bucket_metadata->get_state() == S3BucketMetadataState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id,
           "Bucket metadata load operation failed due to pre launch failure\n");
    set_s3_error("ServiceUnavailable");
  } else if (bucket_metadata->get_state() == S3BucketMetadataState::missing) {
    s3_log(S3_LOG_ERROR, request_id, "Bucket metadata load operation failed\n");
    set_s3_error("NoSuchBucket");
  } else {
    set_s3_error("InternalError");
    s3_log(S3_LOG_ERROR, request_id,
           "Bucket metadata load operation failed due to internal error\n");
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_INFO, "", "Exiting\n");
}

void S3GetBucketTaggingAction::send_response_to_s3_client() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  if (reject_if_shutting_down() ||
      (is_error_state() && !get_s3_error_code().empty())) {
    S3Error error(get_s3_error_code(), request->get_request_id(),
                  request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));
    if (get_s3_error_code() == "ServiceUnavailable") {
      request->set_out_header_value("Retry-After", "1");
    }

    request->send_response(error.get_http_status_code(), response_xml);
  } else {
    request->send_response(S3HttpSuccess200,
                           bucket_metadata->get_tags_as_xml());
  }

  s3_log(S3_LOG_INFO, "", "Exiting\n");
  done();
}
