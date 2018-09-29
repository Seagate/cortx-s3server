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
 * Original author:  Rajesh Nambiar   <rajesh.nambiar@seagate.com>
 * Original creation date: 19-May-2016
 */

#include "s3_put_bucket_acl_action.h"
#include "s3_bucket_acl.h"
#include "s3_error_codes.h"
#include "s3_log.h"

S3PutBucketACLAction::S3PutBucketACLAction(
    std::shared_ptr<S3RequestObject> req,
    std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory)
    : S3Action(req) {
  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");

  s3_log(S3_LOG_INFO, request_id, "S3 API: Put Bucket Acl. Bucket[%s]\n",
         request->get_bucket_name().c_str());

  if (bucket_meta_factory) {
    bucket_metadata_factory = bucket_meta_factory;
  } else {
    bucket_metadata_factory = std::make_shared<S3BucketMetadataFactory>();
  }

  setup_steps();
}

void S3PutBucketACLAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setting up the action\n");
  add_task(std::bind(&S3PutBucketACLAction::validate_request, this));
  add_task(std::bind(&S3PutBucketACLAction::fetch_bucket_info, this));
  add_task(std::bind(&S3PutBucketACLAction::setacl, this));
  add_task(std::bind(&S3PutBucketACLAction::send_response_to_s3_client, this));
}

void S3PutBucketACLAction::validate_request() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

  if (request->has_all_body_content()) {
    new_bucket_acl = request->get_full_body_content_as_string();
    validate_request_body(new_bucket_acl);
  } else {
    // Start streaming, logically pausing action till we get data.
    request->listen_for_incoming_data(
        std::bind(&S3PutBucketACLAction::consume_incoming_content, this),
        request->get_data_length() /* we ask for all */
        );
  }

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutBucketACLAction::consume_incoming_content() {
  s3_log(S3_LOG_DEBUG, request_id, "Consume data\n");
  if (request->has_all_body_content()) {
    new_bucket_acl = request->get_full_body_content_as_string();
    validate_request_body(new_bucket_acl);
  } else {
    // else just wait till entire body arrives. rare.
    request->resume();
  }
}

void S3PutBucketACLAction::validate_request_body(std::string content) {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

  // TODO: ACL implementation is partial, fix this when adding full support.
  // S3PutBucketAclBody bucket_acl(content);
  // if (bucket_acl.isOK()) {
  //   next();
  // } else {
  //   invalid_request = true;
  //   set_s3_error("MalformedXML");
  //   send_response_to_s3_client();
  // }
  next();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutBucketACLAction::fetch_bucket_info() {
  s3_log(S3_LOG_DEBUG, request_id, "Fetching bucket metadata\n");

  bucket_metadata =
      bucket_metadata_factory->create_bucket_metadata_obj(request);
  bucket_metadata->load(
      std::bind(&S3PutBucketACLAction::next, this),
      std::bind(&S3PutBucketACLAction::fetch_bucket_info_failed, this));

  // for shutdown testcases, check FI and set shutdown signal
  S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL(
      "put_bucket_acl_action_fetch_bucket_info_shutdown_fail");
}

void S3PutBucketACLAction::fetch_bucket_info_failed() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

  if (bucket_metadata->get_state() == S3BucketMetadataState::missing) {
    set_s3_error("NoSuchBucket");
  } else if (bucket_metadata->get_state() ==
             S3BucketMetadataState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id,
           "Bucket metadata load operation failed due to pre launch failure\n");
    set_s3_error("ServiceUnavailable");
  } else {
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutBucketACLAction::setacl() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

  bucket_metadata->setacl(new_bucket_acl);
  // bypass shutdown signal check for next task
  check_shutdown_signal_for_next_task(false);
  bucket_metadata->save(std::bind(&S3PutBucketACLAction::next, this),
                        std::bind(&S3PutBucketACLAction::setacl_failed, this));

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutBucketACLAction::setacl_failed() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  s3_log(S3_LOG_ERROR, request_id, "setting acl failed\n");
  if (bucket_metadata->get_state() == S3BucketMetadataState::failed_to_launch) {
    set_s3_error("ServiceUnavailable");
  } else if (bucket_metadata->get_state() == S3BucketMetadataState::missing) {
    set_s3_error("NoSuchBucket");
  } else {
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutBucketACLAction::send_response_to_s3_client() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

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
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  i_am_done();  // self delete
}
