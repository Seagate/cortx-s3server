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
#include "s3_log.h"

S3HeadObjectAction::S3HeadObjectAction(std::shared_ptr<S3RequestObject> req)
    : S3Action(req) {
  s3_log(S3_LOG_DEBUG, "Constructor\n");
  object_list_index_oid = {0ULL, 0ULL};
  setup_steps();
}

void S3HeadObjectAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, "Setting up the action\n");
  add_task(std::bind(&S3HeadObjectAction::fetch_bucket_info, this));
  add_task(std::bind(&S3HeadObjectAction::fetch_object_info, this));
  add_task(std::bind(&S3HeadObjectAction::send_response_to_s3_client, this));
  // ...
}

void S3HeadObjectAction::fetch_bucket_info() {
  s3_log(S3_LOG_DEBUG, "Fetching bucket metadata\n");
  bucket_metadata = std::make_shared<S3BucketMetadata>(request);
  bucket_metadata->load(std::bind(&S3HeadObjectAction::next, this),
                        std::bind(&S3HeadObjectAction::next, this));
}

void S3HeadObjectAction::fetch_object_info() {
  if (bucket_metadata->get_state() == S3BucketMetadataState::present) {
    s3_log(S3_LOG_DEBUG, "Found bucket metadata\n");
    object_list_index_oid = bucket_metadata->get_object_list_index_oid();
    // bypass shutdown signal check for next task
    check_shutdown_signal_for_next_task(false);

    if (object_list_index_oid.u_lo == 0ULL &&
        object_list_index_oid.u_hi == 0ULL) {
      // There is no object list index, hence object doesn't exist
      s3_log(S3_LOG_DEBUG, "Object not found\n");
      send_response_to_s3_client();
    } else {
      object_metadata =
          std::make_shared<S3ObjectMetadata>(request, object_list_index_oid);

      object_metadata->load(std::bind(&S3HeadObjectAction::next, this),
                            std::bind(&S3HeadObjectAction::next, this));
    }
  } else {
    s3_log(S3_LOG_WARN, "Bucket not found\n");
    send_response_to_s3_client();
  }
}

void S3HeadObjectAction::send_response_to_s3_client() {
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
                  request->get_bucket_name());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));

    request->send_response(error.get_http_status_code(), response_xml);
  } else if (object_list_index_oid.u_lo == 0ULL &&
             object_list_index_oid.u_hi == 0ULL) {
    S3Error error("NoSuchKey", request->get_request_id(),
                  request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));

    request->send_response(error.get_http_status_code(), response_xml);
  } else if (object_metadata->get_state() == S3ObjectMetadataState::missing) {
    S3Error error("NoSuchKey", request->get_request_id(),
                  request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));

    request->send_response(error.get_http_status_code(), response_xml);
  } else if (object_metadata->get_state() == S3ObjectMetadataState::present) {
    request->set_out_header_value("Last-Modified",
                                  object_metadata->get_last_modified_gmt());
    request->set_out_header_value("ETag", object_metadata->get_md5());
    request->set_out_header_value("Accept-Ranges", "bytes");
    request->set_out_header_value("Content-Length",
                                  object_metadata->get_content_length_str());

    for (auto it : object_metadata->get_user_attributes()) {
      request->set_out_header_value(it.first, it.second);
    }

    request->send_response(S3HttpSuccess200);
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
  i_am_done();  // self delete
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}
