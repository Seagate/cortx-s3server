/*
 * Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
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

#include "s3_put_bucket_versioning_action.h"
#include "s3_error_codes.h"
#include "s3_log.h"

S3PutBucketVersioningAction::S3PutBucketVersioningAction(
    std::shared_ptr<S3RequestObject> req,
    std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory,
    std::shared_ptr<S3PutBucketVersioningBodyFactory> bucket_body_factory)
    : S3BucketAction(std::move(req), std::move(bucket_meta_factory)) {
  s3_log(S3_LOG_DEBUG, request_id, "%s Ctor\n", __func__);

  s3_log(S3_LOG_INFO, stripped_request_id,
         "S3 API: Put Bucket Tagging. Bucket[%s]\n",
         request->get_bucket_name().c_str());

  if (bucket_body_factory) {
    put_bucket_version_factory = bucket_body_factory;
  } else {
    put_bucket_version_factory =
        std::make_shared<S3PutBucketVersioningBodyFactory>();
  }
  setup_steps();
}

void S3PutBucketVersioningAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setting up the action\n");
  ACTION_TASK_ADD(S3PutBucketVersioningAction::validate_request, this);
  ACTION_TASK_ADD(S3PutBucketVersioningAction::validate_request_xml_tags, this);
  ACTION_TASK_ADD(
      S3PutBucketVersioningAction::save_versioning_status_to_bucket_metadata,
      this);
  ACTION_TASK_ADD(S3PutBucketVersioningAction::send_response_to_s3_client,
                  this);
  // ...
}

void S3PutBucketVersioningAction::validate_request() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  if (request->has_all_body_content()) {
    bucket_content_string = request->get_full_body_content_as_string();
    validate_request_body(bucket_content_string);
  } else {
    // Start streaming, logically pausing action till we get data.
    request->listen_for_incoming_data(
        std::bind(&S3PutBucketVersioningAction::consume_incoming_content, this),
        request->get_data_length() /* we ask for all */
        );
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutBucketVersioningAction::consume_incoming_content() {
  s3_log(S3_LOG_INFO, stripped_request_id, "Consume data\n");
  if (request->is_s3_client_read_error()) {
    client_read_error();
  } else if (request->has_all_body_content()) {
    bucket_content_string = request->get_full_body_content_as_string();
    validate_request_body(bucket_content_string);
  } else {
    // else just wait till entire body arrives. rare.
    request->resume();
  }
}

void S3PutBucketVersioningAction::validate_request_body(std::string content) {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  put_bucket_version_body =
      put_bucket_version_factory->create_put_resource_versioning_body(
          content, request_id);
  if (put_bucket_version_body->isOK()) {
    bucket_version_status = put_bucket_version_body->get_versioning_status();
    next();
  } else {
    set_s3_error("MalformedXML");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutBucketVersioningAction::validate_request_xml_tags() {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);

  if (put_bucket_version_body->validate_bucket_xml_versioning_status(
          bucket_version_status)) {
    next();
  } else {
    set_s3_error("IllegalVersioningConfigurationException");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

/*void S3PutBucketVersioningAction::fetch_bucket_info_failed() {
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
}*/

void S3PutBucketVersioningAction::save_versioning_status_to_bucket_metadata() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (bucket_metadata->get_state() == S3BucketMetadataState::present) {
    s3_log(S3_LOG_DEBUG, request_id, "Setting bucket versioning =%s\n",
           bucket_content_string.c_str());
    bucket_metadata->set_bucket_versioning(bucket_version_status);
    // bypass shutdown signal check for next task
    check_shutdown_signal_for_next_task(false);
    bucket_metadata->update(
        std::bind(&S3PutBucketVersioningAction::next, this),
        std::bind(&S3PutBucketVersioningAction::
                       save_versioning_status_to_bucket_metadata_failed,
                  this));
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutBucketVersioningAction::
    save_versioning_status_to_bucket_metadata_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (bucket_metadata->get_state() == S3BucketMetadataState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id,
           "Save Bucket metadata operation failed due to prelaunch failure\n");
    set_s3_error("ServiceUnavailable");
  } else {
    s3_log(S3_LOG_ERROR, request_id, "Save Bucket metadata operation failed\n");
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PutBucketVersioningAction::send_response_to_s3_client() {
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
    request->send_response(S3HttpSuccess200);
  }

  S3_RESET_SHUTDOWN_SIGNAL;  // for shutdown testcases
  done();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}
