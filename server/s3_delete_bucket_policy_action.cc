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
 * Original author:  Rajesh Nambiar  <rajesh.nambiar@seagate.com>
 * Original creation date: 23-May-2016
 */

#include "s3_delete_bucket_policy_action.h"
#include "s3_error_codes.h"
#include "s3_log.h"

S3DeleteBucketPolicyAction::S3DeleteBucketPolicyAction(
    std::shared_ptr<S3RequestObject> req,
    std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory)
    : S3Action(req, false), delete_successful(false) {
  s3_log(S3_LOG_DEBUG, "Constructor\n");

  if (bucket_meta_factory) {
    bucket_metadata_factory = bucket_meta_factory;
  } else {
    bucket_metadata_factory = std::make_shared<S3BucketMetadataFactory>();
  }
  setup_steps();
}

void S3DeleteBucketPolicyAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, "Setting up the action\n");
  add_task(std::bind(&S3DeleteBucketPolicyAction::fetch_bucket_metadata, this));
  add_task(std::bind(&S3DeleteBucketPolicyAction::delete_bucket_policy, this));
  add_task(
      std::bind(&S3DeleteBucketPolicyAction::send_response_to_s3_client, this));
  // ...
}

void S3DeleteBucketPolicyAction::fetch_bucket_metadata() {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  // Trigger metadata read async operation with callback
  bucket_metadata =
      bucket_metadata_factory->create_bucket_metadata_obj(request);
  bucket_metadata->load(std::bind(&S3DeleteBucketPolicyAction::next, this),
                        std::bind(&S3DeleteBucketPolicyAction::next, this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3DeleteBucketPolicyAction::delete_bucket_policy() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (bucket_metadata->get_state() == S3BucketMetadataState::present) {
    bucket_metadata->deletepolicy();
    bucket_metadata->save(
        std::bind(&S3DeleteBucketPolicyAction::delete_bucket_policy_successful,
                  this),
        std::bind(&S3DeleteBucketPolicyAction::delete_bucket_policy_failed,
                  this));
  } else {
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3DeleteBucketPolicyAction::delete_bucket_policy_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  delete_successful = true;
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3DeleteBucketPolicyAction::delete_bucket_policy_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  s3_log(S3_LOG_ERROR, "Bucket policy deletion failed\n");
  delete_successful = false;
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3DeleteBucketPolicyAction::send_response_to_s3_client() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  // Trigger metadata read async operation with callback
  if (bucket_metadata->get_state() == S3BucketMetadataState::missing) {
    S3Error error("NoSuchBucket", request->get_request_id(),
                  request->get_bucket_name());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));

    request->send_response(error.get_http_status_code(), response_xml);
  } else if (bucket_metadata->get_state() == S3BucketMetadataState::failed) {
    S3Error error("InternalError", request->get_request_id(),
                  request->get_bucket_name());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));
    request->send_response(error.get_http_status_code(), response_xml);
  } else if (delete_successful) {
    request->send_response(S3HttpSuccess204);
  } else {
    S3Error error("InternalError", request->get_request_id(),
                  request->get_bucket_name());
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
