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
 * Original creation date: 6-Jan-2016
 */

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <unistd.h>

#include "s3_abort_multipart_action.h"
#include "s3_error_codes.h"

S3AbortMultipartAction::S3AbortMultipartAction(std::shared_ptr<S3RequestObject> req) : S3Action(req) {
  s3_log(S3_LOG_DEBUG, "Constructor\n");
  upload_id = request->get_query_string_value("uploadId");
  object_name = request->get_object_name();
  bucket_name = request->get_bucket_name();
  setup_steps();
}

void S3AbortMultipartAction::setup_steps(){
  s3_log(S3_LOG_DEBUG, "Setup the action\n");
  add_task(std::bind( &S3AbortMultipartAction::fetch_bucket_info, this ));
  add_task(std::bind( &S3AbortMultipartAction::delete_multipart_metadata, this));
  add_task(std::bind( &S3AbortMultipartAction::fetch_parts_info, this ));
  add_task(std::bind( &S3AbortMultipartAction::delete_parts, this));
  add_task(std::bind( &S3AbortMultipartAction::delete_part_index, this));
  add_task(std::bind( &S3AbortMultipartAction::send_response_to_s3_client, this ));
  // ...
}

void S3AbortMultipartAction::fetch_bucket_info() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  bucket_metadata = std::make_shared<S3BucketMetadata>(request);
  bucket_metadata->load(std::bind( &S3AbortMultipartAction::next, this), std::bind( &S3AbortMultipartAction::next, this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3AbortMultipartAction::fetch_parts_info() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  part_metadata = std::make_shared<S3PartMetadata>(request, upload_id, 1);
  part_metadata->load(std::bind( &S3AbortMultipartAction::next, this), std::bind( &S3AbortMultipartAction::fetch_parts_failed, this), 1);
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3AbortMultipartAction::fetch_parts_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  s3_log(S3_LOG_ERROR, "Fetching of parts failed\n");
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3AbortMultipartAction::delete_parts() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  clovis_writer = std::make_shared<S3ClovisWriter>(request);
  clovis_writer->delete_object(std::bind( &S3AbortMultipartAction::next, this), std::bind( &S3AbortMultipartAction::delete_parts_failed, this));
  s3_log(S3_LOG_ERROR, "Exiting\n");
}

void S3AbortMultipartAction::delete_parts_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  s3_log(S3_LOG_ERROR, "Deletion of parts failed\n");
  send_response_to_s3_client();
  s3_log(S3_LOG_ERROR, "Exiting\n");
}


void S3AbortMultipartAction::delete_multipart_metadata() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  object_multipart_metadata = std::make_shared<S3ObjectMetadata>(request, true, upload_id);
  object_multipart_metadata->remove(std::bind( &S3AbortMultipartAction::next, this), std::bind( &S3AbortMultipartAction::delete_multipart_failed, this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3AbortMultipartAction::delete_multipart_failed() {
  //Log error
  s3_log(S3_LOG_DEBUG, "Entering\n");
  s3_log(S3_LOG_ERROR, "Deletion of multipart index failed\n");
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3AbortMultipartAction::delete_part_index() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  part_metadata->remove_index(std::bind( &S3AbortMultipartAction::next, this), std::bind( &S3AbortMultipartAction::next, this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}


void S3AbortMultipartAction::send_response_to_s3_client() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if ( bucket_metadata && (bucket_metadata->get_state() == S3BucketMetadataState::missing)) {
    // Invalid Bucket Name
    S3Error error("NoSuchBucket", request->get_request_id(), request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->send_response(error.get_http_status_code(), response_xml);
    } else if(object_multipart_metadata && (object_multipart_metadata->get_state() == S3ObjectMetadataState::missing)) {
      S3Error error("NoSuchUpload", request->get_request_id(), request->get_object_uri());
      std::string& response_xml = error.to_xml();
      request->set_out_header_value("Content-Type", "application/xml");
      request->send_response(error.get_http_status_code(), response_xml);
    } else if ((clovis_writer && (clovis_writer->get_state() == S3ClovisWriterOpState::deleted)) ||
               (part_metadata && (part_metadata->get_state() != S3PartMetadataState::failed)) ||
               (object_multipart_metadata && (object_multipart_metadata->get_state() != S3ObjectMetadataState::failed))) {
      request->send_response(S3HttpSuccess200);
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
