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

#include <json/json.h>

#include "s3_factory.h"
#include "s3_global_bucket_index_metadata.h"
#include "s3_iem.h"
#include "s3_log.h"
#include "s3_motr_kvs_reader.h"
#include "s3_motr_kvs_writer.h"
#include "s3_request_object.h"

extern struct m0_uint128 global_bucket_list_index_oid;

S3GlobalBucketIndexMetadata::S3GlobalBucketIndexMetadata(
    std::shared_ptr<S3RequestObject> req, const std::string& str_bucket_name,
    const std::string& str_account_id, const std::string& str_account_name,
    std::shared_ptr<MotrAPI> motr_api,
    std::shared_ptr<S3MotrKVSReaderFactory> motr_s3_kvs_reader_factory,
    std::shared_ptr<S3MotrKVSWriterFactory> motr_s3_kvs_writer_factory)
    : request(std::move(req)) {

  request_id = request->get_request_id();
  stripped_request_id = request->get_stripped_request_id();

  s3_log(S3_LOG_DEBUG, request_id, "%s Ctor\n", __func__);

  if (str_account_id.empty()) {
    account_id = request->get_account_id();
    state = S3GlobalBucketIndexMetadataState::empty;
  } else {
    account_id = str_account_id;
    state = S3GlobalBucketIndexMetadataState::present;
  }
  if (str_account_name.empty()) {
    account_name = request->get_account_name();
  } else {
    account_name = str_account_name;
  }
  if (str_bucket_name.empty()) {
    bucket_name = request->get_bucket_name();
  } else {
    bucket_name = str_bucket_name;
  }
  if (motr_api) {
    s3_motr_api = std::move(motr_api);
  } else {
    s3_motr_api = std::make_shared<ConcreteMotrAPI>();
  }
  if (motr_s3_kvs_reader_factory) {
    motr_kvs_reader_factory = std::move(motr_s3_kvs_reader_factory);
  } else {
    motr_kvs_reader_factory = std::make_shared<S3MotrKVSReaderFactory>();
  }
  if (motr_s3_kvs_writer_factory) {
    motr_kvs_writer_factory = std::move(motr_s3_kvs_writer_factory);
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
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
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
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3GlobalBucketIndexMetadata::load_successful() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

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
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3GlobalBucketIndexMetadata::load_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

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
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3GlobalBucketIndexMetadata::save(std::function<void(void)> on_success,
                                       std::function<void(void)> on_failed) {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

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

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3GlobalBucketIndexMetadata::save_successful() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  state = S3GlobalBucketIndexMetadataState::saved;

  this->handler_on_success();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3GlobalBucketIndexMetadata::save_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_log(S3_LOG_ERROR, request_id, "Saving of root bucket list index failed\n");
  if (motr_kv_writer->get_state() == S3MotrKVSWriterOpState::failed_to_launch) {
    state = S3GlobalBucketIndexMetadataState::failed_to_launch;
  } else {
    state = S3GlobalBucketIndexMetadataState::failed;
  }
  this->handler_on_failed();

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3GlobalBucketIndexMetadata::remove(std::function<void(void)> on_success,
                                         std::function<void(void)> on_failed) {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;

  motr_kv_writer =
      motr_kvs_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  motr_kv_writer->delete_keyval(
      global_bucket_list_index_oid, bucket_name,
      std::bind(&S3GlobalBucketIndexMetadata::remove_successful, this),
      std::bind(&S3GlobalBucketIndexMetadata::remove_failed, this));

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3GlobalBucketIndexMetadata::remove_successful() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  state = S3GlobalBucketIndexMetadataState::deleted;

  this->handler_on_success();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3GlobalBucketIndexMetadata::remove_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_log(S3_LOG_ERROR, request_id, "Removal of bucket information failed\n");

  if (motr_kv_writer->get_state() == S3MotrKVSWriterOpState::failed_to_launch) {
    state = S3GlobalBucketIndexMetadataState::failed_to_launch;
  } else {
    state = S3GlobalBucketIndexMetadataState::failed;
  }
  this->handler_on_failed();

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

// Streaming to json
std::string S3GlobalBucketIndexMetadata::to_json() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  Json::Value root;

  root["account_name"] = account_name;
  root["account_id"] = account_id;
  root["location_constraint"] = location_constraint;

  Json::FastWriter fastWriter;
  return fastWriter.write(root);
}

int S3GlobalBucketIndexMetadata::from_json(std::string content) {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
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
