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

#include "s3_object_acl.h"
#include "s3_put_object_acl_action.h"
#include "s3_error_codes.h"
#include "s3_log.h"

S3PutObjectACLAction::S3PutObjectACLAction(std::shared_ptr<S3RequestObject> req) : S3Action(req) {
  s3_log(S3_LOG_DEBUG, "Constructor\n");
  setup_steps();
}

void S3PutObjectACLAction::setup_steps(){
  s3_log(S3_LOG_DEBUG, "Setting up the action\n");
  add_task(std::bind( &S3PutObjectACLAction::fetch_bucket_info, this ));
  add_task(std::bind( &S3PutObjectACLAction::get_object_metadata, this ));
  add_task(std::bind( &S3PutObjectACLAction::setacl, this ));
  add_task(std::bind( &S3PutObjectACLAction::send_response_to_s3_client, this ));
}

void S3PutObjectACLAction::fetch_bucket_info() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  bucket_metadata = std::make_shared<S3BucketMetadata>(request);
  bucket_metadata->load(std::bind( &S3PutObjectACLAction::next, this), std::bind( &S3PutObjectACLAction::send_response_to_s3_client, this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PutObjectACLAction::get_object_metadata() {
  s3_log(S3_LOG_DEBUG, "Fetching object metadata\n");
  object_metadata = std::make_shared<S3ObjectMetadata>(request);
  object_metadata->load(std::bind( &S3PutObjectACLAction::next, this), std::bind( &S3PutObjectACLAction::next, this));
}

void S3PutObjectACLAction::setacl() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (object_metadata->get_state() == S3ObjectMetadataState::present) {
    object_metadata->setacl(request->get_full_body_content_as_string());
    object_metadata->save_metadata(std::bind( &S3PutObjectACLAction::next, this), std::bind( &S3PutObjectACLAction::next, this));
  } else {
    next();
  }
}

void S3PutObjectACLAction::send_response_to_s3_client() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (bucket_metadata->get_state() != S3BucketMetadataState::present) {
    S3Error error("NoSuchBucket", request->get_request_id(), request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length", std::to_string(response_xml.length()));
    request->send_response(error.get_http_status_code(), response_xml);
  } else if (object_metadata->get_state() == S3ObjectMetadataState::saved) {
    request->send_response(S3HttpSuccess200);
  } else if (object_metadata->get_state() == S3ObjectMetadataState::missing) {
    S3Error error("NoSuchObject", request->get_request_id(), request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length", std::to_string(response_xml.length()));
    request->send_response(error.get_http_status_code(), response_xml);
  } else {
    S3Error error("InternalError", request->get_request_id(), request->get_bucket_name());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length", std::to_string(response_xml.length()));
    request->send_response(error.get_http_status_code(), response_xml);
  }
  done();
  i_am_done();  // self delete
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}
