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

#include "motr_head_index_action.h"
#include "s3_error_codes.h"
#include "s3_m0_uint128_helper.h"
#include "s3_motr_wrapper.h"

MotrHeadIndexAction::MotrHeadIndexAction(
    std::shared_ptr<MotrRequestObject> req,
    std::shared_ptr<S3MotrKVSReaderFactory> motr_kvs_reader_factory)
    : MotrAction(std::move(req)) {
  s3_log(S3_LOG_DEBUG, request_id, "Constructor");
  motr_api = std::make_shared<ConcreteMotrAPI>();

  if (motr_kvs_reader_factory) {
    motr_kvs_reader_factory_ptr = std::move(motr_kvs_reader_factory);
  } else {
    motr_kvs_reader_factory_ptr = std::make_shared<S3MotrKVSReaderFactory>();
  }

  setup_steps();
}

void MotrHeadIndexAction::setup_steps() {
  ACTION_TASK_ADD(MotrHeadIndexAction::validate_request, this);
  ACTION_TASK_ADD(MotrHeadIndexAction::check_index_exist, this);
  ACTION_TASK_ADD(MotrHeadIndexAction::send_response_to_s3_client, this);
}

void MotrHeadIndexAction::validate_request() {
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

void MotrHeadIndexAction::check_index_exist() {
  motr_kv_reader =
      motr_kvs_reader_factory_ptr->create_motr_kvs_reader(request, motr_api);
  motr_kv_reader->lookup_index(
      index_id,
      std::bind(&MotrHeadIndexAction::check_index_exist_success, this),
      std::bind(&MotrHeadIndexAction::check_index_exist_failure, this));
}

void MotrHeadIndexAction::check_index_exist_success() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  next();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void MotrHeadIndexAction::check_index_exist_failure() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (motr_kv_reader->get_state() == S3MotrKVSReaderOpState::missing) {
    s3_log(S3_LOG_DEBUG, request_id, "Index not found\n");
    set_s3_error("NoSuchIndex");
  } else if (motr_kv_reader->get_state() ==
             S3MotrKVSReaderOpState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id, "Failed to launch index lookup.\n");
    set_s3_error("ServiceUnavailable");
  } else {
    s3_log(S3_LOG_DEBUG, request_id, "Failed to lookup index.\n");
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
}

void MotrHeadIndexAction::send_response_to_s3_client() {
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
    // Index found
    request->send_response(S3HttpSuccess200);
  }
  done();
}
