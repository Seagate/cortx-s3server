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
 * Original author:  Rajesh Nambiar <rajesh.nambiar@seagate.com>
 * Original author:  Abrarahmed Momin   <abrar.habib@seagate.com>
 * Original creation date: 23-May-2016
 */

#include "s3_get_object_acl_action.h"
#include "s3_error_codes.h"
#include "s3_log.h"
#include "s3_object_acl.h"

S3GetObjectACLAction::S3GetObjectACLAction(
    std::shared_ptr<S3RequestObject> req,
    std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory,
    std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory)
    : S3Action(req) {
  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");
  if (bucket_meta_factory) {
    bucket_metadata_factory = bucket_meta_factory;
  } else {
    bucket_metadata_factory = std::make_shared<S3BucketMetadataFactory>();
  }
  if (object_meta_factory) {
    object_metadata_factory = object_meta_factory;
  } else {
    object_metadata_factory = std::make_shared<S3ObjectMetadataFactory>();
  }
  object_list_index_oid = {0ULL, 0ULL};
  setup_steps();
}

void S3GetObjectACLAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setting up the action\n");
  add_task(std::bind(&S3GetObjectACLAction::fetch_bucket_info, this));
  add_task(std::bind(&S3GetObjectACLAction::get_object_metadata, this));
  add_task(std::bind(&S3GetObjectACLAction::send_response_to_s3_client, this));
  // ...
}

void S3GetObjectACLAction::fetch_bucket_info() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  bucket_metadata =
      bucket_metadata_factory->create_bucket_metadata_obj(request);
  bucket_metadata->load(
      std::bind(&S3GetObjectACLAction::next, this),
      std::bind(&S3GetObjectACLAction::fetch_bucket_info_failed, this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3GetObjectACLAction::fetch_bucket_info_failed() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if (bucket_metadata->get_state() == S3BucketMetadataState::missing) {
    set_s3_error("NoSuchBucket");
  } else {
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3GetObjectACLAction::get_object_metadata() {
  s3_log(S3_LOG_DEBUG, request_id, "Fetching object metadata\n");
  // bypass shutdown signal check for next task
  check_shutdown_signal_for_next_task(false);
  object_list_index_oid = bucket_metadata->get_object_list_index_oid();
  if (object_list_index_oid.u_lo == 0ULL &&
      object_list_index_oid.u_hi == 0ULL) {
    // There is no object list index, hence object doesn't exist
    s3_log(S3_LOG_DEBUG, request_id, "Object not found\n");
    set_s3_error("NoSuchKey");
    send_response_to_s3_client();
  } else {
    object_metadata = object_metadata_factory->create_object_metadata_obj(
        request, object_list_index_oid);
    object_metadata->load(
        std::bind(&S3GetObjectACLAction::next, this),
        std::bind(&S3GetObjectACLAction::get_object_metadata_failed, this));
  }
}

void S3GetObjectACLAction::get_object_metadata_failed() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if (object_metadata->get_state() == S3ObjectMetadataState::missing) {
    set_s3_error("NoSuchKey");
  } else {
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3GetObjectACLAction::send_response_to_s3_client() {
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
  } else if (object_metadata->get_state() == S3ObjectMetadataState::present) {
    std::string response_xml = object_metadata->get_acl_as_xml();
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
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  i_am_done();  // self delete
}
