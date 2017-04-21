/*
 * COPYRIGHT 2016 SEAGATE LLC
 *
 * THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
 * HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE TECHNOLOGY
 * LIMITED, ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
 * THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
 * BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
 * USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
 * EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.
 *
 * YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
 * THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
 * http://www.seagate.com/contact
 *
 * Original author:  Kaustubh Deorukhkar   <kaustubh.deorukhkar@seagate.com>
 * Original creation date: 31-Mar-2016
 */

#include "s3_get_bucket_acl_action.h"
#include "s3_bucket_acl.h"
#include "s3_error_codes.h"
#include "s3_log.h"

S3GetBucketACLAction::S3GetBucketACLAction(
    std::shared_ptr<S3RequestObject> req,
    std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory)
    : S3Action(req) {
  s3_log(S3_LOG_DEBUG, "Constructor\n");

  if (bucket_meta_factory) {
    bucket_metadata_factory = bucket_meta_factory;
  } else {
    bucket_metadata_factory = std::make_shared<S3BucketMetadataFactory>();
  }

  setup_steps();
}

void S3GetBucketACLAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, "Setting up the action\n");
  add_task(std::bind(&S3GetBucketACLAction::fetch_bucket_info, this));
  add_task(std::bind(&S3GetBucketACLAction::send_response_to_s3_client, this));
  // ...
}

void S3GetBucketACLAction::fetch_bucket_info() {
  s3_log(S3_LOG_DEBUG, "Fetching bucket metadata\n");
  bucket_metadata =
      bucket_metadata_factory->create_bucket_metadata_obj(request);

  // bypass shutdown signal check for next task
  check_shutdown_signal_for_next_task(false);
  bucket_metadata->load(
      std::bind(&S3GetBucketACLAction::next, this),
      std::bind(&S3GetBucketACLAction::fetch_bucket_info_failed, this));
}

void S3GetBucketACLAction::fetch_bucket_info_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (bucket_metadata->get_state() == S3BucketMetadataState::missing) {
    s3_log(S3_LOG_WARN, "Bucket not found\n");
    set_s3_error("NoSuchBucket");
  } else {
    s3_log(S3_LOG_WARN, "Failed to fetch Bucket metadata\n");
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3GetBucketACLAction::send_response_to_s3_client() {
  s3_log(S3_LOG_DEBUG, "Entering\n");

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
  } else if (bucket_metadata->get_state() == S3BucketMetadataState::present) {
    std::string response_xml = bucket_metadata->get_acl_as_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));
    request->send_response(S3HttpSuccess200, response_xml);
  } else {
    S3Error error("InternalError", request->get_request_id(),
                  request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));
    request->send_response(error.get_http_status_code(), response_xml);
  }

  done();
  i_am_done();  // self delete
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}
