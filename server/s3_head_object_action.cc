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

#include "s3_head_object_action.h"
#include "s3_error_codes.h"

S3HeadObjectAction::S3HeadObjectAction(std::shared_ptr<S3RequestObject> req) : S3Action(req) {
  setup_steps();
}

void S3HeadObjectAction::setup_steps(){
  add_task(std::bind( &S3HeadObjectAction::fetch_bucket_info, this ));
  add_task(std::bind( &S3HeadObjectAction::fetch_object_info, this ));
  add_task(std::bind( &S3HeadObjectAction::send_response_to_s3_client, this ));
  // ...
}

void S3HeadObjectAction::fetch_bucket_info() {
  printf("Called S3HeadObjectAction::fetch_bucket_info\n");
  bucket_metadata = std::make_shared<S3BucketMetadata>(request);
  bucket_metadata->load(std::bind( &S3HeadObjectAction::next, this), std::bind( &S3HeadObjectAction::next, this));
}

void S3HeadObjectAction::fetch_object_info() {
  printf("Called S3HeadObjectAction::fetch_bucket_info\n");
  if (bucket_metadata->get_state() == S3BucketMetadataState::present) {
    object_metadata = std::make_shared<S3ObjectMetadata>(request);

    object_metadata->load(std::bind( &S3HeadObjectAction::next, this), std::bind( &S3HeadObjectAction::next, this));
  } else {
    send_response_to_s3_client();
  }
}

void S3HeadObjectAction::send_response_to_s3_client() {
  printf("Called S3HeadObjectAction::send_response_to_s3_client\n");
  // Trigger metadata read async operation with callback
  if (bucket_metadata->get_state() == S3BucketMetadataState::missing) {
    // Invalid Bucket Name
    S3Error error("NoSuchBucket", request->get_request_id(), request->get_bucket_name());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length", std::to_string(response_xml.length()));

    request->send_response(error.get_http_status_code(), response_xml);
  } else if (object_metadata->get_state() == S3ObjectMetadataState::missing) {
    S3Error error("NoSuchKey", request->get_request_id(), request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length", std::to_string(response_xml.length()));

    request->send_response(error.get_http_status_code(), response_xml);
  } else if (object_metadata->get_state() == S3ObjectMetadataState::present) {
    request->set_out_header_value("Last-Modified", object_metadata->get_last_modified());
    request->set_out_header_value("ETag", object_metadata->get_md5());
    request->set_out_header_value("Accept-Ranges", "bytes");
    request->set_out_header_value("Content-Length", object_metadata->get_content_length_str());

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
}
