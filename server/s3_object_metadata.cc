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

#include <cstdlib>
#include <cassert>

#include <json/json.h>

#include "base64.h"
#include "s3_datetime.h"
#include "s3_factory.h"
#include "s3_iem.h"
#include "s3_log.h"
#include "s3_object_metadata.h"
#include "s3_object_versioning_helper.h"
#include "s3_uri_to_motr_oid.h"
#include "s3_common_utilities.h"
#include "s3_m0_uint128_helper.h"
#include "s3_stats.h"

extern struct m0_uint128 global_instance_id;

S3ObjectMetadata::S3ObjectMetadata(const S3ObjectMetadata& src)
    : S3ObjectMetadataCopyable(src), state(S3ObjectMetadataState::present) {

  s3_log(S3_LOG_DEBUG, request_id, "Partial copy constructor");

  bucket_name = request->get_bucket_name();
  object_name = request->get_object_name();

  initialize(src.is_multipart, src.upload_id);
}

static void str_set_default(std::string& sref, const char* sz) {
  if (sref.empty() && sz) {
    sref = sz;
  }
}

void S3ObjectMetadata::initialize(bool ismultipart, std::string uploadid) {
  account_name = request->get_account_name();
  account_id = request->get_account_id();
  user_name = request->get_user_name();
  canonical_id = request->get_canonical_id();
  user_id = request->get_user_id();

  object_key_uri = bucket_name + "\\" + object_name;

  // Set the defaults
  (void)system_defined_attribute["Date"];
  (void)system_defined_attribute["Content-Length"];
  (void)system_defined_attribute["Last-Modified"];
  (void)system_defined_attribute["Content-MD5"];

  // Initially set to requestor of API, but on load json can be overriden
  system_defined_attribute["Owner-User"] = user_name;
  system_defined_attribute["Owner-Canonical-id"] = canonical_id;
  system_defined_attribute["Owner-User-id"] = user_id;
  system_defined_attribute["Owner-Account"] = account_name;
  system_defined_attribute["Owner-Account-id"] = account_id;

  str_set_default(system_defined_attribute["x-amz-server-side-encryption"],
                  "None");
  (void)system_defined_attribute["x-amz-version-id"];

  // Generate if versioning enabled
  str_set_default(system_defined_attribute["x-amz-storage-class"], "STANDARD");
  str_set_default(system_defined_attribute["x-amz-website-redirect-location"],
                  "None");

  (void)system_defined_attribute["x-amz-server-side-encryption-aws-kms-key-id"];
  (void)system_defined_attribute
      ["x-amz-server-side-encryption-customer-algorithm"];
  (void)system_defined_attribute["x-amz-server-side-encryption-customer-key"];
  (void)
      system_defined_attribute["x-amz-server-side-encryption-customer-key-MD5"];

  if (is_multipart) {
    index_name = get_multipart_index_name();
    system_defined_attribute["Upload-ID"] = upload_id;
    (void)system_defined_attribute["Part-One-Size"];
  } else {
    index_name = get_object_list_index_name();
  }
}

S3ObjectMetadata::S3ObjectMetadata(
    std::shared_ptr<S3RequestObject> req, std::string bucketname,
    std::string objectname, bool ismultipart, std::string uploadid,
    std::shared_ptr<S3MotrKVSReaderFactory> kv_reader_factory,
    std::shared_ptr<S3MotrKVSWriterFactory> kv_writer_factory,
    std::shared_ptr<MotrAPI> motr_api)
    : upload_id(std::move(uploadid)),
      is_multipart(ismultipart),
      state(S3ObjectMetadataState::empty) {

  request = std::move(req);
  request_id = request->get_request_id();
  stripped_request_id = request->get_stripped_request_id();
  s3_log(S3_LOG_DEBUG, request_id, "%s Ctor\n", __func__);

  if (bucketname.empty()) {
    bucket_name = request->get_bucket_name();
  } else {
    bucket_name = std::move(bucketname);
  }
  if (objectname.empty()) {
    object_name = request->get_object_name();
  } else {
    object_name = std::move(objectname);
  }
  if (motr_api) {
    s3_motr_api = std::move(motr_api);
  } else {
    s3_motr_api = std::make_shared<ConcreteMotrAPI>();
  }
  if (kv_reader_factory) {
    motr_kv_reader_factory = std::move(kv_reader_factory);
  } else {
    motr_kv_reader_factory = std::make_shared<S3MotrKVSReaderFactory>();
  }
  if (kv_writer_factory) {
    mote_kv_writer_factory = std::move(kv_writer_factory);
  } else {
    mote_kv_writer_factory = std::make_shared<S3MotrKVSWriterFactory>();
  }
  initialize();
}

S3ObjectMetadata::S3ObjectMetadata(
    std::shared_ptr<S3RequestObject> req, bool ismultipart,
    std::string uploadid,
    std::shared_ptr<S3MotrKVSReaderFactory> kv_reader_factory,
    std::shared_ptr<S3MotrKVSWriterFactory> kv_writer_factory,
    std::shared_ptr<MotrAPI> motr_api)
    : S3ObjectMetadata(std::move(req), "", "", ismultipart, std::move(uploadid),
                       std::move(kv_reader_factory),
                       std::move(kv_writer_factory), std::move(motr_api)) {}

std::string S3ObjectMetadata::get_owner_id() {
  return system_defined_attribute["Owner-User-id"];
}

std::string S3ObjectMetadata::get_owner_name() {
  return system_defined_attribute["Owner-User"];
}

std::string S3ObjectMetadata::get_object_name() { return object_name; }

std::string S3ObjectMetadata::get_bucket_name() { return bucket_name; }

void S3ObjectMetadata::set_object_list_index_oid(struct m0_uint128 id) {
  object_list_index_oid.u_hi = id.u_hi;
  object_list_index_oid.u_lo = id.u_lo;
}

void S3ObjectMetadata::set_objects_version_list_index_oid(
    struct m0_uint128 id) {
  objects_version_list_index_oid.u_hi = id.u_hi;
  objects_version_list_index_oid.u_lo = id.u_lo;
}

struct m0_uint128 S3ObjectMetadata::get_object_list_index_oid() const {
  return object_list_index_oid;
}

struct m0_uint128 S3ObjectMetadata::get_objects_version_list_index_oid() const {
  return objects_version_list_index_oid;
}

void S3ObjectMetadata::regenerate_version_id() {
  // generate new epoch time value for new object
  rev_epoch_version_id_key = S3ObjectVersioingHelper::generate_new_epoch_time();
  // set version id
  object_version_id = S3ObjectVersioingHelper::get_versionid_from_epoch_time(
      rev_epoch_version_id_key);
  system_defined_attribute["x-amz-version-id"] = object_version_id;
}

std::string S3ObjectMetadata::get_version_key_in_index() {
  assert(!object_name.empty());
  assert(!rev_epoch_version_id_key.empty());
  // sample objectname/revversionkey
  return object_name + "/" + rev_epoch_version_id_key;
}

std::string S3ObjectMetadata::get_user_id() { return user_id; }

std::string S3ObjectMetadata::get_upload_id() { return upload_id; }

std::string S3ObjectMetadata::get_user_name() { return user_name; }

std::string S3ObjectMetadata::get_canonical_id() { return canonical_id; }

std::string S3ObjectMetadata::get_account_name() { return account_name; }

std::string S3ObjectMetadata::get_creation_date() {
  return system_defined_attribute["Date"];
}

std::string S3ObjectMetadata::get_last_modified() {
  return system_defined_attribute["Last-Modified"];
}

std::string S3ObjectMetadata::get_last_modified_gmt() {
  S3DateTime temp_time;
  temp_time.init_with_iso(system_defined_attribute["Last-Modified"]);
  return temp_time.get_gmtformat_string();
}

std::string S3ObjectMetadata::get_last_modified_iso() {
  // we store isofmt in json
  return system_defined_attribute["Last-Modified"];
}

void S3ObjectMetadata::reset_date_time_to_current() {
  S3DateTime current_time;
  current_time.init_current_time();
  std::string time_now = current_time.get_isoformat_string();
  system_defined_attribute["Date"] = time_now;
  system_defined_attribute["Last-Modified"] = time_now;
}

std::string S3ObjectMetadata::get_storage_class() {
  return system_defined_attribute["x-amz-storage-class"];
}

void S3ObjectMetadata::set_content_length(std::string length) {
  system_defined_attribute["Content-Length"] = length;
}

void S3ObjectMetadata::set_content_type(std::string content_type) {
  system_defined_attribute["Content-Type"] = std::move(content_type);
}

std::string S3ObjectMetadata::get_content_type() {
  return system_defined_attribute["Content-Type"];
}

size_t S3ObjectMetadata::get_content_length() {
  return atol(system_defined_attribute["Content-Length"].c_str());
}

std::string S3ObjectMetadata::get_content_length_str() {
  return system_defined_attribute["Content-Length"];
}

void S3ObjectMetadata::set_part_one_size(const size_t& part_size) {
  system_defined_attribute["Part-One-Size"] = std::to_string(part_size);
}

size_t S3ObjectMetadata::get_part_one_size() {
  if (system_defined_attribute["Part-One-Size"] == "") {
    return 0;
  } else {
    return std::stoul(system_defined_attribute["Part-One-Size"].c_str());
  }
}

void S3ObjectMetadata::set_md5(std::string md5) {
  system_defined_attribute["Content-MD5"] = md5;
}

std::string S3ObjectMetadata::get_md5() {
  return system_defined_attribute["Content-MD5"];
}

void S3ObjectMetadata::set_oid(struct m0_uint128 id) {
  oid = id;
  motr_oid_str = S3M0Uint128Helper::to_string(oid);
}

void S3ObjectMetadata::set_version_id(std::string ver_id) {
  object_version_id = ver_id;
  rev_epoch_version_id_key =
      S3ObjectVersioingHelper::generate_keyid_from_versionid(object_version_id);
}

void S3ObjectMetadata::set_old_oid(struct m0_uint128 id) {
  old_oid = id;
  motr_old_oid_str = S3M0Uint128Helper::to_string(old_oid);
}

void S3ObjectMetadata::set_old_version_id(std::string old_obj_ver_id) {
  motr_old_object_version_id = old_obj_ver_id;
}

void S3ObjectMetadata::set_part_index_oid(struct m0_uint128 id) {
  part_index_oid = id;
  motr_part_oid_str = S3M0Uint128Helper::to_string(part_index_oid);
}

void S3ObjectMetadata::add_system_attribute(std::string key, std::string val) {
  system_defined_attribute[key] = val;
}

void S3ObjectMetadata::add_user_defined_attribute(std::string key,
                                                  std::string val) {
  user_defined_attribute[key] = val;
}

void S3ObjectMetadata::validate() {
  // TODO
}

void S3ObjectMetadata::load(std::function<void(void)> on_success,
                            std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);
  // object_list_index_oid should be set before using this method
  assert(object_list_index_oid.u_hi || object_list_index_oid.u_lo);

  s3_timer.start();

  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;

  state = S3ObjectMetadataState::empty;

  motr_kv_reader =
      motr_kv_reader_factory->create_motr_kvs_reader(request, s3_motr_api);
  motr_kv_reader->get_keyval(
      object_list_index_oid, object_name,
      std::bind(&S3ObjectMetadata::load_successful, this),
      std::bind(&S3ObjectMetadata::load_failed, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3ObjectMetadata::load_successful() {
  s3_log(S3_LOG_DEBUG, request_id, "Object metadata load successful\n");
  if (this->from_json(motr_kv_reader->get_value()) != 0) {
    s3_log(S3_LOG_ERROR, request_id,
           "Json Parsing failed. Index oid = "
           "%" SCNx64 ":%" SCNx64 ", Key = %s, Value = %s\n",
           object_list_index_oid.u_hi, object_list_index_oid.u_lo,
           object_name.c_str(), motr_kv_reader->get_value().c_str());
    s3_iem(LOG_ERR, S3_IEM_METADATA_CORRUPTED, S3_IEM_METADATA_CORRUPTED_STR,
           S3_IEM_METADATA_CORRUPTED_JSON);

    state = S3ObjectMetadataState::invalid;
    load_failed();
  } else if (!request->validate_attrs(bucket_name, object_name)) {
    s3_log(S3_LOG_ERROR, request_id,
           "Metadata read from KVS are different from expected. Index oid = "
           "%" SCNx64 ":%" SCNx64
           ", req_bucket = %s, got_bucket = %s, req_object = %s, got_object = "
           "%s\n",
           object_list_index_oid.u_hi, object_list_index_oid.u_lo,
           request->get_bucket_name().c_str(), bucket_name.c_str(),
           request->get_object_name().c_str(), object_name.c_str());
    state = S3ObjectMetadataState::invalid;
    load_failed();
  } else {
    s3_timer.stop();
    const auto mss = s3_timer.elapsed_time_in_millisec();
    LOG_PERF("load_object_metadata_ms", request_id.c_str(), mss);
    s3_stats_timing("load_object_metadata", mss);

    state = S3ObjectMetadataState::present;
    this->handler_on_success();
  }
}

void S3ObjectMetadata::load_failed() {
  switch (motr_kv_reader->get_state()) {
  case S3MotrKVSReaderOpState::failed_to_launch:
    state = S3ObjectMetadataState::failed_to_launch;
    s3_log(S3_LOG_WARN, request_id, "Object metadata load failed to launch - ServiceUnavailable");
    break;
  case S3MotrKVSReaderOpState::failed:
  case S3MotrKVSReaderOpState::failed_e2big:
    s3_log(S3_LOG_WARN, request_id, "Internal server error - InternalError");
    state = S3ObjectMetadataState::failed;
    break;
  case S3MotrKVSReaderOpState::missing:
    state = S3ObjectMetadataState::missing;
    s3_log(S3_LOG_DEBUG, request_id, "Object metadata missing for %s",
           object_name.c_str());
    break;
  case S3MotrKVSReaderOpState::present:
    // This state is allowed here only if validaton failed
    if (state != S3ObjectMetadataState::invalid) {
      s3_log(S3_LOG_ERROR, request_id, "Invalid state - InternalError");
      state = S3ObjectMetadataState::failed;
    }
    break;
  default: // S3MotrKVSReaderOpState::{empty,start}
    s3_log(S3_LOG_ERROR, request_id, "Unexpected state - InternalError");
    state = S3ObjectMetadataState::failed;
    break;
  }

  this->handler_on_failed();
}

void S3ObjectMetadata::save(std::function<void(void)> on_success,
                            std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, request_id, "Saving Object metadata\n");

  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;
  if (is_multipart) {
    // Write only to multpart object list and not real object list in a bucket.
    save_metadata();
  } else {
    // First write metadata to objects version list index for a bucket.
    // Next write metadata to object list index for a bucket.
    save_version_metadata();
  }
}

// Save to objects version list index
void S3ObjectMetadata::save_version_metadata() {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);
  // objects_version_list_index_oid should be set before using this method
  assert(objects_version_list_index_oid.u_hi ||
         objects_version_list_index_oid.u_lo);

  motr_kv_writer =
      mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  motr_kv_writer->put_keyval(
      objects_version_list_index_oid, get_version_key_in_index(),
      this->version_entry_to_json(),
      std::bind(&S3ObjectMetadata::save_version_metadata_successful, this),
      std::bind(&S3ObjectMetadata::save_version_metadata_failed, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3ObjectMetadata::save_version_metadata_successful() {
  s3_log(S3_LOG_DEBUG, request_id, "Version metadata saved for Object [%s].\n",
         object_name.c_str());
  save_metadata();
}

void S3ObjectMetadata::save_version_metadata_failed() {
  s3_log(S3_LOG_ERROR, request_id,
         "Version metadata save failed for Object [%s].\n",
         object_name.c_str());
  if (motr_kv_writer->get_state() == S3MotrKVSWriterOpState::failed_to_launch) {
    state = S3ObjectMetadataState::failed_to_launch;
  } else {
    state = S3ObjectMetadataState::failed;
  }
  this->handler_on_failed();
}

void S3ObjectMetadata::save_metadata() {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);
  // object_list_index_oid should be set before using this method
  assert(object_list_index_oid.u_hi || object_list_index_oid.u_lo);

  motr_kv_writer =
      mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  motr_kv_writer->put_keyval(
      object_list_index_oid, object_name, this->to_json(),
      std::bind(&S3ObjectMetadata::save_metadata_successful, this),
      std::bind(&S3ObjectMetadata::save_metadata_failed, this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

// Save to objects list index
void S3ObjectMetadata::save_metadata(std::function<void(void)> on_success,
                                     std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);
  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;
  save_metadata();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3ObjectMetadata::save_metadata_successful() {
  s3_log(S3_LOG_DEBUG, request_id, "Object metadata saved for Object [%s].\n",
         object_name.c_str());
  state = S3ObjectMetadataState::saved;
  this->handler_on_success();
}

void S3ObjectMetadata::save_metadata_failed() {
  s3_log(S3_LOG_ERROR, request_id,
         "Object metadata save failed for Object [%s].\n", object_name.c_str());
  if (motr_kv_writer->get_state() == S3MotrKVSWriterOpState::failed_to_launch) {
    state = S3ObjectMetadataState::failed_to_launch;
  } else {
    state = S3ObjectMetadataState::failed;
  }
  this->handler_on_failed();
}

void S3ObjectMetadata::remove(std::function<void(void)> on_success,
                              std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, request_id,
         "Deleting Object metadata for Object [%s].\n", object_name.c_str());
  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;
  // Remove Object metadata from object list followed by
  // removing version entry from version list.
  remove_object_metadata();
}

void S3ObjectMetadata::remove_object_metadata() {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);
  // object_list_index_oid should be set before using this method
  assert(object_list_index_oid.u_hi || object_list_index_oid.u_lo);

  motr_kv_writer =
      mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  motr_kv_writer->delete_keyval(
      object_list_index_oid, object_name,
      std::bind(&S3ObjectMetadata::remove_object_metadata_successful, this),
      std::bind(&S3ObjectMetadata::remove_object_metadata_failed, this));
}

void S3ObjectMetadata::remove_object_metadata_successful() {
  s3_log(S3_LOG_DEBUG, request_id, "Deleted metadata for Object [%s].\n",
         object_name.c_str());
  if (is_multipart) {
    // In multipart, version entry is not yet created.
    state = S3ObjectMetadataState::deleted;
    this->handler_on_success();
  } else {
    remove_version_metadata();
  }
}

void S3ObjectMetadata::remove_object_metadata_failed() {
  s3_log(S3_LOG_DEBUG, request_id,
         "Delete Object metadata failed for Object [%s].\n",
         object_name.c_str());
  if (motr_kv_writer->get_state() == S3MotrKVSWriterOpState::failed_to_launch) {
    state = S3ObjectMetadataState::failed_to_launch;
  } else {
    state = S3ObjectMetadataState::failed;
  }
  this->handler_on_failed();
}

void S3ObjectMetadata::remove_version_metadata(
    std::function<void(void)> on_success, std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, request_id,
         "Deleting Object Version metadata for Object [%s].\n",
         object_name.c_str());
  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;
  remove_version_metadata();
}

void S3ObjectMetadata::remove_version_metadata() {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry\n", __func__);
  // objects_version_list_index_oid should be set before using this method
  assert(objects_version_list_index_oid.u_hi ||
         objects_version_list_index_oid.u_lo);

  motr_kv_writer =
      mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  motr_kv_writer->delete_keyval(
      objects_version_list_index_oid, get_version_key_in_index(),
      std::bind(&S3ObjectMetadata::remove_version_metadata_successful, this),
      std::bind(&S3ObjectMetadata::remove_version_metadata_failed, this));
}

void S3ObjectMetadata::remove_version_metadata_successful() {
  s3_log(S3_LOG_DEBUG, request_id,
         "Deleted [%s] Version metadata for Object [%s].\n",
         object_version_id.c_str(), object_name.c_str());
  state = S3ObjectMetadataState::deleted;
  this->handler_on_success();
}

void S3ObjectMetadata::remove_version_metadata_failed() {
  s3_log(S3_LOG_DEBUG, request_id,
         "Delete Version [%s] metadata failed for Object [%s].\n",
         object_version_id.c_str(), object_name.c_str());
  // We still treat remove object metadata as successful as from S3 client
  // perspective object is gone. Version can be clean up by backgrounddelete
  state = S3ObjectMetadataState::deleted;
  this->handler_on_success();
}

// Streaming to json
std::string S3ObjectMetadata::to_json() {
  s3_log(S3_LOG_DEBUG, request_id, "Called\n");
  Json::Value root;
  root["Bucket-Name"] = bucket_name;
  if (s3_di_fi_is_enabled("di_metadata_bcktname_on_write_corrupted")) {
    root["Bucket-Name"] = "@" + bucket_name + "@";
  }
  root["Object-Name"] = object_name;
  if (s3_di_fi_is_enabled("di_metadata_objname_on_write_corrupted")) {
    root["Object-Name"] = "@" + object_name + "@";
  }
  root["Object-URI"] = object_key_uri;
  root["layout_id"] = layout_id;

  if (is_multipart) {
    root["Upload-ID"] = upload_id;
    root["motr_part_oid"] = motr_part_oid_str;
    root["motr_old_oid"] = motr_old_oid_str;
    root["old_layout_id"] = old_layout_id;
    root["motr_old_object_version_id"] = motr_old_object_version_id;
  }

  root["motr_oid"] = motr_oid_str;

  for (auto sit : system_defined_attribute) {
    root["System-Defined"][sit.first] = sit.second;
  }
  for (auto uit : user_defined_attribute) {
    root["User-Defined"][uit.first] = uit.second;
  }
  for (const auto& tag : object_tags) {
    root["User-Defined-Tags"][tag.first] = tag.second;
  }
  if (encoded_acl == "") {

    root["ACL"] = request->get_default_acl();

  } else {
    root["ACL"] = encoded_acl;
  }

  S3DateTime current_time;
  current_time.init_current_time();
  root["create_timestamp"] = current_time.get_isoformat_string();

  if (s3_di_fi_is_enabled("di_obj_md5_corrupted")) {
    // MD5 of empty string - md5("")
    root["System-Defined"]["Content-MD5"] = "d41d8cd98f00b204e9800998ecf8427e";
  }

  Json::FastWriter fastWriter;
  return fastWriter.write(root);
  ;
}

// Streaming to json
std::string S3ObjectMetadata::version_entry_to_json() {
  s3_log(S3_LOG_DEBUG, request_id, "Called\n");
  Json::Value root;
  // Processing version entry currently only needs minimal information
  // In future when real S3 versioning is supported, this method will not be
  // required and we can simply use to_json.

  // root["Object-Name"] = object_name;
  root["motr_oid"] = motr_oid_str;
  root["layout_id"] = layout_id;

  S3DateTime current_time;
  current_time.init_current_time();
  root["create_timestamp"] = current_time.get_isoformat_string();

  Json::FastWriter fastWriter;
  return fastWriter.write(root);
  ;
}

/*
 *  <IEM_INLINE_DOCUMENTATION>
 *    <event_code>047006003</event_code>
 *    <application>S3 Server</application>
 *    <submodule>S3 Actions</submodule>
 *    <description>Metadata corrupted causing Json parsing failure</description>
 *    <audience>Development</audience>
 *    <details>
 *      Json parsing failed due to metadata corruption.
 *      The data section of the event has following keys:
 *        time - timestamp.
 *        node - node name.
 *        pid  - process-id of s3server instance, useful to identify logfile.
 *        file - source code filename.
 *        line - line number within file where error occurred.
 *    </details>
 *    <service_actions>
 *      Save the S3 server log files.
 *      Contact development team for further investigation.
 *    </service_actions>
 *  </IEM_INLINE_DOCUMENTATION>
 */

int S3ObjectMetadata::from_json(std::string content) {
  s3_log(S3_LOG_DEBUG, request_id, "Called with content [%s]\n",
         content.c_str());
  Json::Value newroot;
  Json::Reader reader;
  bool parsingSuccessful = reader.parse(content.c_str(), newroot);
  if (!parsingSuccessful || s3_di_fi_is_enabled("object_metadata_corrupted")) {
    s3_log(S3_LOG_ERROR, request_id, "Json Parsing failed\n");
    return -1;
  }

  bucket_name = newroot["Bucket-Name"].asString();
  if (s3_di_fi_is_enabled("di_metadata_bcktname_on_read_corrupted")) {
    bucket_name = "@" + bucket_name + "@";
  }
  object_name = newroot["Object-Name"].asString();
  if (s3_di_fi_is_enabled("di_metadata_objname_on_read_corrupted")) {
    object_name = "@" + object_name + "@";
  }
  object_key_uri = newroot["Object-URI"].asString();
  upload_id = newroot["Upload-ID"].asString();
  motr_part_oid_str = newroot["motr_part_oid"].asString();

  motr_oid_str = newroot["motr_oid"].asString();
  layout_id = newroot["layout_id"].asInt();

  oid = S3M0Uint128Helper::to_m0_uint128(motr_oid_str);

  //
  // Old oid is needed to remove the OID when the object already exists
  // during upload, this is needed in case of multipart upload only
  // As upload happens in multiple sessions, so this old oid
  // will be used in post complete action.
  //
  if (is_multipart) {
    motr_old_oid_str = newroot["motr_old_oid"].asString();
    old_oid = S3M0Uint128Helper::to_m0_uint128(motr_old_oid_str);
    old_layout_id = newroot["old_layout_id"].asInt();
    motr_old_object_version_id =
        newroot["motr_old_object_version_id"].asString();
  }

  part_index_oid = S3M0Uint128Helper::to_m0_uint128(motr_part_oid_str);

  Json::Value::Members members = newroot["System-Defined"].getMemberNames();
  for (auto it : members) {
    system_defined_attribute[it.c_str()] =
        newroot["System-Defined"][it].asString().c_str();
  }
  user_name = system_defined_attribute["Owner-User"];
  canonical_id = system_defined_attribute["Owner-Canonical-id"];
  user_id = system_defined_attribute["Owner-User-id"];
  account_name = system_defined_attribute["Owner-Account"];
  account_id = system_defined_attribute["Owner-Account-id"];
  object_version_id = system_defined_attribute["x-amz-version-id"];
  rev_epoch_version_id_key =
      S3ObjectVersioingHelper::generate_keyid_from_versionid(object_version_id);

  members = newroot["User-Defined"].getMemberNames();
  for (auto it : members) {
    user_defined_attribute[it.c_str()] =
        newroot["User-Defined"][it].asString().c_str();
  }
  members = newroot["User-Defined-Tags"].getMemberNames();
  for (const auto& tag : members) {
    object_tags[tag] = newroot["User-Defined-Tags"][tag].asString();
  }
  acl_from_json(newroot["ACL"].asString());

  return 0;
}

void S3ObjectMetadata::acl_from_json(std::string acl_json_str) {
  s3_log(S3_LOG_DEBUG, "", "Called\n");
  encoded_acl = acl_json_str;
}

std::string& S3ObjectMetadata::get_encoded_object_acl() {
  // base64 encoded acl
  return encoded_acl;
}

void S3ObjectMetadata::setacl(const std::string& input_acl) {
  encoded_acl = input_acl;
}

std::string S3ObjectMetadata::get_acl_as_xml() {
  return base64_decode(encoded_acl);
}

void S3ObjectMetadata::set_tags(
    const std::map<std::string, std::string>& tags_as_map) {
  object_tags = tags_as_map;
}

const std::map<std::string, std::string>& S3ObjectMetadata::get_tags() {
  return object_tags;
}

void S3ObjectMetadata::delete_object_tags() { object_tags.clear(); }

std::string S3ObjectMetadata::get_tags_as_xml() {

  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  std::string user_defined_tags;
  std::string tags_as_xml_str;

  for (const auto& tag : object_tags) {
    user_defined_tags +=
        "<Tag>" + S3CommonUtilities::format_xml_string("Key", tag.first) +
        S3CommonUtilities::format_xml_string("Value", tag.second) + "</Tag>";
  }

  tags_as_xml_str =
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
      "<Tagging xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">"
      "<TagSet>" +
      user_defined_tags +
      "</TagSet>"
      "</Tagging>";
  s3_log(S3_LOG_DEBUG, request_id, "Tags xml: %s\n", tags_as_xml_str.c_str());
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Exit", __func__);
  return tags_as_xml_str;
}

bool S3ObjectMetadata::check_object_tags_exists() {
  return !object_tags.empty();
}

int S3ObjectMetadata::object_tags_count() { return object_tags.size(); }
