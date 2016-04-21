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

#include "s3_post_multipartobject_action.h"
#include "s3_uri_to_mero_oid.h"
#include "s3_error_codes.h"
#include "s3_log.h"

#define MAX_COLLISION_RETRY 20

S3PostMultipartObjectAction::S3PostMultipartObjectAction(std::shared_ptr<S3RequestObject> req) : S3Action(req) {
  s3_log(S3_LOG_DEBUG, "Constructor\n");
  S3UriToMeroOID(request->get_object_uri().c_str(), &oid);
  tried_count = 0;
  salt = "uri_salt_";
  setup_steps();
}

void S3PostMultipartObjectAction::setup_steps(){
  s3_log(S3_LOG_DEBUG, "Setting up the action\n");
  add_task(std::bind( &S3PostMultipartObjectAction::fetch_bucket_info, this ));
  add_task(std::bind( &S3PostMultipartObjectAction::check_upload_is_inprogress, this ));
  add_task(std::bind( &S3PostMultipartObjectAction::create_object, this ));
  add_task(std::bind( &S3PostMultipartObjectAction::save_upload_metadata, this ));
  add_task(std::bind( &S3PostMultipartObjectAction::create_part_meta_index, this ));
  add_task(std::bind( &S3PostMultipartObjectAction::send_response_to_s3_client, this ));
  // ...
}

void S3PostMultipartObjectAction::fetch_bucket_info() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  bucket_metadata = std::make_shared<S3BucketMetadata>(request);
  bucket_metadata->load(std::bind( &S3PostMultipartObjectAction::next, this), std::bind( &S3PostMultipartObjectAction::next, this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PostMultipartObjectAction::check_upload_is_inprogress() {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  if (bucket_metadata->get_state() == S3BucketMetadataState::present) {
    S3Uuid uuid;
    upload_id = uuid.get_string_uuid();
    object_multipart_metadata = std::make_shared<S3ObjectMetadata>(request, true, upload_id);
    object_multipart_metadata->load(std::bind( &S3PostMultipartObjectAction::next, this), std::bind( &S3PostMultipartObjectAction::next, this));
  } else {
    s3_log(S3_LOG_WARN, "Missing bucket [%s]\n", request->get_bucket_name().c_str());
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PostMultipartObjectAction::create_object() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (object_multipart_metadata->get_state() != S3ObjectMetadataState::present) {
    create_object_timer.start();
    clovis_writer = std::make_shared<S3ClovisWriter>(request, oid);
    clovis_writer->create_object(std::bind( &S3PostMultipartObjectAction::next, this), std::bind( &S3PostMultipartObjectAction::create_object_failed, this));
  } else {
    //++
    // Bailing out in case if the multipart upload is already in progress,
    // this will ensure that object doesn't go to inconsistent state

    // Once multipart is taken care in mero, it may not be needed
    // Note - Currently as per aws doc. (http://docs.aws.amazon.com/AmazonS3/latest/API/mpUploadListMPUpload.html) multipart upload details
    // of the same key initiated at different times is allowed.
    //--
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PostMultipartObjectAction::create_object_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  create_object_timer.stop();
  LOG_PERF("create_object_failed_ms", create_object_timer.elapsed_time_in_millisec());

  if (clovis_writer->get_state() == S3ClovisWriterOpState::exists) {
    // If object exists, it may be due to the actual existance of object or due to oid collision.
    if (tried_count) { // No need of lookup of metadata in case if it was oid collision before
      collision_occured();
    } else {
       object_metadata = std::make_shared<S3ObjectMetadata>(request);
       // Lookup object metadata, if the object doesn't exist then its collision, do collision resolution
       // If object exist in metadata then we overwrite it
       object_metadata->load(std::bind( &S3PostMultipartObjectAction::next, this), std::bind( &S3PostMultipartObjectAction::collision_occured, this));
    }
  } else {
    s3_log(S3_LOG_WARN, "Create object failed.\n");
    // TODO - rollback
    // Any other error report failure.
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PostMultipartObjectAction::collision_occured() {
  if(object_metadata->get_state() == S3ObjectMetadataState::missing && tried_count < MAX_COLLISION_RETRY) {
    s3_log(S3_LOG_INFO, "Object ID collision happened for uri %s\n", request->get_object_uri().c_str());
    // Handle Collision
    create_new_oid();
    tried_count++;
    if (tried_count > 5) {
      s3_log(S3_LOG_INFO, "Object ID collision happened %d times for uri %s\n", tried_count, request->get_object_uri().c_str());
    }
    create_object();
  } else {
    if (tried_count > MAX_COLLISION_RETRY) {
      s3_log(S3_LOG_ERROR, "Failed to resolve object id collision %d times for uri %s\n",
             tried_count, request->get_object_uri().c_str());
    }
    send_response_to_s3_client();
  }
}

void S3PostMultipartObjectAction::create_new_oid() {
  std::string salted_uri = request->get_object_uri() + salt + std::to_string(tried_count);
  S3UriToMeroOID(salted_uri.c_str(), &oid);
  return;
}

void S3PostMultipartObjectAction::save_upload_metadata() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  create_object_timer.stop();
  LOG_PERF("create_object_successful_ms", create_object_timer.elapsed_time_in_millisec());

  for (auto it: request->get_in_headers_copy()) {
    if(it.first.find("x-amz-meta-") != std::string::npos) {
      object_multipart_metadata->add_user_defined_attribute(it.first, it.second);
    }
  }
  object_multipart_metadata->set_oid(oid);
  object_multipart_metadata->save(std::bind( &S3PostMultipartObjectAction::next, this), std::bind( &S3PostMultipartObjectAction::save_upload_metadata_failed, this));

  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PostMultipartObjectAction::save_upload_metadata_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  // TODO rollback
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PostMultipartObjectAction::create_part_meta_index() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  part_metadata = std::make_shared<S3PartMetadata>(request, upload_id, 0);
  part_metadata->create_index(std::bind( &S3PostMultipartObjectAction::next, this), std::bind( &S3PostMultipartObjectAction::next, this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PostMultipartObjectAction::send_response_to_s3_client() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (bucket_metadata->get_state() == S3BucketMetadataState::missing) {
    // Invalid Bucket Name
    S3Error error("NoSuchBucket", request->get_request_id(), request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");

    request->send_response(error.get_http_status_code(), response_xml);
  } else if (object_multipart_metadata->get_state() == S3ObjectMetadataState::present) {
    S3Error error("InvalidObjectState", request->get_request_id(), request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length", std::to_string(response_xml.length()));
    request->send_response(error.get_http_status_code(), response_xml);
  } else if ((object_multipart_metadata->get_state() == S3ObjectMetadataState::saved) &&
    ((part_metadata->get_state() == S3PartMetadataState::store_created) || (part_metadata->get_state() == S3PartMetadataState::present))) {
    std::string response;
    std::string object_name = request->get_object_name();
    std::string bucket_name = request->get_bucket_name();
    response = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    response += "<InitiateMultipartUploadResult xmlns=\"http://s3\">\n";
    response += "<Bucket>" + bucket_name + "</Bucket>\n";
    response += "<Key>" + object_name + "</Key>\n";
    response += "<UploadId>" + upload_id + "</UploadId>";
    response += "</InitiateMultipartUploadResult>";
    request->set_out_header_value("UploadId", upload_id);
    request->send_response(S3HttpSuccess200, response);
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
