/*
 * Copyright (c) 2021 Seagate Technology LLC and/or its Affiliates
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For any questions about this software or licensing,
 * please email opensource@seagate.com or cortx-questions@seagate.com.
 *
 */

#include "s3_get_bucket_versioning_action.h"
#include "s3_error_codes.h"
#include "s3_log.h"

S3GetBucketVersioningAction::S3GetBucketVersioningAction(
    std::shared_ptr<S3RequestObject> req,
    std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory)
    : S3BucketAction(std::move(req), std::move(bucket_meta_factory)) {
  s3_log(S3_LOG_DEBUG, request_id, "%s Ctor\n", __func__);

  s3_log(S3_LOG_INFO, stripped_request_id,
         "S3 API: Get Bucket Versioning. Bucket[%s]\n",
         request->get_bucket_name().c_str());

  setup_steps();
}

void S3GetBucketVersioningAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setting up the action\n");
  ACTION_TASK_ADD(S3GetBucketVersioningAction::validate_request, this);
  ACTION_TASK_ADD(S3GetBucketVersioningAction::send_response_to_s3_client,
                  this);
  // ...
}

void S3GetBucketVersioningAction::validate_request() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  if (request->has_all_body_content()) {
    next();
  } else {
    // Start streaming, logically pausing action till we get data.
    request->listen_for_incoming_data(
        std::bind(&S3GetBucketVersioningAction::consume_incoming_content, this),
        request->get_data_length() /* we ask for all */
        );
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3GetBucketVersioningAction::consume_incoming_content() {
  s3_log(S3_LOG_INFO, stripped_request_id, "Consume data\n");
  if (request->is_s3_client_read_error()) {
    client_read_error();
  } else if (request->has_all_body_content()) {
    next();
  } else {
    // else just wait till entire body arrives. rare.
    request->resume();
  }
}

void S3GetBucketVersioningAction::fetch_bucket_info_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (bucket_metadata->get_state() == S3BucketMetadataState::missing) {
    set_s3_error("NoSuchBucket");
  } else if (bucket_metadata->get_state() ==
             S3BucketMetadataState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id,
           "Bucket metadata load operation failed due to pre launch failure\n");
    set_s3_error("ServiceUnavailable");
  } else {
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3GetBucketVersioningAction::send_response_to_s3_client() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

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
  } else {
    std::string response_xml =
        bucket_metadata->get_bucket_versioning_status_as_xml();
    request->set_bytes_sent(response_xml.length());
    request->send_response(S3HttpSuccess200, response_xml);
  }

  S3_RESET_SHUTDOWN_SIGNAL;  // for shutdown testcases
  done();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}