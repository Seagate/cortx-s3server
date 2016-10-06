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

S3AbortMultipartAction::S3AbortMultipartAction(
    std::shared_ptr<S3RequestObject> req)
    : S3Action(req, false) {
  s3_log(S3_LOG_DEBUG, "Constructor\n");
  upload_id = request->get_query_string_value("uploadId");
  object_name = request->get_object_name();
  bucket_name = request->get_bucket_name();
  multipart_oid = {0ULL, 0ULL};
  part_index_oid = {0ULL, 0ULL};
  s3_clovis_api = std::make_shared<ConcreteClovisAPI>();
  abort_success = false;
  setup_steps();
}

void S3AbortMultipartAction::setup_steps(){
  s3_log(S3_LOG_DEBUG, "Setup the action\n");
  add_task(std::bind( &S3AbortMultipartAction::fetch_bucket_info, this ));
  add_task(std::bind( &S3AbortMultipartAction::get_multipart_metadata, this ));
  add_task(std::bind( &S3AbortMultipartAction::delete_multipart_metadata, this));
  add_task(std::bind( &S3AbortMultipartAction::check_if_any_parts_present, this ));
  add_task(std::bind( &S3AbortMultipartAction::delete_object, this));
  add_task(std::bind( &S3AbortMultipartAction::delete_part_index_with_parts, this));
  add_task(std::bind( &S3AbortMultipartAction::send_response_to_s3_client, this ));
  // ...
}

void S3AbortMultipartAction::fetch_bucket_info() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  bucket_metadata = std::make_shared<S3BucketMetadata>(request);
  bucket_metadata->load(std::bind( &S3AbortMultipartAction::next, this), std::bind( &S3AbortMultipartAction::next, this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3AbortMultipartAction::get_multipart_metadata() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (bucket_metadata->get_state() == S3BucketMetadataState::present) {
    multipart_oid = bucket_metadata->get_multipart_index_oid();
    if (multipart_oid.u_lo == 0ULL && multipart_oid.u_hi == 0ULL) {
      // There is no multipart upload to abort
      send_response_to_s3_client();
    } else {
      object_multipart_metadata = std::make_shared<S3ObjectMetadata>(
          request, multipart_oid, true, upload_id);
      object_multipart_metadata->load(
          std::bind(&S3AbortMultipartAction::next, this),
          std::bind(&S3AbortMultipartAction::next, this));
    }
  } else {
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3AbortMultipartAction::delete_multipart_metadata() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (object_multipart_metadata->get_state() == S3ObjectMetadataState::present) {
    part_index_oid = object_multipart_metadata->get_part_index_oid();
    object_multipart_metadata->remove(std::bind( &S3AbortMultipartAction::next, this),
                                      std::bind( &S3AbortMultipartAction::next, this));
  } else {
    next();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3AbortMultipartAction::check_if_any_parts_present() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (object_multipart_metadata->get_state() ==
      S3ObjectMetadataState::deleted) {
    abort_success = true;
  }
  if (part_index_oid.u_lo == 0ULL && part_index_oid.u_hi == 0ULL) {
    next();
  } else {
    clovis_kv_reader =
        std::make_shared<S3ClovisKVSReader>(request, s3_clovis_api);
    clovis_kv_reader->next_keyval(
        part_index_oid, "", 1, std::bind(&S3AbortMultipartAction::next, this),
        std::bind(&S3AbortMultipartAction::check_if_any_parts_present_failed,
                  this));
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3AbortMultipartAction::check_if_any_parts_present_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (clovis_kv_reader->get_state() == S3ClovisKVSReaderOpState::missing) {
    s3_log(S3_LOG_INFO, "No parts uploaded in upload-id: %s\n", upload_id.c_str());
    next();
  } else {
    abort_success = false;
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3AbortMultipartAction::delete_object() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if ((object_multipart_metadata->get_state() ==
       S3ObjectMetadataState::present) ||
      (object_multipart_metadata->get_state() ==
       S3ObjectMetadataState::deleted)) {
    clovis_writer = std::make_shared<S3ClovisWriter>(request, object_multipart_metadata->get_oid());
    clovis_writer->delete_object(std::bind( &S3AbortMultipartAction::next, this), std::bind( &S3AbortMultipartAction::delete_object_failed, this));
  } else {
    next();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3AbortMultipartAction::delete_object_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  if (clovis_writer->get_state() == S3ClovisWriterOpState::failed) {
    // todo: how do we have clean up of oid for such cases? background utility?
    s3_log(S3_LOG_ERROR, "Failed to delete object, this will be stale in Mero: u_hi(base64) = [%s] and u_lo(base64) = [%s]\n",
                          object_multipart_metadata->get_oid_u_hi_str().c_str(),
                          object_multipart_metadata->get_oid_u_lo_str().c_str());
    abort_success = false;
  }
  next();

  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3AbortMultipartAction::delete_part_index_with_parts() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (part_index_oid.u_lo == 0ULL && part_index_oid.u_hi == 0ULL) {
    next();
  } else {
    part_metadata =
        std::make_shared<S3PartMetadata>(request, part_index_oid, upload_id, 1);
    part_metadata->remove_index(std::bind(&S3AbortMultipartAction::next, this),
                                std::bind(&S3AbortMultipartAction::next, this));
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3AbortMultipartAction::send_response_to_s3_client() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (bucket_metadata && (bucket_metadata->get_state() == S3BucketMetadataState::missing)) {
    // Invalid Bucket Name
    S3Error error("NoSuchBucket", request->get_request_id(), request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->send_response(error.get_http_status_code(), response_xml);
  } else if (object_multipart_metadata &&
             (object_multipart_metadata->get_state() ==
              S3ObjectMetadataState::missing) &&
             (clovis_kv_reader && (clovis_kv_reader->get_state() ==
                                   S3ClovisKVSReaderOpState::missing))) {
    S3Error error("NoSuchUpload", request->get_request_id(), request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->send_response(error.get_http_status_code(), response_xml);
  } else if (multipart_oid.u_lo == 0ULL && multipart_oid.u_hi == 0ULL) {
    S3Error error("NoSuchUpload", request->get_request_id(),
                  request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->send_response(error.get_http_status_code(), response_xml);
  } else if (abort_success ||
             (part_metadata &&
              (part_metadata->get_state() == S3PartMetadataState::deleted))) {
    request->send_response(S3HttpSuccess200);
  } else {
    s3_log(S3_LOG_DEBUG, "InternalError: Requesting client to retry after 1 second\n");
    S3Error error("InternalError", request->get_request_id(), request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length", std::to_string(response_xml.length()));
    // Suggest the client retry after 1 second delay
    request->set_out_header_value("Retry-After", "1");
    request->send_response(error.get_http_status_code(), response_xml);
  }
  done();
  i_am_done();  // self delete
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}
