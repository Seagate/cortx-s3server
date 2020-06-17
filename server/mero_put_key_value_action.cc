/*
 * COPYRIGHT 2020 SEAGATE LLC
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
 * Original author: Siddhivinayak Shanbhag  <siddhivinayak.shanbhag@seagate.com>
 * Original author: Amit Kumar              <amit.kumar@seagate.com>
 * Original creation date: 16-Jun-2020
 */

#include <json/json.h>
#include "mero_put_key_value_action.h"
#include "s3_error_codes.h"
#include "s3_m0_uint128_helper.h"

MeroPutKeyValueAction::MeroPutKeyValueAction(
    std::shared_ptr<MeroRequestObject> req,
    std::shared_ptr<ClovisAPI> clovis_api,
    std::shared_ptr<S3ClovisKVSWriterFactory> clovis_mero_kvs_writer_factory)
    : MeroAction(req) {
  s3_log(S3_LOG_DEBUG, request_id, "Constructor");
  if (clovis_api) {
    mero_clovis_api = clovis_api;
  } else {
    mero_clovis_api = std::make_shared<ConcreteClovisAPI>();
  }

  if (clovis_mero_kvs_writer_factory) {
    clovis_kvs_writer_factory = clovis_mero_kvs_writer_factory;
  } else {
    clovis_kvs_writer_factory = std::make_shared<S3ClovisKVSWriterFactory>();
  }

  setup_steps();
}

void MeroPutKeyValueAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setting up the action\n");
  ACTION_TASK_ADD(MeroPutKeyValueAction::read_and_validate_key_value, this);
  ACTION_TASK_ADD(MeroPutKeyValueAction::put_key_value, this);
  ACTION_TASK_ADD(MeroPutKeyValueAction::send_response_to_s3_client, this);
}

void MeroPutKeyValueAction::read_and_validate_key_value() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  index_id = S3M0Uint128Helper::to_m0_uint128(request->get_index_id_lo(),
                                              request->get_index_id_hi());

  if (index_id.u_hi == 0ULL && index_id.u_lo == 0ULL) {  // invalid oid
    set_s3_error("BadRequest");
    send_response_to_s3_client();
  } else {
    clovis_kv_writer = clovis_kvs_writer_factory->create_clovis_kvs_writer(
        request, mero_clovis_api);
    if (request->has_all_body_content()) {
      std::string value = request->get_full_body_content_as_string();
      if (!is_valid_json(value)) {
        set_s3_error("BadRequest");
        send_response_to_s3_client();
      } else {
        json_value = value;
        next();
      }
    } else {
      // Start streaming, logically pausing action till we get data.
      request->listen_for_incoming_data(
          std::bind(&MeroPutKeyValueAction::consume_incoming_content, this),
          request->get_data_length() /* we ask for all */
          );
    }
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void MeroPutKeyValueAction::put_key_value() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  clovis_kv_writer->put_keyval(
      index_id, request->get_key_name(), json_value,
      std::bind(&MeroPutKeyValueAction::put_key_value_successful, this),
      std::bind(&MeroPutKeyValueAction::put_key_value_failed, this));

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void MeroPutKeyValueAction::put_key_value_successful() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  next();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void MeroPutKeyValueAction::put_key_value_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (clovis_kv_writer->get_state() ==
      S3ClovisKVSWriterOpState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id,
           "Failed to retrive the key, due to pre launch failure\n");
    set_s3_error("ServiceUnavailable");
  } else {
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void MeroPutKeyValueAction::consume_incoming_content() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (request->is_s3_client_read_error()) {
    client_read_error();
  } else if (request->has_all_body_content()) {
    std::string value = request->get_full_body_content_as_string();
    if (!is_valid_json(value)) {
      set_s3_error("BadRequest");
      send_response_to_s3_client();
    } else {
      json_value = value;
      next();
    }
  } else {
    // else just wait till entire body arrives. rare.
    request->resume();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

bool MeroPutKeyValueAction::is_valid_json(std::string json_str) {
  s3_log(S3_LOG_DEBUG, "", "Entering\n");
  Json::Value root;
  Json::Reader reader;
  bool parsingSuccessful = reader.parse(json_str.c_str(), root);

  if (!parsingSuccessful) {
    s3_log(S3_LOG_ERROR, request_id, "JSON string not valid.\n");
    return false;
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  return true;
}

void MeroPutKeyValueAction::send_response_to_s3_client() {
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
    request->send_response(S3HttpSuccess200);
  }
  done();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}
