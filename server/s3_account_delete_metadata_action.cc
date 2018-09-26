/*
 * COPYRIGHT 2018 SEAGATE LLC
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
 * Original author:  Prashanth Vanaparthy   <prashanth.vanaparthy@seagate.com>
 * Original creation date: 18-JUNE-2018
 */

#include <functional>

#include "s3_error_codes.h"
#include "s3_log.h"
#include "s3_iem.h"
#include "s3_account_delete_metadata_action.h"

S3AccountDeleteMetadataAction::S3AccountDeleteMetadataAction(
    std::shared_ptr<S3RequestObject> req, std::shared_ptr<ClovisAPI> clovis_api,
    std::shared_ptr<S3ClovisKVSWriterFactory> kvs_writer_factory,
    std::shared_ptr<S3ClovisKVSReaderFactory> kvs_reader_factory,
    std::shared_ptr<S3AccountUserIdxMetadataFactory>
        s3_account_user_idx_metadata_factory)
    : S3Action(req) {
  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");
  // get the account_id from uri
  account_id_from_uri = request->c_get_file_name();

  if (clovis_api) {
    s3_clovis_api = clovis_api;
  } else {
    s3_clovis_api = std::make_shared<ConcreteClovisAPI>();
  }
  if (s3_account_user_idx_metadata_factory) {
    account_user_index_metadata_factory = s3_account_user_idx_metadata_factory;
  } else {
    account_user_index_metadata_factory =
        std::make_shared<S3AccountUserIdxMetadataFactory>();
  }
  if (kvs_writer_factory) {
    clovis_kvs_writer_factory = kvs_writer_factory;
  } else {
    clovis_kvs_writer_factory = std::make_shared<S3ClovisKVSWriterFactory>();
  }
  if (kvs_reader_factory) {
    clovis_kvs_reader_factory = kvs_reader_factory;
  } else {
    clovis_kvs_reader_factory = std::make_shared<S3ClovisKVSReaderFactory>();
  }

  bucket_list_index_oid = {0ULL, 0ULL};
  setup_steps();
}

void S3AccountDeleteMetadataAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setting up the action\n");
  add_task(std::bind(&S3AccountDeleteMetadataAction::validate_request, this));
  add_task(std::bind(
      &S3AccountDeleteMetadataAction::fetch_bucket_list_index_oid, this));
  add_task(std::bind(
      &S3AccountDeleteMetadataAction::validate_fetched_bucket_list_index_oid,
      this));
  add_task(std::bind(
      &S3AccountDeleteMetadataAction::fetch_first_bucket_metadata, this));
  add_task(std::bind(&S3AccountDeleteMetadataAction::remove_account_index_info,
                     this));
  add_task(std::bind(&S3AccountDeleteMetadataAction::remove_bucket_list_index,
                     this));
  add_task(std::bind(&S3AccountDeleteMetadataAction::send_response_to_s3_client,
                     this));
  // ...
}

void S3AccountDeleteMetadataAction::validate_request() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
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

void S3AccountDeleteMetadataAction::fetch_bucket_list_index_oid() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

  if (!account_user_index_metadata) {
    account_user_index_metadata =
        account_user_index_metadata_factory
            ->create_s3_account_user_idx_metadata(request);
  }
  account_user_index_metadata->load(
      std::bind(&S3AccountDeleteMetadataAction::next, this),
      std::bind(&S3AccountDeleteMetadataAction::next, this));

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3AccountDeleteMetadataAction::validate_fetched_bucket_list_index_oid() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

  if (account_user_index_metadata->get_state() ==
      S3AccountUserIdxMetadataState::present) {
    bucket_list_index_oid =
        account_user_index_metadata->get_bucket_list_index_oid();
    if (bucket_list_index_oid.u_lo == 0ULL &&
        bucket_list_index_oid.u_hi == 0ULL) {
      s3_log(S3_LOG_INFO, request_id,
             "Account user index metadata present, but bucket list index is "
             "missing.\n");
      send_response_to_s3_client();
    } else {
      s3_log(S3_LOG_INFO, request_id, "Bucket list index exists\n");
      next();
    }
  } else if (account_user_index_metadata->get_state() ==
             S3AccountUserIdxMetadataState::missing) {
    s3_log(S3_LOG_INFO, request_id, "Bucket list index is missing\n");
    send_response_to_s3_client();
  } else {
    set_s3_error("InternalError");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3AccountDeleteMetadataAction::fetch_first_bucket_metadata() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  clovis_kv_reader = clovis_kvs_reader_factory->create_clovis_kvs_reader(
      request, s3_clovis_api);
  clovis_kv_reader->next_keyval(
      bucket_list_index_oid, "", 1,
      std::bind(&S3AccountDeleteMetadataAction::
                     fetch_first_bucket_metadata_successful,
                this),
      std::bind(
          &S3AccountDeleteMetadataAction::fetch_first_bucket_metadata_failed,
          this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3AccountDeleteMetadataAction::fetch_first_bucket_metadata_successful() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  s3_log(S3_LOG_DEBUG, request_id,
         "Account: %s has at least one Bucket. Account delete should not be "
         "allowed.\n",
         account_id_from_uri.c_str());
  set_s3_error("AccountNotEmpty");
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3AccountDeleteMetadataAction::fetch_first_bucket_metadata_failed() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if (clovis_kv_reader->get_state() == S3ClovisKVSReaderOpState::missing) {
    s3_log(S3_LOG_DEBUG, request_id,
           "There is no bucket for the acocunt id: %s\n",
           account_id_from_uri.c_str());
    next();
  } else {
    s3_log(S3_LOG_ERROR, request_id, "Failed to retrieve bucket metadata\n");
    set_s3_error("InternalError");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3AccountDeleteMetadataAction::remove_account_index_info() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  account_user_index_metadata->remove(
      std::bind(
          &S3AccountDeleteMetadataAction::remove_account_index_info_successful,
          this),
      std::bind(
          &S3AccountDeleteMetadataAction::remove_account_index_info_failed,
          this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3AccountDeleteMetadataAction::remove_account_index_info_successful() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if (account_user_index_metadata->get_state() ==
      S3AccountUserIdxMetadataState::deleted) {
    // Account metadata delete success
    next();
  } else {
    s3_log(S3_LOG_WARN, request_id, "Failed to delete account metadata\n");
    set_s3_error("InternalError");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3AccountDeleteMetadataAction::remove_account_index_info_failed() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  s3_log(S3_LOG_ERROR, request_id, "Account metadata cleanup failed.\n");
  set_s3_error("InternalError");
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3AccountDeleteMetadataAction::remove_bucket_list_index() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  // Can happen when only index is present
  if (clovis_kv_writer == nullptr) {
    clovis_kv_writer = clovis_kvs_writer_factory->create_clovis_kvs_writer(
        request, s3_clovis_api);
  }
  clovis_kv_writer->delete_index(
      bucket_list_index_oid,
      std::bind(&S3AccountDeleteMetadataAction::next, this),
      std::bind(&S3AccountDeleteMetadataAction::remove_bucket_list_index_failed,
                this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3AccountDeleteMetadataAction::remove_bucket_list_index_failed() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  s3_log(S3_LOG_ERROR, request_id, "Account delete index failed.\n");
  if (clovis_kv_writer->get_state() ==
      S3ClovisKVSWriterOpState::failed_to_launch) {
    set_s3_error("ServiceUnavailable");
  } else {
    set_s3_error("InternalError");
  }
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3AccountDeleteMetadataAction::send_response_to_s3_client() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

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
  i_am_done();  // self delete
}
