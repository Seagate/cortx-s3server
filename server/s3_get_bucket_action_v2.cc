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

#include <string>
#include <memory>

#include "s3_error_codes.h"
#include "s3_get_bucket_action_v2.h"
#include "s3_object_list_v2_response.h"

// TODO:
// This is partial implementation of List Objects V2 API.
// Need a strategy to obfuscate ContinuationToken/NextContinuationToken.
//   Presently, ContinuationToken and NextContinuationToken are similar
//   to marker/next-marker in V1.
S3GetBucketActionV2::S3GetBucketActionV2(
    std::shared_ptr<S3RequestObject> req, std::shared_ptr<MotrAPI> motr_api,
    std::shared_ptr<S3MotrKVSReaderFactory> motr_kvs_reader_factory,
    std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory,
    std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory)
    : S3GetBucketAction(req, motr_api, motr_kvs_reader_factory,
                        bucket_meta_factory, object_meta_factory),
      request_fetch_owner(true),
      request_cont_token(""),
      request_start_after("") {
  set_object_list_response(std::make_shared<S3ObjectListResponseV2>(
      req->get_query_string_value("encoding-type")));
  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");
  s3_log(S3_LOG_INFO, request_id, "S3 API: Get Bucket(List Objects V2).\n");
}

S3GetBucketActionV2::~S3GetBucketActionV2() {}

void S3GetBucketActionV2::validate_request() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  s3_log(S3_LOG_INFO, request_id, "Validate ListObjects V2 request\n");

  request_cont_token = request->get_query_string_value("continuation-token");
  s3_log(S3_LOG_DEBUG, request_id, "continuation-token = %s\n",
         request_cont_token.c_str());
  request_fetch_owner =
      (request->get_query_string_value("fetch-owner") == "true") ? true : false;
  s3_log(S3_LOG_DEBUG, request_id, "fetch-owner = %d\n", request_fetch_owner);
  request_start_after = request->get_query_string_value("start-after");
  s3_log(S3_LOG_DEBUG, request_id, "start-after = %s\n",
         request_start_after.c_str());

  std::shared_ptr<S3ObjectListResponseV2> obj_v2_list =
      std::dynamic_pointer_cast<S3ObjectListResponseV2>(object_list);
  if (obj_v2_list) {
    obj_v2_list->set_continuation_token(request_cont_token);
    obj_v2_list->set_fetch_owner(request_fetch_owner);
    obj_v2_list->set_start_after(request_start_after);
  }
  // Call base class request validator
  S3GetBucketAction::validate_request();
}

void S3GetBucketActionV2::after_validate_request() {
  std::shared_ptr<S3ObjectListResponseV2> obj_v2_list =
      std::dynamic_pointer_cast<S3ObjectListResponseV2>(object_list);

  if (request_cont_token.empty()) {
    last_key = request_start_after;
  } else {
    last_key = request_cont_token;
  }
  request_marker_key = last_key;
  next();
}

void S3GetBucketActionV2::send_response_to_s3_client() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  if (reject_if_shutting_down() ||
      (is_error_state() && !get_s3_error_code().empty())) {
    s3_log(S3_LOG_DEBUG, request_id, "Sending %s response...\n",
           get_s3_error_code().c_str());
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
      request->set_out_header_value("Retry-After", "1");
    }
    request->send_response(error.get_http_status_code(), response_xml);
  } else if (fetch_successful) {
    std::string& response_xml = object_list->get_xml(
        request->get_canonical_id(), bucket_metadata->get_owner_id(),
        request->get_user_id());
    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));
    request->set_out_header_value("Content-Type", "application/xml");
    s3_log(S3_LOG_DEBUG, request_id, "Object list response_xml = %s\n",
           response_xml.c_str());

    request->send_response(S3HttpSuccess200, response_xml);
  } else {
    S3Error error("InternalError", request->get_request_id(),
                  request->get_bucket_name());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));

    request->send_response(error.get_http_status_code(), response_xml);
  }
  S3_RESET_SHUTDOWN_SIGNAL;  // for shutdown testcases
  done();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}
