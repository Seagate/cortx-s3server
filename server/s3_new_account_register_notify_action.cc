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
#include "s3_new_account_register_notify_action.h"

S3NewAccountRegisterNotifyAction::S3NewAccountRegisterNotifyAction(
    std::shared_ptr<S3RequestObject> req, std::shared_ptr<ClovisAPI> clovis_api,
    std::shared_ptr<S3ClovisKVSWriterFactory> kvs_writer_factory,
    std::shared_ptr<S3AccountUserIdxMetadataFactory>
        s3_account_user_idx_metadata_factory)
    : S3Action(req) {
  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");
  // get the account_id from uri
  account_id_from_uri = request->c_get_file_name();
  salted_bucket_list_index_name = get_account_index_id();

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

  collision_salt = "index_salt_";
  collision_attempt_count = 0;
  bucket_list_index_oid = {0ULL, 0ULL};
  setup_steps();
}

void S3NewAccountRegisterNotifyAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, request_id, "Setting up the action\n");
  add_task(
      std::bind(&S3NewAccountRegisterNotifyAction::validate_request, this));
  add_task(std::bind(
      &S3NewAccountRegisterNotifyAction::fetch_bucket_list_index_oid, this));
  add_task(std::bind(
      &S3NewAccountRegisterNotifyAction::validate_fetched_bucket_list_index_oid,
      this));
  add_task(std::bind(
      &S3NewAccountRegisterNotifyAction::create_bucket_list_index, this));
  add_task(std::bind(
      &S3NewAccountRegisterNotifyAction::save_bucket_list_index_oid, this));
  add_task(std::bind(
      &S3NewAccountRegisterNotifyAction::send_response_to_s3_client, this));
  // ...
}

void S3NewAccountRegisterNotifyAction::validate_request() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  // check for the account_id received from authentication response and
  // account_id is in uri same or not
  if (account_id_from_uri == request->get_account_id()) {
    s3_log(S3_LOG_DEBUG, request_id, "Accound_id validation success.\n");
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

void S3NewAccountRegisterNotifyAction::fetch_bucket_list_index_oid() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

  if (!account_user_index_metadata) {
    account_user_index_metadata =
        account_user_index_metadata_factory
            ->create_s3_account_user_idx_metadata(request);
  }
  account_user_index_metadata->load(
      std::bind(&S3NewAccountRegisterNotifyAction::next, this),
      std::bind(&S3NewAccountRegisterNotifyAction::next, this));

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void
S3NewAccountRegisterNotifyAction::validate_fetched_bucket_list_index_oid() {
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
      next();
    } else {
      s3_log(S3_LOG_INFO, request_id, "Bucket list index already exists\n");
      send_response_to_s3_client();
    }
  } else if (account_user_index_metadata->get_state() ==
             S3AccountUserIdxMetadataState::missing) {
    next();
  } else {
    set_s3_error("InternalError");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3NewAccountRegisterNotifyAction::create_bucket_list_index() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if (collision_attempt_count == 0) {
    clovis_kv_writer = clovis_kvs_writer_factory->create_clovis_kvs_writer(
        request, s3_clovis_api);
  }

  clovis_kv_writer->create_index(
      salted_bucket_list_index_name,
      std::bind(&S3NewAccountRegisterNotifyAction::
                     create_bucket_list_index_successful,
                this),
      std::bind(
          &S3NewAccountRegisterNotifyAction::create_bucket_list_index_failed,
          this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3NewAccountRegisterNotifyAction::create_bucket_list_index_successful() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  bucket_list_index_oid = clovis_kv_writer->get_oid();
  next();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3NewAccountRegisterNotifyAction::create_bucket_list_index_failed() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if (clovis_kv_writer->get_state() == S3ClovisKVSWriterOpState::exists) {
    // create_bucket_list_index is called when there is no id for that index,
    // Hence if clovis returned its present then its due to collision.
    handle_collision(
        get_account_index_id(), salted_bucket_list_index_name,
        std::bind(&S3NewAccountRegisterNotifyAction::create_bucket_list_index,
                  this));
  } else if (clovis_kv_writer->get_state() ==
             S3ClovisKVSWriterOpState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id, "Index creation failed.\n");
    set_s3_error("ServiceUnavailable");
    send_response_to_s3_client();
  } else {
    s3_log(S3_LOG_ERROR, request_id, "Index creation failed.\n");
    set_s3_error("InternalError");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3NewAccountRegisterNotifyAction::save_bucket_list_index_oid() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if (!account_user_index_metadata) {
    account_user_index_metadata =
        account_user_index_metadata_factory
            ->create_s3_account_user_idx_metadata(request);
  }

  account_user_index_metadata->set_bucket_list_index_oid(bucket_list_index_oid);
  account_user_index_metadata->save(
      std::bind(&S3NewAccountRegisterNotifyAction::
                     save_bucket_list_index_oid_successful,
                this),
      std::bind(
          &S3NewAccountRegisterNotifyAction::save_bucket_list_index_oid_failed,
          this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3NewAccountRegisterNotifyAction::save_bucket_list_index_oid_successful() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if (account_user_index_metadata->get_state() ==
      S3AccountUserIdxMetadataState::saved) {
    s3_log(S3_LOG_ERROR, request_id,
           "Saving of Bucket list index oid metadata success\n");

  } else {
    s3_log(S3_LOG_ERROR, request_id,
           "Saving of Bucket list index oid metadata failed\n");
    set_s3_error("InternalError");
  }
  next();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3NewAccountRegisterNotifyAction::save_bucket_list_index_oid_failed() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  s3_log(S3_LOG_ERROR, request_id,
         "Saving of Bucket list index oid metadata failed\n");
  if (account_user_index_metadata->get_state() ==
      S3AccountUserIdxMetadataState::failed_to_launch) {
    set_s3_error("ServiceUnavailable");
  } else {
    set_s3_error("InternalError");
  }
  next();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3NewAccountRegisterNotifyAction::handle_collision(
    std::string base_index_name, std::string& salted_index_name,
    std::function<void()> callback) {
  if (collision_attempt_count < MAX_COLLISION_RETRY_COUNT) {
    s3_log(S3_LOG_INFO, request_id,
           "Index ID collision happened for index %s\n",
           salted_index_name.c_str());
    // Handle Collision
    regenerate_new_index_name(base_index_name, salted_index_name);
    collision_attempt_count++;
    if (collision_attempt_count > 5) {
      s3_log(S3_LOG_INFO, request_id,
             "Index ID collision happened %d times for index %s\n",
             collision_attempt_count, salted_index_name.c_str());
    }
    callback();
  } else {
    if (collision_attempt_count >= MAX_COLLISION_RETRY_COUNT) {
      s3_log(S3_LOG_ERROR, request_id,
             "Failed to resolve index id collision %d times for index %s\n",
             collision_attempt_count, salted_index_name.c_str());
      s3_iem(LOG_ERR, S3_IEM_COLLISION_RES_FAIL, S3_IEM_COLLISION_RES_FAIL_STR,
             S3_IEM_COLLISION_RES_FAIL_JSON);
    }
    set_s3_error("InternalError");
    send_response_to_s3_client();
  }
}

void S3NewAccountRegisterNotifyAction::regenerate_new_index_name(
    std::string base_index_name, std::string& salted_index_name) {
  salted_index_name = base_index_name + collision_salt +
                      std::to_string(collision_attempt_count);
}

void S3NewAccountRegisterNotifyAction::send_response_to_s3_client() {
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
    request->send_response(S3HttpSuccess201);
  }

  S3_RESET_SHUTDOWN_SIGNAL;  // for shutdown testcases
  done();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  i_am_done();  // self delete
}
