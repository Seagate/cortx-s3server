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
#include <stdlib.h>

#include "s3_datetime.h"
#include "s3_factory.h"
#include "s3_iem.h"
#include "s3_log.h"
#include "s3_motr_kvs_reader.h"
#include "s3_motr_kvs_writer.h"
#include "s3_part_metadata.h"
#include "s3_request_object.h"
#include "s3_m0_uint128_helper.h"

void S3PartMetadata::initialize(std::string uploadid, int part_num) {
  bucket_name = request->get_bucket_name();
  object_name = request->get_object_name();
  state = S3PartMetadataState::empty;
  upload_id = uploadid;
  part_number = std::to_string(part_num);
  put_metadata = true;
  index_name = get_part_index_name();
  salt = "index_salt_";
  collision_attempt_count = 0;
  s3_motr_api = std::make_shared<ConcreteMotrAPI>();
  pvid_str = "";
  // Set the defaults
  system_defined_attribute["Date"] = "";
  system_defined_attribute["Content-Length"] = "";
  system_defined_attribute["Last-Modified"] = "";
  system_defined_attribute["Content-MD5"] = "";

  system_defined_attribute["x-amz-server-side-encryption"] =
      "None";  // Valid values aws:kms, AES256
  system_defined_attribute["x-amz-version-id"] =
      "";  // Generate if versioning enabled
  system_defined_attribute["x-amz-storage-class"] =
      "STANDARD";  // Valid Values: STANDARD | STANDARD_IA | REDUCED_REDUNDANCY
  system_defined_attribute["x-amz-website-redirect-location"] = "None";
  system_defined_attribute["x-amz-server-side-encryption-aws-kms-key-id"] = "";
  system_defined_attribute["x-amz-server-side-encryption-customer-algorithm"] =
      "";
  system_defined_attribute["x-amz-server-side-encryption-customer-key"] = "";
  system_defined_attribute["x-amz-server-side-encryption-customer-key-MD5"] =
      "";
}

S3PartMetadata::S3PartMetadata(
    std::shared_ptr<S3RequestObject> req, std::string uploadid, int part_num,
    std::shared_ptr<S3MotrKVSReaderFactory> kv_reader_factory,
    std::shared_ptr<S3MotrKVSWriterFactory> kv_writer_factory)
    : S3PartMetadata(std::move(req), s3_motr_idx_layout{}, std::move(uploadid),
                     part_num, std::move(kv_reader_factory),
                     std::move(kv_writer_factory)) {}

S3PartMetadata::S3PartMetadata(
    std::shared_ptr<S3RequestObject> req,
    const struct s3_motr_idx_layout& part_index_layout, std::string uploadid,
    int part_num, std::shared_ptr<S3MotrKVSReaderFactory> kv_reader_factory,
    std::shared_ptr<S3MotrKVSWriterFactory> kv_writer_factory)
    : request(std::move(req)), part_index_layout(part_index_layout) {

  request_id = request->get_request_id();
  stripped_request_id = request->get_stripped_request_id();
  s3_log(S3_LOG_DEBUG, request_id, "%s Ctor\n", __func__);

  initialize(std::move(uploadid), part_num);

  if (kv_reader_factory) {
    motr_kv_reader_factory = kv_reader_factory;
  } else {
    motr_kv_reader_factory = std::make_shared<S3MotrKVSReaderFactory>();
  }

  if (kv_writer_factory) {
    mote_kv_writer_factory = kv_writer_factory;
  } else {
    mote_kv_writer_factory = std::make_shared<S3MotrKVSWriterFactory>();
  }
}

std::string S3PartMetadata::get_object_name() { return object_name; }

std::string S3PartMetadata::get_upload_id() { return upload_id; }

std::string S3PartMetadata::get_part_number() { return part_number; }

std::string S3PartMetadata::get_last_modified() {
  return system_defined_attribute["Last-Modified"];
}

std::string S3PartMetadata::get_last_modified_gmt() {
  S3DateTime temp_time;
  temp_time.init_with_iso(system_defined_attribute["Last-Modified"]);
  return temp_time.get_gmtformat_string();
}

std::string S3PartMetadata::get_last_modified_iso() {
  // we store isofmt in json
  return system_defined_attribute["Last-Modified"];
}

void S3PartMetadata::reset_date_time_to_current() {
  // we store isofmt in json
  S3DateTime current_time;
  current_time.init_current_time();
  std::string time_now = current_time.get_isoformat_string();
  system_defined_attribute["Date"] = time_now;
  system_defined_attribute["Last-Modified"] = time_now;
}

std::string S3PartMetadata::get_storage_class() {
  return system_defined_attribute["x-amz-storage-class"];
}

void S3PartMetadata::set_content_length(std::string length) {
  system_defined_attribute["Content-Length"] = length;
}

size_t S3PartMetadata::get_content_length() {
  return atol(system_defined_attribute["Content-Length"].c_str());
}

std::string S3PartMetadata::get_content_length_str() {
  return system_defined_attribute["Content-Length"];
}

void S3PartMetadata::set_md5(std::string md5) {
  system_defined_attribute["Content-MD5"] = md5;
}

std::string S3PartMetadata::get_md5() {
  return system_defined_attribute["Content-MD5"];
}

void S3PartMetadata::set_oid(struct m0_uint128 id) {
  oid = id;
  motr_oid_str = S3M0Uint128Helper::to_string(oid);
}

void S3PartMetadata::add_system_attribute(std::string key, std::string val) {
  system_defined_attribute[key] = val;
}

void S3PartMetadata::add_user_defined_attribute(std::string key,
                                                std::string val) {
  user_defined_attribute[key] = val;
}

struct m0_fid S3PartMetadata::get_pvid() {
  struct m0_fid pvid;
  S3M0Uint128Helper::to_m0_fid(pvid_str, pvid);
  return pvid;
}

void S3PartMetadata::set_pvid(const struct m0_fid* p_pvid) {
  if (p_pvid) {
    S3M0Uint128Helper::to_string(*p_pvid, pvid_str);
  } else {
    s3_log(S3_LOG_DEBUG, request_id, "%s - NULL pointer", __func__);
    pvid_str.clear();
  }
}

void S3PartMetadata::load(std::function<void(void)> on_success,
                          std::function<void(void)> on_failed,
                          int part_num = 1) {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);
  str_part_num = "";
  if (part_num > 0) {
    str_part_num = std::to_string(part_num);
  }
  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;

  state = S3PartMetadataState::empty;

  motr_kv_reader =
      motr_kv_reader_factory->create_motr_kvs_reader(request, s3_motr_api);

  motr_kv_reader->get_keyval(part_index_layout, str_part_num,
                             std::bind(&S3PartMetadata::load_successful, this),
                             std::bind(&S3PartMetadata::load_failed, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

bool S3PartMetadata::validate_on_request() {
  return request->validate_attrs(bucket_name, object_name);
}

void S3PartMetadata::load_successful() {
  s3_log(S3_LOG_DEBUG, request_id, "Found part metadata\n");

  if (this->from_json(motr_kv_reader->get_value()) != 0) {

    s3_log(S3_LOG_ERROR, request_id,
           "Json Parsing failed. Index oid = "
           "%" SCNx64 ":%" SCNx64 ", Key = %s, Value = %s\n",
           part_index_layout.oid.u_hi, part_index_layout.oid.u_lo,
           str_part_num.c_str(), motr_kv_reader->get_value().c_str());

    s3_iem(LOG_ERR, S3_IEM_METADATA_CORRUPTED, S3_IEM_METADATA_CORRUPTED_STR,
           S3_IEM_METADATA_CORRUPTED_JSON);

    state = S3PartMetadataState::invalid;
    load_failed();
  } else if (!validate_on_request()) {
    s3_log(S3_LOG_ERROR, request_id,
           "Metadata read from KVS are different from expected. Index oid = "
           "%" SCNx64 ":%" SCNx64
           ", req_bucket = %s, got_bucket = %s, req_object = %s, got_object = "
           "%s\n",
           part_index_layout.oid.u_hi, part_index_layout.oid.u_lo,
           request->get_bucket_name().c_str(), bucket_name.c_str(),
           request->get_object_name().c_str(), object_name.c_str());

    state = S3PartMetadataState::invalid;
    load_failed();
  } else {
    state = S3PartMetadataState::present;
    this->handler_on_success();
  }
}

void S3PartMetadata::load_failed() {
  switch (motr_kv_reader->get_state()) {
    case S3MotrKVSReaderOpState::failed_to_launch:
      state = S3PartMetadataState::failed_to_launch;
      s3_log(S3_LOG_WARN, request_id,
             "Part metadata load failed to launch - ServiceUnavailable\n");
      break;
    case S3MotrKVSReaderOpState::failed:
    case S3MotrKVSReaderOpState::failed_e2big:
      s3_log(S3_LOG_WARN, request_id,
             "Internal server error - InternalError\n");
      state = S3PartMetadataState::failed;
      break;
    case S3MotrKVSReaderOpState::missing:
      state = S3PartMetadataState::missing;
      s3_log(S3_LOG_DEBUG, request_id, "Part metadata missing for %s\n",
             object_name.c_str());
      break;
    case S3MotrKVSReaderOpState::present:
      // This state is allowed here only if validaton failed
      if (state != S3PartMetadataState::invalid) {
        s3_log(S3_LOG_ERROR, request_id, "Invalid state - InternalError\n");
        state = S3PartMetadataState::failed;
      }
      break;
    default:  // S3MotrKVSReaderOpState::{empty,start}
      s3_log(S3_LOG_ERROR, request_id, "Unexpected state - InternalError\n");
      state = S3PartMetadataState::failed;
      break;
  }

  this->handler_on_failed();
}

void S3PartMetadata::create_index(std::function<void(void)> on_success,
                                  std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);
  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;
  put_metadata = false;
  create_part_index();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PartMetadata::save(std::function<void(void)> on_success,
                          std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);

  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;
  put_metadata = true;
  save_metadata();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PartMetadata::create_part_index() {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);
  // Mark missing as we initiate write, in case it fails to write.
  state = S3PartMetadataState::missing;

  motr_kv_writer =
      mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);

  motr_kv_writer->create_index(
      index_name,
      std::bind(&S3PartMetadata::create_part_index_successful, this),
      std::bind(&S3PartMetadata::create_part_index_failed, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PartMetadata::create_part_index_successful() {
  s3_log(S3_LOG_DEBUG, request_id, "Created index for part info\n");

  part_index_layout = motr_kv_writer->get_index_layout();

  if (put_metadata) {
    save_metadata();
  } else {
    state = S3PartMetadataState::store_created;
    this->handler_on_success();
  }
}

void S3PartMetadata::create_part_index_failed() {
  s3_log(S3_LOG_DEBUG, request_id, "Failed to create index for part info\n");
  if (motr_kv_writer->get_state() == S3MotrKVSWriterOpState::exists) {
    // Since part index name comprises of bucket name + object name + upload id,
    // upload id is unique
    // hence if motr returns exists then its due to collision, resolve it
    s3_log(S3_LOG_WARN, request_id, "Collision detected for Index %s\n",
           index_name.c_str());
    handle_collision();
  } else if (motr_kv_writer->get_state() ==
             S3MotrKVSWriterOpState::failed_to_launch) {
    state = S3PartMetadataState::failed_to_launch;
    this->handler_on_failed();
  } else {
    state = S3PartMetadataState::failed;  // todo Check error
    this->handler_on_failed();
  }
}

void S3PartMetadata::save_metadata() {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);
  // Set up system attributes
  system_defined_attribute["Upload-ID"] = upload_id;
  if (!motr_kv_writer) {
    motr_kv_writer =
        mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->put_keyval(
      part_index_layout, part_number, this->to_json(),
      std::bind(&S3PartMetadata::save_metadata_successful, this),
      std::bind(&S3PartMetadata::save_metadata_failed, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PartMetadata::save_metadata_successful() {
  s3_log(S3_LOG_DEBUG, request_id, "Saved part metadata\n");
  state = S3PartMetadataState::saved;
  this->handler_on_success();
}

void S3PartMetadata::save_metadata_failed() {
  // TODO - do anything more for failure?
  s3_log(S3_LOG_ERROR, request_id, "Failed to save part metadata\n");
  if (motr_kv_writer->get_state() == S3MotrKVSWriterOpState::failed_to_launch) {
    state = S3PartMetadataState::failed_to_launch;
  } else {
    state = S3PartMetadataState::failed;
  }
  this->handler_on_failed();
}

void S3PartMetadata::remove(std::function<void(void)> on_success,
                            std::function<void(void)> on_failed,
                            int remove_part = 0) {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);
  std::string part_removal = std::to_string(remove_part);

  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;

  s3_log(S3_LOG_DEBUG, request_id, "Deleting part info for part = %s\n",
         part_removal.c_str());

  if (!motr_kv_writer) {
    motr_kv_writer =
        mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->delete_keyval(
      part_index_layout, part_removal,
      std::bind(&S3PartMetadata::remove_successful, this),
      std::bind(&S3PartMetadata::remove_failed, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PartMetadata::remove_successful() {
  s3_log(S3_LOG_DEBUG, request_id, "Deleted part info\n");
  state = S3PartMetadataState::deleted;
  this->handler_on_success();
}

void S3PartMetadata::remove_failed() {
  s3_log(S3_LOG_WARN, request_id, "Failed to delete part info\n");
  if (motr_kv_writer->get_state() == S3MotrKVSWriterOpState::failed_to_launch) {
    state = S3PartMetadataState::failed_to_launch;
  } else {
    state = S3PartMetadataState::failed;
  }
  this->handler_on_failed();
}

void S3PartMetadata::remove_index(std::function<void(void)> on_success,
                                  std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);

  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;
  if (!motr_kv_writer) {
    motr_kv_writer =
        mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->delete_index(
      part_index_layout,
      std::bind(&S3PartMetadata::remove_index_successful, this),
      std::bind(&S3PartMetadata::remove_index_failed, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3PartMetadata::remove_index_successful() {
  s3_log(S3_LOG_DEBUG, request_id, "Deleted index for part info\n");
  state = S3PartMetadataState::index_deleted;
  this->handler_on_success();
}

void S3PartMetadata::remove_index_failed() {
  s3_log(S3_LOG_WARN, request_id, "Failed to remove index for part info\n");
  // s3_iem(LOG_ERR, S3_IEM_DELETE_IDX_FAIL, S3_IEM_DELETE_IDX_FAIL_STR,
  //     S3_IEM_DELETE_IDX_FAIL_JSON);
  if (motr_kv_writer->get_state() == S3MotrKVSWriterOpState::failed) {
    state = S3PartMetadataState::failed;
  } else if (motr_kv_writer->get_state() ==
             S3MotrKVSWriterOpState::failed_to_launch) {
    state = S3PartMetadataState::failed_to_launch;
  }
  this->handler_on_failed();
}

// Streaming to json
std::string S3PartMetadata::to_json() {
  s3_log(S3_LOG_DEBUG, request_id, "\n");
  Json::Value root;
  root["Bucket-Name"] = bucket_name;
  if (s3_di_fi_is_enabled("di_part_metadata_bcktname_on_write_corrupted")) {
    root["Bucket-Name"] = "@" + bucket_name + "@";
  }
  root["Object-Name"] = object_name;
  if (s3_di_fi_is_enabled("di_part_metadata_objname_on_write_corrupted")) {
    root["Object-Name"] = "@" + object_name + "@";
  }
  root["Upload-ID"] = upload_id;
  root["Part-Num"] = part_number;
  root["motr_oid"] = motr_oid_str;
  root["layout_id"] = layout_id;
  root["PVID"] = this->pvid_str;

  for (auto sit : system_defined_attribute) {
    root["System-Defined"][sit.first] = sit.second;
  }
  for (auto uit : user_defined_attribute) {
    root["User-Defined"][uit.first] = uit.second;
  }
  Json::FastWriter fastWriter;
  return fastWriter.write(root);
}

int S3PartMetadata::from_json(std::string content) {
  s3_log(S3_LOG_DEBUG, request_id, "\n");
  Json::Value newroot;
  Json::Reader reader;
  bool parsingSuccessful = reader.parse(content.c_str(), newroot);
  if (!parsingSuccessful || s3_di_fi_is_enabled("part_metadata_corrupted")) {
    s3_log(S3_LOG_ERROR, request_id, "Json Parsing failed.\n");
    return -1;
  }

  bucket_name = newroot["Bucket-Name"].asString();
  if (s3_di_fi_is_enabled("di_part_metadata_bcktname_on_read_corrupted")) {
    bucket_name = "@" + bucket_name + "@";
  }
  object_name = newroot["Object-Name"].asString();
  if (s3_di_fi_is_enabled("di_part_metadata_objname_on_read_corrupted")) {
    object_name = "@" + object_name + "@";
  }
  upload_id = newroot["Upload-ID"].asString();
  part_number = newroot["Part-Num"].asString();
  motr_oid_str = newroot["motr_oid"].asString();
  oid = S3M0Uint128Helper::to_m0_uint128(motr_oid_str);
  layout_id = newroot["layout_id"].asInt();
  pvid_str = newroot["PVID"].asString();

  Json::Value::Members members = newroot["System-Defined"].getMemberNames();
  for (auto it : members) {
    system_defined_attribute[it.c_str()] =
        newroot["System-Defined"][it].asString().c_str();
  }
  members = newroot["User-Defined"].getMemberNames();
  for (auto it : members) {
    user_defined_attribute[it.c_str()] =
        newroot["User-Defined"][it].asString().c_str();
  }

  return 0;
}

void S3PartMetadata::regenerate_new_indexname() {
  index_name = index_name + salt + std::to_string(collision_attempt_count);
}

void S3PartMetadata::handle_collision() {
  if (collision_attempt_count < MAX_COLLISION_RETRY_COUNT) {
    s3_log(S3_LOG_INFO, stripped_request_id,
           "Object ID collision happened for index %s\n", index_name.c_str());
    // Handle Collision
    regenerate_new_indexname();
    collision_attempt_count++;
    if (collision_attempt_count > 5) {
      s3_log(S3_LOG_INFO, stripped_request_id,
             "Object ID collision happened %zu times for index %s\n",
             collision_attempt_count, index_name.c_str());
    }
    create_part_index();
  } else {
    if (collision_attempt_count >= MAX_COLLISION_RETRY_COUNT) {
      s3_log(S3_LOG_ERROR, request_id,
             "Failed to resolve object id collision %zu times for index %s\n",
             collision_attempt_count, index_name.c_str());
      // s3_iem(LOG_ERR, S3_IEM_COLLISION_RES_FAIL,
      // S3_IEM_COLLISION_RES_FAIL_STR,
      //     S3_IEM_COLLISION_RES_FAIL_JSON);
    }
    state = S3PartMetadataState::failed;
    this->handler_on_failed();
  }
}
