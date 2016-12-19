/*
 * COPYRIGHT 2015 SEAGATE LLC
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
 * Original author:  Kaustubh Deorukhkar   <kaustubh.deorukhkar@seagate.com>
 * Original creation date: 1-Oct-2015
 */

#include <string>

#include "s3_option.h"
#include "s3_get_service_action.h"
#include "s3_bucket_metadata.h"
#include "s3_error_codes.h"
#include "s3_log.h"

S3GetServiceAction::S3GetServiceAction(std::shared_ptr<S3RequestObject> req) : S3Action(req), last_key(""), fetch_successful(false) {
  s3_log(S3_LOG_DEBUG, "Constructor\n");
  s3_clovis_api = std::make_shared<ConcreteClovisAPI>();
  setup_steps();
  bucket_list.set_owner_name(request->get_user_name());
  bucket_list.set_owner_id(request->get_user_id());
  bucket_list_index_oid = {0ULL, 0ULL};
}

void S3GetServiceAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, "Setting up the action\n");
  add_task(std::bind(&S3GetServiceAction::fetch_bucket_list_index_oid, this));
  add_task(std::bind( &S3GetServiceAction::get_next_buckets, this ));
  add_task(std::bind( &S3GetServiceAction::send_response_to_s3_client, this ));
  // ...
}

void S3GetServiceAction::fetch_bucket_list_index_oid() {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  account_user_index_metadata =
      std::make_shared<S3AccountUserIdxMetadata>(request);
  account_user_index_metadata->load(std::bind(&S3GetServiceAction::next, this),
                                    std::bind(&S3GetServiceAction::next, this));

  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3GetServiceAction::get_next_buckets() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "Exiting\n");
    return;
  }
  if (account_user_index_metadata->get_state() ==
      S3AccountUserIdxMetadataState::present) {
    bucket_list_index_oid =
        account_user_index_metadata->get_bucket_list_index_oid();
  }

  if (bucket_list_index_oid.u_hi == 0ULL &&
      bucket_list_index_oid.u_lo == 0ULL) {
    fetch_successful = true;  // With no entries.
    send_response_to_s3_client();
  } else {
    s3_log(S3_LOG_DEBUG, "Fetching bucket list from KV store\n");
    size_t count = S3Option::get_instance()->get_clovis_idx_fetch_count();

    clovis_kv_reader =
        std::make_shared<S3ClovisKVSReader>(request, s3_clovis_api);
    clovis_kv_reader->next_keyval(
        bucket_list_index_oid, last_key, count,
        std::bind(&S3GetServiceAction::get_next_buckets_successful, this),
        std::bind(&S3GetServiceAction::get_next_buckets_failed, this));
  }

  // for shutdown testcases, check FI and set shutdown signal
  S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL(
      "get_service_action_get_next_buckets_shutdown_fail");
}

void S3GetServiceAction::get_next_buckets_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "Exiting\n");
    return;
  }
  s3_log(S3_LOG_DEBUG, "Found buckets listing\n");
  auto& kvps = clovis_kv_reader->get_key_values();
  size_t length = kvps.size();
  for (auto& kv : kvps) {
    auto bucket = std::make_shared<S3BucketMetadata>(request);
    bucket->from_json(kv.second.second);
    bucket_list.add_bucket(bucket);
    if (--length == 0) {
      // this is the last element returned.
      last_key = kv.first;
    }
  }
  // We ask for more if there is any.
  size_t count_we_requested = S3Option::get_instance()->get_clovis_idx_fetch_count();

  if (kvps.size() < count_we_requested) {
    // Go ahead and respond.
    fetch_successful = true;
    send_response_to_s3_client();
  } else {
    get_next_buckets();
  }
}

void S3GetServiceAction::get_next_buckets_failed() {
  if (clovis_kv_reader->get_state() == S3ClovisKVSReaderOpState::missing) {
    s3_log(S3_LOG_DEBUG, "Buckets list is empty\n");
    fetch_successful = true;  // With no entries.
  } else {
    s3_log(S3_LOG_ERROR, "Failed to fetch bucket list info\n");
    fetch_successful = false;
  }
  send_response_to_s3_client();
}

void S3GetServiceAction::send_response_to_s3_client() {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  if (reject_if_shutting_down()) {
    // Send response with 'Service Unavailable' code.
    s3_log(S3_LOG_DEBUG, "sending 'Service Unavailable' response...\n");
    S3Error error("ServiceUnavailable", request->get_request_id(),
                  request->get_object_uri());
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));
    request->set_out_header_value("Retry-After", "1");

    request->send_response(error.get_http_status_code(), response_xml);
  } else if (fetch_successful) {
    std::string& response_xml = bucket_list.get_xml();
    request->set_out_header_value("Content-Length", std::to_string(response_xml.length()));
    request->set_out_header_value("Content-Type", "application/xml");
    s3_log(S3_LOG_DEBUG, "Bucket list response_xml = %s\n", response_xml.c_str());
    request->send_response(S3HttpSuccess200, response_xml);
  } else {
    S3Error error("InternalError", request->get_request_id(), "");
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length", std::to_string(response_xml.length()));

    request->send_response(error.get_http_status_code(), response_xml);
  }
  S3_RESET_SHUTDOWN_SIGNAL;  // for shutdown testcases
  done();
  i_am_done();  // self delete
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}
