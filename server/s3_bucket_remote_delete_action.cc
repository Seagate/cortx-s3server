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

#include "s3_bucket_remote_delete_action.h"
#include "s3_error_codes.h"
#include "s3_log.h"

S3BucketRemoteDeleteAction::S3BucketRemoteDeleteAction(
    std::shared_ptr<S3RequestObject> req,
    std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory)
    : S3BucketAction(std::move(req), std::move(bucket_meta_factory)) {
  s3_log(S3_LOG_DEBUG, request_id, "%s Ctor\n", __func__);

  s3_log(S3_LOG_INFO, stripped_request_id,
         "S3 API: Bucket Remote Delete. Bucket[%s]\n",
         request->get_bucket_name().c_str());

  setup_steps();
}

// Action task list
void S3BucketRemoteDeleteAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setting up the action\n");
  ACTION_TASK_ADD(S3BucketRemoteDeleteAction::delete_remote_bucket_creds, this);
  ACTION_TASK_ADD(S3BucketRemoteDeleteAction::send_response_to_s3_client, this);
  // ...
}

// Check if bucket metadata failed to launch
void S3BucketRemoteDeleteAction::fetch_bucket_info_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (bucket_metadata->get_state() == S3BucketMetadataState::failed_to_launch) {
    set_s3_error("ServiceUnavailable");
  } else if (bucket_metadata->get_state() == S3BucketMetadataState::missing) {
    set_s3_error("NoSuchBucket");
  } else {
    set_s3_error("InternalError");
  }
  s3_log(S3_LOG_INFO, "", "%s Exit", __func__);
  send_response_to_s3_client();
}

// Delete remote bucket credentials from metadata
void S3BucketRemoteDeleteAction::delete_remote_bucket_creds() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  std::string alias_name = request->get_query_string_value("aliasName");
  s3_log(S3_LOG_INFO, stripped_request_id, "alias_name to delete - %s \n",
         alias_name.c_str());

  bucket_metadata->delete_bucket_remote_creds_and_alias_name(alias_name);
  bucket_metadata->update(
      std::bind(&S3BucketRemoteDeleteAction::next, this),
      std::bind(&S3BucketRemoteDeleteAction::delete_remote_bucket_creds_failed,
                this));

  s3_log(S3_LOG_INFO, "", "%s Exit", __func__);
}

// Check if deleting remote bucket credentials failed due to failed to launch
// metadata
void S3BucketRemoteDeleteAction::delete_remote_bucket_creds_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (bucket_metadata->get_state() == S3BucketMetadataState::failed_to_launch) {
    set_s3_error("ServiceUnavailable");
  } else {
    set_s3_error("InternalError");
  }
  s3_log(S3_LOG_INFO, "", "%s Exit", __func__);
  next();
}

// Send appropriate response to client
void S3BucketRemoteDeleteAction::send_response_to_s3_client() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  if (reject_if_shutting_down() ||
      (is_error_state() && !get_s3_error_code().empty())) {
    S3Error error(get_s3_error_code(), request->get_request_id(),
                  request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));

    if (get_s3_error_code() == "ServiceUnavailable" ||
        get_s3_error_code() == "InternalError") {
      request->set_out_header_value("Connection", "close");
    }

    if (get_s3_error_code() == "ServiceUnavailable") {
      if (reject_if_shutting_down()) {
        int retry_after_period =
            S3Option::get_instance()->get_s3_retry_after_sec();
        request->set_out_header_value("Retry-After",
                                      std::to_string(retry_after_period));
      } else {
        request->set_out_header_value("Retry-After", "1");
      }
    }

    request->send_response(error.get_http_status_code(), response_xml);

  } else {
    request->send_response(S3HttpSuccess204);
  }

  done();
  s3_log(S3_LOG_INFO, "", "%s Exit", __func__);
}
