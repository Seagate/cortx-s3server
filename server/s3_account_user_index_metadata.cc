/*
 * COPYRIGHT 2016 SEAGATE LLC
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
 * Original author:  Rajesh Nambiar   <rajesh.nambiar@seagate.com>
 * Original creation date: 20-July-2016
 */

#include "s3_account_user_index_metadata.h"
#include <json/json.h>
#include <string>
#include "base64.h"
#include "s3_datetime.h"
#include "s3_iem.h"

extern struct m0_uint128 root_account_user_index_oid;

S3AccountUserIdxMetadata::S3AccountUserIdxMetadata(
    std::shared_ptr<S3RequestObject> req)
    : request(req) {
  s3_log(S3_LOG_DEBUG, "Constructor");

  account_name = request->get_account_name();
  account_id = request->get_account_id();
  user_name = request->get_user_name();
  user_id = request->get_user_id();
  bucket_list_index_oid = {0ULL, 0ULL};
  state = S3AccountUserIdxMetadataState::empty;
  s3_clovis_api = std::make_shared<ConcreteClovisAPI>();
}

std::string S3AccountUserIdxMetadata::get_account_name() {
  return account_name;
}

std::string S3AccountUserIdxMetadata::get_account_id() { return account_id; }

std::string S3AccountUserIdxMetadata::get_user_name() { return user_name; }

std::string S3AccountUserIdxMetadata::get_user_id() { return user_id; }

struct m0_uint128 S3AccountUserIdxMetadata::get_bucket_list_index_oid() {
  return bucket_list_index_oid;
}

void S3AccountUserIdxMetadata::set_bucket_list_index_oid(struct m0_uint128 id) {
  bucket_list_index_oid = id;
}

void S3AccountUserIdxMetadata::load(std::function<void(void)> on_success,
                                    std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;

  // Mark missing as we initiate fetch, in case it fails to load due to missing.
  state = S3AccountUserIdxMetadataState::missing;

  clovis_kv_reader =
      std::make_shared<S3ClovisKVSReader>(request, s3_clovis_api);
  clovis_kv_reader->get_keyval(
      root_account_user_index_oid, get_account_user_index_name(),
      std::bind(&S3AccountUserIdxMetadata::load_successful, this),
      std::bind(&S3AccountUserIdxMetadata::load_failed, this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3AccountUserIdxMetadata::load_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  if (this->from_json(clovis_kv_reader->get_value()) != 0) {
    s3_log(S3_LOG_ERROR,
           "Json Parsing failed. Index = %lu %lu, Key = %s, Value = %s\n",
           root_account_user_index_oid.u_hi, root_account_user_index_oid.u_lo,
           get_account_user_index_name().c_str(),
           clovis_kv_reader->get_value().c_str());
    s3_iem(LOG_ERR, S3_IEM_METADATA_CORRUPTED, S3_IEM_METADATA_CORRUPTED_STR,
           S3_IEM_METADATA_CORRUPTED_JSON);
  }
  state = S3AccountUserIdxMetadataState::present;

  this->handler_on_success();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3AccountUserIdxMetadata::load_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  if (clovis_kv_reader->get_state() == S3ClovisKVSReaderOpState::missing) {
    s3_log(S3_LOG_DEBUG, "Account User index metadata is missing\n");
    state = S3AccountUserIdxMetadataState::missing;
  } else {
    s3_log(S3_LOG_ERROR, "Loading of account user index metadata failed\n");
    state = S3AccountUserIdxMetadataState::failed;
  }

  this->handler_on_failed();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3AccountUserIdxMetadata::save(std::function<void(void)> on_success,
                                    std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;

  // Mark missing as we initiate write, in case it fails to write.
  state = S3AccountUserIdxMetadataState::missing;
  clovis_kv_writer =
      std::make_shared<S3ClovisKVSWriter>(request, s3_clovis_api);

  clovis_kv_writer->put_keyval(
      root_account_user_index_oid, get_account_user_index_name(),
      this->to_json(),
      std::bind(&S3AccountUserIdxMetadata::save_successful, this),
      std::bind(&S3AccountUserIdxMetadata::save_failed, this));

  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3AccountUserIdxMetadata::save_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  state = S3AccountUserIdxMetadataState::saved;
  this->handler_on_success();

  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3AccountUserIdxMetadata::save_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  s3_log(S3_LOG_ERROR, "Saving of Account user metadata failed\n");

  state = S3AccountUserIdxMetadataState::failed;
  this->handler_on_failed();

  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3AccountUserIdxMetadata::remove(std::function<void(void)> on_success,
                                      std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;

  clovis_kv_writer =
      std::make_shared<S3ClovisKVSWriter>(request, s3_clovis_api);
  clovis_kv_writer->delete_keyval(
      root_account_user_index_oid, get_account_user_index_name(),
      std::bind(&S3AccountUserIdxMetadata::remove_successful, this),
      std::bind(&S3AccountUserIdxMetadata::remove_failed, this));

  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3AccountUserIdxMetadata::remove_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  state = S3AccountUserIdxMetadataState::deleted;
  this->handler_on_success();

  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3AccountUserIdxMetadata::remove_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  s3_log(S3_LOG_ERROR, "Removal of Account User metadata failed\n");

  state = S3AccountUserIdxMetadataState::failed;
  this->handler_on_failed();

  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

// Streaming to json
std::string S3AccountUserIdxMetadata::to_json() {
  s3_log(S3_LOG_DEBUG, "Called\n");
  Json::Value root;

  root["account_name"] = account_name;
  root["account_id"] = account_id;
  root["user_name"] = user_name;
  root["user_id"] = user_id;

  root["bucket_list_index_oid_u_hi"] =
      base64_encode((unsigned char const*)&bucket_list_index_oid.u_hi,
                    sizeof(bucket_list_index_oid.u_hi));
  root["bucket_list_index_oid_u_lo"] =
      base64_encode((unsigned char const*)&bucket_list_index_oid.u_lo,
                    sizeof(bucket_list_index_oid.u_lo));

  Json::FastWriter fastWriter;
  return fastWriter.write(root);
}

int S3AccountUserIdxMetadata::from_json(std::string content) {
  s3_log(S3_LOG_DEBUG, "Called\n");
  Json::Value root;
  Json::Reader reader;
  bool parsingSuccessful = reader.parse(content.c_str(), root);
  if (!parsingSuccessful) {
    s3_log(S3_LOG_ERROR, "Json Parsing failed.\n");
    return -1;
  }

  account_name = root["account_name"].asString();
  account_id = root["account_id"].asString();
  user_name = root["user_name"].asString();
  user_id = root["user_id"].asString();

  std::string bucket_list_index_oid_u_hi_str =
      base64_decode(root["bucket_list_index_oid_u_hi"].asString());
  std::string bucket_list_index_oid_u_lo_str =
      base64_decode(root["bucket_list_index_oid_u_lo"].asString());

  memcpy((void*)&bucket_list_index_oid.u_hi,
         bucket_list_index_oid_u_hi_str.c_str(),
         bucket_list_index_oid_u_hi_str.length());
  memcpy((void*)&bucket_list_index_oid.u_lo,
         bucket_list_index_oid_u_lo_str.c_str(),
         bucket_list_index_oid_u_lo_str.length());

  return 0;
}
