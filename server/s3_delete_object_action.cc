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

#include "s3_delete_object_action.h"
#include "s3_error_codes.h"

S3DeleteObjectAction::S3DeleteObjectAction(std::shared_ptr<S3RequestObject> req)
    : S3Action(req, false) {
  s3_log(S3_LOG_DEBUG, "Constructor\n");
  setup_steps();
}

void S3DeleteObjectAction::setup_steps(){
  s3_log(S3_LOG_DEBUG, "Setup the action\n");
  add_task(std::bind( &S3DeleteObjectAction::fetch_bucket_info, this ));
  add_task(std::bind( &S3DeleteObjectAction::fetch_object_info, this ));
  add_task(std::bind( &S3DeleteObjectAction::delete_object, this ));
  add_task(std::bind( &S3DeleteObjectAction::delete_metadata, this ));
  add_task(std::bind( &S3DeleteObjectAction::send_response_to_s3_client, this ));
  // ...
}

void S3DeleteObjectAction::fetch_bucket_info() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  bucket_metadata = std::make_shared<S3BucketMetadata>(request);
  bucket_metadata->load(std::bind( &S3DeleteObjectAction::next, this), std::bind( &S3DeleteObjectAction::next, this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3DeleteObjectAction::fetch_object_info() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (bucket_metadata->get_state() == S3BucketMetadataState::present) {
    object_metadata = std::make_shared<S3ObjectMetadata>(request);

    object_metadata->load(std::bind( &S3DeleteObjectAction::next, this), std::bind( &S3DeleteObjectAction::next, this));
  } else {
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3DeleteObjectAction::delete_object() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (object_metadata->get_state() == S3ObjectMetadataState::present) {
    clovis_writer = std::make_shared<S3ClovisWriter>(request, object_metadata->get_oid());
    clovis_writer->delete_object(std::bind( &S3DeleteObjectAction::next, this), std::bind( &S3DeleteObjectAction::delete_object_failed, this));
  } else {
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3DeleteObjectAction::delete_object_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (clovis_writer->get_state() == S3ClovisWriterOpState::missing) {
    next();
  } else {
    // Any other error report failure.
    s3_log(S3_LOG_ERROR, "Deletion of object failed\n");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3DeleteObjectAction::delete_metadata() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  object_metadata = std::make_shared<S3ObjectMetadata>(
      request, bucket_metadata->get_object_list_index_oid());
  object_metadata->remove(std::bind( &S3DeleteObjectAction::next, this), std::bind( &S3DeleteObjectAction::next, this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3DeleteObjectAction::send_response_to_s3_client() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (bucket_metadata->get_state() == S3BucketMetadataState::missing) {
    // Invalid Bucket Name
    S3Error error("NoSuchBucket", request->get_request_id(), request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length", std::to_string(response_xml.length()));

    request->send_response(error.get_http_status_code(), response_xml);
  } else if ((object_metadata->get_state() == S3ObjectMetadataState::missing) ||
      (object_metadata->get_state() == S3ObjectMetadataState::deleted)) {
    request->send_response(S3HttpSuccess204);
  } else {
    S3Error error("InternalError", request->get_request_id(), request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length", std::to_string(response_xml.length()));

    request->send_response(error.get_http_status_code(), response_xml);
  }
  done();
  i_am_done();  // self delete
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}
