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

#include "s3_global_bucket_index_metadata.h"
#include <json/json.h>
#include <string>
#include "s3_factory.h"
#include "s3_iem.h"
#include "s3_datetime.h"

extern struct m0_uint128 global_bucket_list_index_oid;
extern struct m0_uint128 replica_global_bucket_list_index_oid;

S3GlobalBucketIndexMetadata::S3GlobalBucketIndexMetadata(
    std::shared_ptr<S3RequestObject> req, std::shared_ptr<MotrAPI> motr_api,
    std::shared_ptr<S3MotrKVSReaderFactory> motr_s3_kvs_reader_factory,
    std::shared_ptr<S3MotrKVSWriterFactory> motr_s3_kvs_writer_factory)
    : request(req), json_parsing_error(false) {
  request_id = request->get_request_id();
  s3_log(S3_LOG_DEBUG, request_id, "Constructor");

  account_name = request->get_account_name();
  account_id = request->get_account_id();
  bucket_name = request->get_bucket_name();
  state = S3GlobalBucketIndexMetadataState::empty;
  location_constraint = "us-west-2";
  if (motr_api) {
    s3_motr_api = motr_api;
  } else {
    s3_motr_api = std::make_shared<ConcreteMotrAPI>();
  }
  if (motr_s3_kvs_reader_factory) {
    motr_kvs_reader_factory = motr_s3_kvs_reader_factory;
  } else {
    motr_kvs_reader_factory = std::make_shared<S3MotrKVSReaderFactory>();
  }
  if (motr_s3_kvs_writer_factory) {
    motr_kvs_writer_factory = motr_s3_kvs_writer_factory;
  } else {
    motr_kvs_writer_factory = std::make_shared<S3MotrKVSWriterFactory>();
  }
}

std::string S3GlobalBucketIndexMetadata::get_account_name() {
  return account_name;
}

std::string S3GlobalBucketIndexMetadata::get_account_id() { return account_id; }

void S3GlobalBucketIndexMetadata::load(std::function<void(void)> on_success,
                                       std::function<void(void)> on_failed) {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;

  // Mark missing as we initiate fetch, in case it fails to load due to missing.
  state = S3GlobalBucketIndexMetadataState::missing;

  motr_kv_reader =
      motr_kvs_reader_factory->create_motr_kvs_reader(request, s3_motr_api);
  motr_kv_reader->get_keyval(
      global_bucket_list_index_oid, bucket_name,
      std::bind(&S3GlobalBucketIndexMetadata::load_successful, this),
      std::bind(&S3GlobalBucketIndexMetadata::load_failed, this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3GlobalBucketIndexMetadata::load_successful() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  if (this->from_json(motr_kv_reader->get_value()) != 0) {
    s3_log(S3_LOG_ERROR, request_id,
           "Json Parsing failed. Index oid = "
           "%" SCNx64 " : %" SCNx64 ", Key = %s, Value = %s\n",
           global_bucket_list_index_oid.u_hi, global_bucket_list_index_oid.u_lo,
           bucket_name.c_str(), motr_kv_reader->get_value().c_str());
    s3_iem(LOG_ERR, S3_IEM_METADATA_CORRUPTED, S3_IEM_METADATA_CORRUPTED_STR,
           S3_IEM_METADATA_CORRUPTED_JSON);

    json_parsing_error = true;
    load_failed();
  } else {
    state = S3GlobalBucketIndexMetadataState::present;
    this->handler_on_success();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3GlobalBucketIndexMetadata::load_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  if (json_parsing_error) {
    state = S3GlobalBucketIndexMetadataState::failed;
  } else if (motr_kv_reader->get_state() == S3MotrKVSReaderOpState::missing) {
    s3_log(S3_LOG_DEBUG, request_id, "bucket information is missing\n");
    state = S3GlobalBucketIndexMetadataState::missing;
  } else if (motr_kv_reader->get_state() ==
             S3MotrKVSReaderOpState::failed_to_launch) {
    state = S3GlobalBucketIndexMetadataState::failed_to_launch;
  } else {
    s3_log(S3_LOG_ERROR, request_id,
           "Loading of root bucket list index failed\n");
    state = S3GlobalBucketIndexMetadataState::failed;
  }

  this->handler_on_failed();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3GlobalBucketIndexMetadata::save(std::function<void(void)> on_success,
                                       std::function<void(void)> on_failed) {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;

  // Mark missing as we initiate write, in case it fails to write.
  state = S3GlobalBucketIndexMetadataState::missing;
  motr_kv_writer =
      motr_kvs_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  motr_kv_writer->put_keyval(
      global_bucket_list_index_oid, bucket_name, this->to_json(),
      std::bind(&S3GlobalBucketIndexMetadata::save_successful, this),
      std::bind(&S3GlobalBucketIndexMetadata::save_failed, this));

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3GlobalBucketIndexMetadata::save_successful() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  state = S3GlobalBucketIndexMetadataState::saved;

  // attempt to save the KV in replica global bucket list index
  if (!motr_kv_writer) {
    motr_kv_writer =
        motr_kvs_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->put_keyval(
      replica_global_bucket_list_index_oid, bucket_name, this->to_json(),
      std::bind(&S3GlobalBucketIndexMetadata::save_replica, this),
      std::bind(&S3GlobalBucketIndexMetadata::save_replica, this));

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3GlobalBucketIndexMetadata::save_replica() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  // PUT Operation pass even if we failed to put KV in replica index.
  if (motr_kv_writer->get_state() != S3MotrKVSWriterOpState::created) {
    s3_log(S3_LOG_ERROR, request_id, "Failed to save KV in replica index.\n");

    s3_iem_syslog(LOG_INFO, S3_IEM_METADATA_CORRUPTED,
                  "Failed to save KV in replica index for bucket: %s",
                  bucket_name.c_str());
  }
  this->handler_on_success();

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3GlobalBucketIndexMetadata::save_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  s3_log(S3_LOG_ERROR, request_id, "Saving of root bucket list index failed\n");
  if (motr_kv_writer->get_state() == S3MotrKVSWriterOpState::failed_to_launch) {
    state = S3GlobalBucketIndexMetadataState::failed_to_launch;
  } else {
    state = S3GlobalBucketIndexMetadataState::failed;
  }
  this->handler_on_failed();

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3GlobalBucketIndexMetadata::remove(std::function<void(void)> on_success,
                                         std::function<void(void)> on_failed) {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;

  motr_kv_writer =
      motr_kvs_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  motr_kv_writer->delete_keyval(
      global_bucket_list_index_oid, bucket_name,
      std::bind(&S3GlobalBucketIndexMetadata::remove_successful, this),
      std::bind(&S3GlobalBucketIndexMetadata::remove_failed, this));

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3GlobalBucketIndexMetadata::remove_successful() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  state = S3GlobalBucketIndexMetadataState::deleted;

  // attempt to remove KV from the replica index as well
  if (!motr_kv_writer) {
    motr_kv_writer =
        motr_kvs_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->delete_keyval(
      replica_global_bucket_list_index_oid, bucket_name,
      std::bind(&S3GlobalBucketIndexMetadata::remove_replica, this),
      std::bind(&S3GlobalBucketIndexMetadata::remove_replica, this));

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3GlobalBucketIndexMetadata::remove_replica() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  // DELETE bucket operation pass even if we failed to remove KV
  // from replica index
  if (motr_kv_writer->get_state() != S3MotrKVSWriterOpState::deleted) {
    s3_log(S3_LOG_ERROR, request_id,
           "Failed to remove KV from replica index.\n");

    s3_iem_syslog(LOG_INFO, S3_IEM_METADATA_CORRUPTED,
                  "Failed to remove KV from replica index of bucket: %s",
                  bucket_name.c_str());
  }
  this->handler_on_success();

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3GlobalBucketIndexMetadata::remove_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  s3_log(S3_LOG_ERROR, request_id, "Removal of bucket information failed\n");

  if (motr_kv_writer->get_state() == S3MotrKVSWriterOpState::failed_to_launch) {
    state = S3GlobalBucketIndexMetadataState::failed_to_launch;
  } else {
    state = S3GlobalBucketIndexMetadataState::failed;
  }
  this->handler_on_failed();

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

// Streaming to json
std::string S3GlobalBucketIndexMetadata::to_json() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  Json::Value root;

  root["account_name"] = account_name;
  root["account_id"] = account_id;
  root["location_constraint"] = location_constraint;

  S3DateTime current_time;
  current_time.init_current_time();
  root["create_timestamp"] = current_time.get_isoformat_string();

  Json::FastWriter fastWriter;
  return fastWriter.write(root);
}

int S3GlobalBucketIndexMetadata::from_json(std::string content) {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  Json::Value root;
  Json::Reader reader;
  bool parsingSuccessful = reader.parse(content.c_str(), root);

  if (!parsingSuccessful ||
      s3_fi_is_enabled("root_bucket_list_index_metadata_corrupted")) {
    s3_log(S3_LOG_ERROR, request_id, "Json Parsing failed.\n");
    return -1;
  }

  account_name = root["account_name"].asString();
  account_id = root["account_id"].asString();
  location_constraint = root["location_constraint"].asString();

  return 0;
}
