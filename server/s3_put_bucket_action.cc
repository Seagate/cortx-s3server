/*
 * COPYRIGHT 2015 SEAGATE LLC
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
 * Original creation date: 1-Oct-2015
 */

#include <functional>

#include "s3_error_codes.h"
#include "s3_log.h"
#include "s3_put_bucket_action.h"

S3PutBucketAction::S3PutBucketAction(
    std::shared_ptr<S3RequestObject> req,
    std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory,
    std::shared_ptr<S3PutBucketBodyFactory> bucket_body_factory)
    : S3Action(req) {
  s3_log(S3_LOG_DEBUG, "Constructor\n");
  location_constraint = "";
  if (bucket_meta_factory) {
    bucket_metadata_factory = bucket_meta_factory;
  } else {
    bucket_metadata_factory = std::make_shared<S3BucketMetadataFactory>();
  }
  if (bucket_body_factory) {
    put_bucketbody_factory = bucket_body_factory;
  } else {
    put_bucketbody_factory = std::make_shared<S3PutBucketBodyFactory>();
  }
  setup_steps();
}

void S3PutBucketAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, "Setting up the action\n");
  add_task(std::bind(&S3PutBucketAction::validate_request, this));
  add_task(std::bind(&S3PutBucketAction::read_metadata, this));
  add_task(std::bind(&S3PutBucketAction::create_bucket, this));
  add_task(std::bind(&S3PutBucketAction::send_response_to_s3_client, this));
  // ...
}

void S3PutBucketAction::validate_request() {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  if (request->has_all_body_content()) {
    validate_request_body(request->get_full_body_content_as_string());
  } else {
    // Start streaming, logically pausing action till we get data.
    request->listen_for_incoming_data(
        std::bind(&S3PutBucketAction::consume_incoming_content, this),
        request->get_data_length() /* we ask for all */
        );
  }

  // for shutdown testcases, check FI and set shutdown signal
  S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL(
      "put_bucket_action_validate_request_shutdown_fail");
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PutBucketAction::consume_incoming_content() {
  s3_log(S3_LOG_DEBUG, "Consume data\n");
  if (request->has_all_body_content()) {
    validate_request_body(request->get_full_body_content_as_string());
  } else {
    // else just wait till entire body arrives. rare.
    request->resume();
  }
}

void S3PutBucketAction::validate_request_body(std::string content) {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  // S3PutBucketBody bucket(content);
  put_bucket_body = put_bucketbody_factory->create_put_bucket_body(content);
  if (put_bucket_body->isOK()) {
    location_constraint = put_bucket_body->get_location_constraint();
    next();
  } else {
    invalid_request = true;
    set_s3_error("MalformedXML");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PutBucketAction::read_metadata() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  // Trigger metadata read async operation with callback
  bucket_metadata =
      bucket_metadata_factory->create_bucket_metadata_obj(request);
  bucket_metadata->load(std::bind(&S3PutBucketAction::next, this),
                        std::bind(&S3PutBucketAction::next, this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PutBucketAction::create_bucket() {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  // Trigger metadata write async operation with callback
  // XXX Check if last step was successful.
  if (bucket_metadata->get_state() == S3BucketMetadataState::present) {
    s3_log(S3_LOG_WARN, "Bucket already exists\n");

    // Report 409 bucket exists.
    set_s3_error("BucketAlreadyExists");
    send_response_to_s3_client();
  } else if (bucket_metadata->get_state() == S3BucketMetadataState::missing) {
    // xxx set attributes & save
    if (!location_constraint.empty()) {
      bucket_metadata->set_location_constraint(location_constraint);
    }
    // bypass shutdown signal check for next task
    check_shutdown_signal_for_next_task(false);
    bucket_metadata->save(std::bind(&S3PutBucketAction::next, this),
                          std::bind(&S3PutBucketAction::next, this));
  } else {
    set_s3_error("InternalError");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PutBucketAction::send_response_to_s3_client() {
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
  } else if (bucket_metadata->get_state() == S3BucketMetadataState::saved) {
    // request->set_header_value(...)
    request->send_response(S3HttpSuccess200);
  } else {
    S3Error error("InternalError", request->get_request_id(),
                  request->get_bucket_name());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));

    request->send_response(error.get_http_status_code(), response_xml);
  }
  S3_RESET_SHUTDOWN_SIGNAL;  // for shutdown testcases
  done();
  i_am_done();  // self delete
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}
