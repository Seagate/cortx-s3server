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


#include "mero_delete_key_value_action.h"
#include "s3_error_codes.h"
#include "s3_m0_uint128_helper.h"

MeroDeleteKeyValueAction::MeroDeleteKeyValueAction(
    std::shared_ptr<MeroRequestObject> req,
    std::shared_ptr<ClovisAPI> clovis_api,
    std::shared_ptr<S3ClovisKVSWriterFactory> clovis_mero_kvs_writer_factory,
    std::shared_ptr<S3ClovisKVSReaderFactory> clovis_mero_kvs_reader_factory)
    : MeroAction(req) {
  s3_log(S3_LOG_DEBUG, request_id, "Constructor");
  if (clovis_api) {
    mero_clovis_api = clovis_api;
  } else {
    mero_clovis_api = std::make_shared<ConcreteClovisAPI>();
  }

  if (clovis_mero_kvs_reader_factory) {
    clovis_kvs_reader_factory = clovis_mero_kvs_reader_factory;
  } else {
    clovis_kvs_reader_factory = std::make_shared<S3ClovisKVSReaderFactory>();
  }

  if (clovis_mero_kvs_writer_factory) {
    clovis_kvs_writer_factory = clovis_mero_kvs_writer_factory;
  } else {
    clovis_kvs_writer_factory = std::make_shared<S3ClovisKVSWriterFactory>();
  }

  setup_steps();
}

void MeroDeleteKeyValueAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setting up the action\n");
  ACTION_TASK_ADD(MeroDeleteKeyValueAction::delete_key_value, this);
  ACTION_TASK_ADD(MeroDeleteKeyValueAction::send_response_to_s3_client, this);
  // ...
}

void MeroDeleteKeyValueAction::delete_key_value() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  index_id = S3M0Uint128Helper::to_m0_uint128(request->get_index_id_lo(),
                                              request->get_index_id_hi());
  // invalid oid
  if (index_id.u_hi == 0ULL && index_id.u_lo == 0ULL) {
    set_s3_error("BadRequest");
    send_response_to_s3_client();
  } else {
    if (!clovis_kv_writer) {
      clovis_kv_writer = clovis_kvs_writer_factory->create_clovis_kvs_writer(
          request, mero_clovis_api);
    }
    clovis_kv_writer->delete_keyval(
        index_id, request->get_key_name(),
        std::bind(&MeroDeleteKeyValueAction::delete_key_value_successful, this),
        std::bind(&MeroDeleteKeyValueAction::delete_key_value_failed, this));
  }

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void MeroDeleteKeyValueAction::delete_key_value_successful() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  next();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void MeroDeleteKeyValueAction::delete_key_value_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (clovis_kv_writer->get_state() == S3ClovisKVSWriterOpState::missing) {
    next();
  } else {
    set_s3_error("InternalError");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void MeroDeleteKeyValueAction::send_response_to_s3_client() {
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
