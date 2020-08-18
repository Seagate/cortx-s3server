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

#include "motr_head_object_action.h"
#include "s3_error_codes.h"
#include "s3_common_utilities.h"
#include "s3_m0_uint128_helper.h"

MotrHeadObjectAction::MotrHeadObjectAction(
    std::shared_ptr<MotrRequestObject> req,
    std::shared_ptr<S3MotrReaderFactory> reader_factory)
    : MotrAction(std::move(req)), layout_id(0) {
  s3_log(S3_LOG_DEBUG, request_id, "Constructor");
  oid = {0ULL, 0ULL};
  if (reader_factory) {
    motr_reader_factory = std::move(reader_factory);
  } else {
    motr_reader_factory = std::make_shared<S3MotrReaderFactory>();
  }

  setup_steps();
}

void MotrHeadObjectAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setting up the action\n");
  ACTION_TASK_ADD(MotrHeadObjectAction::validate_request, this);
  ACTION_TASK_ADD(MotrHeadObjectAction::check_object_exist, this);
  ACTION_TASK_ADD(MotrHeadObjectAction::send_response_to_s3_client, this);
  // ...
}

void MotrHeadObjectAction::validate_request() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  oid = S3M0Uint128Helper::to_m0_uint128(request->get_object_oid_lo(),
                                         request->get_object_oid_hi());
  // invalid oid
  if (!oid.u_hi && !oid.u_lo) {
    s3_log(S3_LOG_ERROR, request_id, "Invalid object oid\n");
    set_s3_error("BadRequest");
    send_response_to_s3_client();
  } else {
    std::string object_layout_id = request->get_query_string_value("layout-id");
    if (!S3CommonUtilities::stoi(object_layout_id, layout_id) ||
        (layout_id <= 0)) {
      s3_log(S3_LOG_ERROR, request_id, "Invalid object layout-id: %s\n",
             object_layout_id.c_str());
      set_s3_error("BadRequest");
      send_response_to_s3_client();
    } else {
      next();
    }
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void MotrHeadObjectAction::check_object_exist() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  motr_reader =
      motr_reader_factory->create_motr_reader(request, oid, layout_id);

  motr_reader->check_object_exist(
      std::bind(&MotrHeadObjectAction::check_object_exist_success, this),
      std::bind(&MotrHeadObjectAction::check_object_exist_failure, this));

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void MotrHeadObjectAction::check_object_exist_success() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  next();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void MotrHeadObjectAction::check_object_exist_failure() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (motr_reader->get_state() == S3MotrReaderOpState::missing) {
    s3_log(S3_LOG_DEBUG, request_id, "Object not found\n");
    set_s3_error("NoSuchKey");
  } else if (motr_reader->get_state() ==
             S3MotrReaderOpState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id, "Failed to lookup object.\n");
    set_s3_error("ServiceUnavailable");
  } else {
    s3_log(S3_LOG_DEBUG, request_id, "Failed to lookup object\n");
    set_s3_error("InternalError");
  }
  next();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void MotrHeadObjectAction::send_response_to_s3_client() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (is_error_state() && !get_s3_error_code().empty()) {
    S3Error error(get_s3_error_code(), request->get_request_id(),
                  request->c_get_full_path());
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
  } else {
    // Object found
    request->send_response(S3HttpSuccess200);
  }
  done();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}
