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

#include "s3_error_codes.h"
#include "motr_delete_index_action.h"
#include "s3_common_utilities.h"
#include "s3_iem.h"
#include "s3_log.h"
#include "s3_m0_uint128_helper.h"

MotrDeleteIndexAction::MotrDeleteIndexAction(
    std::shared_ptr<MotrRequestObject> req,
    std::shared_ptr<S3MotrKVSWriterFactory> motr_kvs_writer_factory)
    : MotrAction(req) {
  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");
  motr_api = std::make_shared<ConcreteMotrAPI>();

  s3_log(S3_LOG_INFO, request_id, "Motr API: Index delete.\n");

  if (motr_kvs_writer_factory) {
    motr_kvs_writer_factory_ptr = motr_kvs_writer_factory;
  } else {
    motr_kvs_writer_factory_ptr = std::make_shared<S3MotrKVSWriterFactory>();
  }

  setup_steps();
}

void MotrDeleteIndexAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setting up the action\n");
  ACTION_TASK_ADD(MotrDeleteIndexAction::validate_request, this);
  ACTION_TASK_ADD(MotrDeleteIndexAction::delete_index, this);
  ACTION_TASK_ADD(MotrDeleteIndexAction::send_response_to_s3_client, this);
  // ...
}

void MotrDeleteIndexAction::validate_request() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  index_id = S3M0Uint128Helper::to_m0_uint128(request->get_index_id_lo(),
                                              request->get_index_id_hi());
  // invalid oid check
  if (index_id.u_hi == 0ULL && index_id.u_lo == 0ULL) {
    set_s3_error("BadRequest");
    send_response_to_s3_client();
    return;
  }

  next();
}

void MotrDeleteIndexAction::delete_index() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  motr_kv_writer =
      motr_kvs_writer_factory_ptr->create_motr_kvs_writer(request, motr_api);

  motr_kv_writer->delete_index(
      index_id,
      std::bind(&MotrDeleteIndexAction::delete_index_successful, this),
      std::bind(&MotrDeleteIndexAction::delete_index_failed, this));

  // for shutdown testcases, check FI and set shutdown signal
  S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL("delete_index_action_shutdown_fail");
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void MotrDeleteIndexAction::delete_index_successful() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  next();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void MotrDeleteIndexAction::delete_index_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (motr_kv_writer->get_state() == S3MotrKVSWriterOpState::missing) {
    s3_log(S3_LOG_DEBUG, request_id, "Index is missing.\n");
  } else if (motr_kv_writer->get_state() ==
             S3MotrKVSWriterOpState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id, "Failed to launch.\n");
    set_s3_error("ServiceUnavailable");
  } else {
    s3_log(S3_LOG_DEBUG, request_id, "Failed to delete index.\n");
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void MotrDeleteIndexAction::send_response_to_s3_client() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  if (reject_if_shutting_down() ||
      (is_error_state() && !get_s3_error_code().empty())) {
    s3_log(S3_LOG_DEBUG, request_id, "Sending %s response...\n",
           get_s3_error_code().c_str());
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
  S3_RESET_SHUTDOWN_SIGNAL;  // for shutdown testcases
  done();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}
