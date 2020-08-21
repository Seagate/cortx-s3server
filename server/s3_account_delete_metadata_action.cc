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

#include <functional>

#include "s3_error_codes.h"
#include "s3_log.h"
#include "s3_iem.h"
#include "s3_account_delete_metadata_action.h"

extern struct m0_uint128 bucket_metadata_list_index_oid;

S3AccountDeleteMetadataAction::S3AccountDeleteMetadataAction(
    std::shared_ptr<S3RequestObject> req, std::shared_ptr<MotrAPI> motr_api,
    std::shared_ptr<S3MotrKVSReaderFactory> kvs_reader_factory)
    : S3Action(req, true, nullptr, false, true) {
  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");
  // get the account_id from uri
  account_id_from_uri = request->c_get_file_name();

  if (motr_api) {
    s3_motr_api = motr_api;
  } else {
    s3_motr_api = std::make_shared<ConcreteMotrAPI>();
  }

  if (kvs_reader_factory) {
    motr_kvs_reader_factory = kvs_reader_factory;
  } else {
    motr_kvs_reader_factory = std::make_shared<S3MotrKVSReaderFactory>();
  }
  setup_steps();
}

void S3AccountDeleteMetadataAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setting up the action\n");
  ACTION_TASK_ADD(S3AccountDeleteMetadataAction::validate_request, this);
  ACTION_TASK_ADD(S3AccountDeleteMetadataAction::fetch_first_bucket_metadata,
                  this);
  ACTION_TASK_ADD(S3AccountDeleteMetadataAction::send_response_to_s3_client,
                  this);
  // ...
}

void S3AccountDeleteMetadataAction::validate_request() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  // check for the account_id got from authentication response and account_id is
  // in uri same or not
  if (account_id_from_uri == request->get_account_id()) {
    s3_log(S3_LOG_DEBUG, request_id,
           "Accound_id validation success. URI account_id: %s matches with the "
           "account_id: %s recevied from auth server.\n",
           account_id_from_uri.c_str(), request->get_account_id().c_str());
    next();  // account_id's are same, so perform next action
  } else {
    s3_log(S3_LOG_DEBUG, request_id,
           "Accound_id validation failed. URI account_id: %s does not match "
           "with the account_id: %s recevied from auth server.\n",
           account_id_from_uri.c_str(), request->get_account_id().c_str());
    set_s3_error("InvalidAccountForMgmtApi");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3AccountDeleteMetadataAction::fetch_first_bucket_metadata() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  motr_kv_reader =
      motr_kvs_reader_factory->create_motr_kvs_reader(request, s3_motr_api);
  bucket_account_id_key_prefix = account_id_from_uri + "/";
  motr_kv_reader->next_keyval(
      bucket_metadata_list_index_oid, bucket_account_id_key_prefix, 1,
      std::bind(&S3AccountDeleteMetadataAction::
                     fetch_first_bucket_metadata_successful,
                this),
      std::bind(
          &S3AccountDeleteMetadataAction::fetch_first_bucket_metadata_failed,
          this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3AccountDeleteMetadataAction::fetch_first_bucket_metadata_successful() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  auto& kvps = motr_kv_reader->get_key_values();
  for (auto& kv : kvps) {
    // account_id_from_uri has buckets
    if (kv.first.find(bucket_account_id_key_prefix) != std::string::npos) {
      s3_log(S3_LOG_DEBUG, request_id,
             "Account: %s has at least one Bucket. Account delete should not "
             "be allowed.\n",
             account_id_from_uri.c_str());
      set_s3_error("AccountNotEmpty");
      send_response_to_s3_client();
    } else {  // no buckets found in given account
      next();
    }
    break;
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3AccountDeleteMetadataAction::fetch_first_bucket_metadata_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (motr_kv_reader->get_state() == S3MotrKVSReaderOpState::missing) {
    s3_log(S3_LOG_DEBUG, request_id,
           "There is no bucket for the acocunt id: %s\n",
           account_id_from_uri.c_str());
    next();
  } else if (motr_kv_reader->get_state() ==
             S3MotrKVSReaderOpState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id,
           "Bucket metadata next keyval operation failed due to pre launch "
           "failure\n");
    set_s3_error("ServiceUnavailable");
    send_response_to_s3_client();
  } else {
    s3_log(S3_LOG_ERROR, request_id, "Failed to retrieve bucket metadata\n");
    set_s3_error("InternalError");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3AccountDeleteMetadataAction::send_response_to_s3_client() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  if (reject_if_shutting_down() ||
      (is_error_state() && !get_s3_error_code().empty())) {
    S3Error error(get_s3_error_code(), request->get_request_id(),
                  request->get_account_id());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));
    if (get_s3_error_code() == "ServiceUnavailable") {
      request->set_out_header_value("Retry-After", "1");
    }

    request->send_response(error.get_http_status_code(), response_xml);
  } else if (get_s3_error_code().empty()) {
    request->send_response(S3HttpSuccess204);
  }

  S3_RESET_SHUTDOWN_SIGNAL;  // for shutdown testcases
  done();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}
