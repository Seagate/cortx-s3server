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

#include "motr_delete_object_action.h"
#include "s3_error_codes.h"
#include "s3_common_utilities.h"
#include "s3_m0_uint128_helper.h"

MotrDeleteObjectAction::MotrDeleteObjectAction(
    std::shared_ptr<MotrRequestObject> req,
    std::shared_ptr<S3MotrWriterFactory> writer_factory)
    : MotrAction(std::move(req)) {
  s3_log(S3_LOG_DEBUG, request_id, "Constructor");

  if (writer_factory) {
    motr_writer_factory = std::move(writer_factory);
  } else {
    motr_writer_factory = std::make_shared<S3MotrWriterFactory>();
  }

  setup_steps();
}

void MotrDeleteObjectAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setting up the action\n");
  ACTION_TASK_ADD(MotrDeleteObjectAction::validate_request, this);
  ACTION_TASK_ADD(MotrDeleteObjectAction::delete_object, this);
  ACTION_TASK_ADD(MotrDeleteObjectAction::send_response_to_s3_client, this);
  // ...
}

void MotrDeleteObjectAction::validate_request() {
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
    if (!S3CommonUtilities::stoi(object_layout_id, layout_id)) {
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

void MotrDeleteObjectAction::delete_object() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  motr_writer = motr_writer_factory->create_motr_writer(request, oid);
  motr_writer->delete_object(
      std::bind(&MotrDeleteObjectAction::delete_object_successful, this),
      std::bind(&MotrDeleteObjectAction::delete_object_failed, this),
      layout_id);

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void MotrDeleteObjectAction::delete_object_successful() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  next();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void MotrDeleteObjectAction::delete_object_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  // Object missing is treated as object deleted similar to S3 object delete.
  if (motr_writer->get_state() == S3MotrWiterOpState::missing) {
    s3_log(S3_LOG_DEBUG, request_id,
           "Object with oid %" SCNx64 " : %" SCNx64 " is missing\n", oid.u_hi,
           oid.u_lo);
  } else {
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void MotrDeleteObjectAction::send_response_to_s3_client() {
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
    request->send_response(S3HttpSuccess204);
  }
  done();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}
