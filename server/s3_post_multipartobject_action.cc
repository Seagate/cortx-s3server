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
#include "s3_error_codes.h"
#include "s3_iem.h"
#include "s3_log.h"
#include "s3_stats.h"
#include "s3_uri_to_mero_oid.h"

#define MAX_COLLISION_RETRY 20

S3PostMultipartObjectAction::S3PostMultipartObjectAction(
    std::shared_ptr<S3RequestObject> req)
    : S3Action(req) {
  s3_log(S3_LOG_DEBUG, "Constructor\n");
  S3UriToMeroOID(request->get_object_uri().c_str(), &oid);
  tried_count = 0;
  salt = "uri_salt_";
  setup_steps();
}

void S3PostMultipartObjectAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, "Setting up the action\n");
  add_task(std::bind(&S3PostMultipartObjectAction::fetch_bucket_info, this));
  add_task(std::bind(&S3PostMultipartObjectAction::check_upload_is_inprogress,
                     this));
  add_task(std::bind(&S3PostMultipartObjectAction::create_object, this));
  add_task(
      std::bind(&S3PostMultipartObjectAction::create_part_meta_index, this));
  add_task(std::bind(&S3PostMultipartObjectAction::save_upload_metadata, this));
  add_task(
      std::bind(&S3PostMultipartObjectAction::save_multipart_metadata, this));
  add_task(std::bind(&S3PostMultipartObjectAction::send_response_to_s3_client,
                     this));
  // ...
}

void S3PostMultipartObjectAction::fetch_bucket_info() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  bucket_metadata = std::make_shared<S3BucketMetadata>(request);
  bucket_metadata->load(std::bind(&S3PostMultipartObjectAction::next, this),
                        std::bind(&S3PostMultipartObjectAction::next, this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PostMultipartObjectAction::check_upload_is_inprogress() {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  if (bucket_metadata->get_state() == S3BucketMetadataState::present) {
    S3Uuid uuid;
    upload_id = uuid.get_string_uuid();
    multipart_index_oid = bucket_metadata->get_multipart_index_oid();
    object_multipart_metadata = std::make_shared<S3ObjectMetadata>(
        request, multipart_index_oid, true, upload_id);
    if (multipart_index_oid.u_lo == 0ULL && multipart_index_oid.u_hi == 0ULL) {
      // There is no multipart in progress, move on
      next();
    } else {
      object_multipart_metadata->load(
          std::bind(&S3PostMultipartObjectAction::next, this),
          std::bind(&S3PostMultipartObjectAction::next, this));
    }
  } else {
    s3_log(S3_LOG_WARN, "Missing bucket [%s]\n",
           request->get_bucket_name().c_str());
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PostMultipartObjectAction::create_object() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (object_multipart_metadata->get_state() !=
      S3ObjectMetadataState::present) {
    create_object_timer.start();
    if (tried_count == 0) {
      clovis_writer = std::make_shared<S3ClovisWriter>(request, oid);
    } else {
      clovis_writer->set_oid(oid);
    }
    clovis_writer->create_object(
        std::bind(&S3PostMultipartObjectAction::next, this),
        std::bind(&S3PostMultipartObjectAction::create_object_failed, this));
  } else {
    //++
    // Bailing out in case if the multipart upload is already in progress,
    // this will ensure that object doesn't go to inconsistent state

    // Once multipart is taken care in mero, it may not be needed
    // Note - Currently as per aws doc.
    // (http://docs.aws.amazon.com/AmazonS3/latest/API/mpUploadListMPUpload.html)
    // multipart upload details
    // of the same key initiated at different times is allowed.
    //--
    send_response_to_s3_client();
  }

  // for shutdown testcases, check FI and set shutdown signal
  S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL(
      "post_multipartobject_action_create_object_shutdown_fail");
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PostMultipartObjectAction::create_object_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "Exiting\n");
    return;
  }
  create_object_timer.stop();
  LOG_PERF("create_object_failed_ms",
           create_object_timer.elapsed_time_in_millisec());
  s3_stats_timing("multipart_create_object_failed",
                  create_object_timer.elapsed_time_in_millisec());

  if (clovis_writer->get_state() == S3ClovisWriterOpState::exists) {
    struct m0_uint128 object_list_indx_oid =
        bucket_metadata->get_object_list_index_oid();
    // If object exists, it may be due to the actual existance of object or due
    // to oid collision.
    if (tried_count || (object_list_indx_oid.u_hi == 0ULL &&
                        object_list_indx_oid.u_lo == 0ULL)) {
      // No need of lookup of metadata in case if it was oid collision before or
      // There is no object list index(no object metadata).
      collision_occured();
    } else {
      object_metadata =
          std::make_shared<S3ObjectMetadata>(request, object_list_indx_oid);
      // Lookup object metadata, if the object doesn't exist then its collision,
      // do collision resolution
      // If object exist in metadata then we overwrite it
      object_metadata->load(
          std::bind(&S3PostMultipartObjectAction::next, this),
          std::bind(&S3PostMultipartObjectAction::collision_occured, this));
    }
  } else {
    s3_log(S3_LOG_WARN, "Create object failed.\n");
    // Any other error report failure.
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PostMultipartObjectAction::collision_occured() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "Exiting\n");
    return;
  }
  if (tried_count < MAX_COLLISION_RETRY) {
    s3_log(S3_LOG_INFO, "Object ID collision happened for uri %s\n",
           request->get_object_uri().c_str());
    // Handle Collision
    create_new_oid();
    tried_count++;
    if (tried_count > 5) {
      s3_log(S3_LOG_INFO, "Object ID collision happened %d times for uri %s\n",
             tried_count, request->get_object_uri().c_str());
    }
    create_object();
  } else {
    if (tried_count > MAX_COLLISION_RETRY) {
      s3_log(S3_LOG_ERROR,
             "Failed to resolve object id collision %d times for uri %s\n",
             tried_count, request->get_object_uri().c_str());
      s3_iem(LOG_ERR, S3_IEM_COLLISION_RES_FAIL, S3_IEM_COLLISION_RES_FAIL_STR,
             S3_IEM_COLLISION_RES_FAIL_JSON);
    }
    send_response_to_s3_client();
  }
}

void S3PostMultipartObjectAction::create_new_oid() {
  std::string salted_uri =
      request->get_object_uri() + salt + std::to_string(tried_count);
  S3UriToMeroOID(salted_uri.c_str(), &oid);
  return;
}

void S3PostMultipartObjectAction::rollback_create() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  clovis_writer->delete_object(
      std::bind(&S3PostMultipartObjectAction::rollback_next, this),
      std::bind(&S3PostMultipartObjectAction::rollback_create_failed, this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PostMultipartObjectAction::rollback_create_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (clovis_writer->get_state() == S3ClovisWriterOpState::missing) {
    rollback_next();
  } else {
    // Log rollback failure.
    s3_log(S3_LOG_WARN, "Deletion of object failed\n");
    rollback_done();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PostMultipartObjectAction::rollback_create_part_meta_index() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  part_metadata->remove_index(
      std::bind(&S3PostMultipartObjectAction::rollback_next, this),
      std::bind(
          &S3PostMultipartObjectAction::rollback_create_part_meta_index_failed,
          this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PostMultipartObjectAction::rollback_create_part_meta_index_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  s3_log(S3_LOG_WARN, "Deletion of part metadata failed\n");
  rollback_done();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PostMultipartObjectAction::save_upload_metadata() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  create_object_timer.stop();
  LOG_PERF("create_object_successful_ms",
           create_object_timer.elapsed_time_in_millisec());
  s3_stats_timing("multipart_create_object_success",
                  create_object_timer.elapsed_time_in_millisec());

  // mark rollback point
  add_task_rollback(std::bind(
      &S3PostMultipartObjectAction::rollback_create_part_meta_index, this));

  for (auto it : request->get_in_headers_copy()) {
    if (it.first.find("x-amz-meta-") != std::string::npos) {
      object_multipart_metadata->add_user_defined_attribute(it.first,
                                                            it.second);
    }
  }
  object_multipart_metadata->set_oid(oid);
  object_multipart_metadata->set_part_index_oid(
      part_metadata->get_part_index_oid());
  object_multipart_metadata->save(
      std::bind(&S3PostMultipartObjectAction::next, this),
      std::bind(&S3PostMultipartObjectAction::save_upload_metadata_failed,
                this));

  // for shutdown testcases, check FI and set shutdown signal
  S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL(
      "post_multipartobject_action_save_upload_metadata_shutdown_fail");
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PostMultipartObjectAction::save_upload_metadata_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  // Trigger rollback to undo changes done and report error
  rollback_start();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PostMultipartObjectAction::rollback_upload_metadata() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  object_multipart_metadata->remove(
      std::bind(&S3PostMultipartObjectAction::rollback_next, this),
      std::bind(&S3PostMultipartObjectAction::rollback_upload_metadata_failed,
                this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PostMultipartObjectAction::rollback_upload_metadata_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (object_multipart_metadata->get_state() ==
      S3ObjectMetadataState::missing) {
    rollback_next();
  } else {
    // Log rollback failure.
    s3_log(S3_LOG_WARN, "Deletion of multipart metadata failed\n");
    rollback_done();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PostMultipartObjectAction::create_part_meta_index() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  // mark rollback point
  add_task_rollback(
      std::bind(&S3PostMultipartObjectAction::rollback_create, this));

  part_metadata = std::make_shared<S3PartMetadata>(request, upload_id, 0);
  part_metadata->create_index(
      std::bind(&S3PostMultipartObjectAction::next, this),
      std::bind(&S3PostMultipartObjectAction::rollback_start, this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PostMultipartObjectAction::save_multipart_metadata() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  // bypass shutdown signal check for next task
  check_shutdown_signal_for_next_task(false);
  if (multipart_index_oid.u_lo == 0 && multipart_index_oid.u_hi == 0) {
    // multipart index created, set it
    bucket_metadata->set_multipart_index_oid(
        object_multipart_metadata->get_index_oid());
    // mark rollback point
    add_task_rollback(std::bind(
        &S3PostMultipartObjectAction::rollback_upload_metadata, this));
    bucket_metadata->save(
        std::bind(&S3PostMultipartObjectAction::next, this),
        std::bind(&S3PostMultipartObjectAction::save_multipart_metadata_failed,
                  this));
  } else {
    next();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PostMultipartObjectAction::save_multipart_metadata_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  s3_log(S3_LOG_ERROR, "Failed to save multipart index oid metadata\n");
  // Trigger rollback to undo changes done and report error
  rollback_start();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3PostMultipartObjectAction::send_response_to_s3_client() {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  if (reject_if_shutting_down()) {
    // Send response with 'Service Unavailable' code.
    s3_log(S3_LOG_DEBUG, "sending 'Service Unavailable' response...\n");
    S3Error error("ServiceUnavailable", request->get_request_id(),
                  request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));
    request->set_out_header_value("Retry-After", "1");

    request->send_response(error.get_http_status_code(), response_xml);
  } else if (bucket_metadata->get_state() == S3BucketMetadataState::missing) {
    // Invalid Bucket Name
    S3Error error("NoSuchBucket", request->get_request_id(),
                  request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");

    request->send_response(error.get_http_status_code(), response_xml);
  } else if (bucket_metadata->get_state() == S3BucketMetadataState::failed) {
    S3Error error("InternalError", request->get_request_id(),
                  request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));
    request->send_response(error.get_http_status_code(), response_xml);
  } else if (object_multipart_metadata->get_state() ==
             S3ObjectMetadataState::present) {
    S3Error error("InvalidObjectState", request->get_request_id(),
                  request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));
    request->send_response(error.get_http_status_code(), response_xml);
  } else if ((object_multipart_metadata->get_state() ==
              S3ObjectMetadataState::saved) &&
             ((part_metadata->get_state() ==
               S3PartMetadataState::store_created) ||
              (part_metadata->get_state() == S3PartMetadataState::present))) {
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
    S3Error error("InternalError", request->get_request_id(),
                  request->get_object_uri());
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
