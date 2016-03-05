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

#include "s3_delete_bucket_action.h"
#include "s3_error_codes.h"
#include "s3_log.h"

S3DeleteBucketAction::S3DeleteBucketAction(std::shared_ptr<S3RequestObject> req) : S3Action(req), last_key(""), is_bucket_empty(false), delete_successful(false) {
  s3_log(S3_LOG_DEBUG, "Constructor\n");
  setup_steps();
}

void S3DeleteBucketAction::setup_steps(){
  s3_log(S3_LOG_DEBUG, "Setting up the action\n");
  add_task(std::bind( &S3DeleteBucketAction::fetch_bucket_metadata, this ));
  add_task(std::bind( &S3DeleteBucketAction::fetch_first_object_metadata, this ));
  add_task(std::bind( &S3DeleteBucketAction::delete_bucket, this ));
  add_task(std::bind( &S3DeleteBucketAction::send_response_to_s3_client, this ));
  // ...
}

void S3DeleteBucketAction::fetch_bucket_metadata() {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  // Trigger metadata read async operation with callback
  bucket_metadata = std::make_shared<S3BucketMetadata>(request);
  bucket_metadata->load(std::bind( &S3DeleteBucketAction::next, this), std::bind( &S3DeleteBucketAction::next, this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3DeleteBucketAction::fetch_first_object_metadata() {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  if (bucket_metadata->get_state() == S3BucketMetadataState::present) {
    clovis_kv_reader = std::make_shared<S3ClovisKVSReader>(request);
    // Try to fetch one object at least
    clovis_kv_reader->next_keyval(get_bucket_index_name(), last_key, 1, std::bind( &S3DeleteBucketAction::fetch_first_object_metadata_successful, this), std::bind( &S3DeleteBucketAction::fetch_first_object_metadata_failed, this));
  } else {
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3DeleteBucketAction::fetch_first_object_metadata_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  is_bucket_empty = false;
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3DeleteBucketAction::fetch_first_object_metadata_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (clovis_kv_reader->get_state() == S3ClovisKVSReaderOpState::missing) {
    s3_log(S3_LOG_DEBUG, "There is no object in bucket\n");
    is_bucket_empty = true;
    next();
  } else {
    is_bucket_empty = false;
    s3_log(S3_LOG_ERROR, "Failed to retrieve object metadata\n");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3DeleteBucketAction::delete_bucket() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  bucket_metadata->remove(std::bind( &S3DeleteBucketAction::delete_bucket_successful, this), std::bind( &S3DeleteBucketAction::delete_bucket_failed, this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3DeleteBucketAction::delete_bucket_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  delete_successful = true;
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3DeleteBucketAction::delete_bucket_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  s3_log(S3_LOG_ERROR, "Bucket deletion failed\n");
  delete_successful = false;
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3DeleteBucketAction::send_response_to_s3_client() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  // Trigger metadata read async operation with callback
  if (bucket_metadata->get_state() == S3BucketMetadataState::missing) {
    S3Error error("NoSuchBucket", request->get_request_id(), request->get_bucket_name());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length", std::to_string(response_xml.length()));

    request->send_response(error.get_http_status_code(), response_xml);
  } else if (is_bucket_empty == false) {
    // Bucket not empty, cannot delete
    S3Error error("BucketNotEmpty", request->get_request_id(), request->get_bucket_name());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length", std::to_string(response_xml.length()));

    request->send_response(error.get_http_status_code(), response_xml);
  } else if (delete_successful) {
    request->send_response(S3HttpSuccess204);
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
