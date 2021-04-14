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

#include "base64.h"
#include "s3_bucket_metadata.h"
#include "s3_common_utilities.h"
#include "s3_datetime.h"
#include "s3_factory.h"
#include "s3_iem.h"
#include "s3_log.h"
#include "s3_m0_uint128_helper.h"
#include "s3_request_object.h"
#include "s3_uri_to_motr_oid.h"

S3BucketMetadata::S3BucketMetadata(std::shared_ptr<S3RequestObject> req,
                                   const std::string& str_bucket_name)
    : request(std::move(req)) {

  request_id = request->get_request_id();
  stripped_request_id = request->get_stripped_request_id();

  s3_log(S3_LOG_DEBUG, request_id, "%s Ctor\n", __func__);

  if (str_bucket_name.empty()) {
    bucket_name = request->get_bucket_name();
  } else {
    bucket_name = str_bucket_name;
  }
}

S3BucketMetadata::~S3BucketMetadata() = default;

const std::string& S3BucketMetadata::get_bucket_name() const {
  return bucket_name;
}

const std::string& S3BucketMetadata::get_creation_time() {
  return system_defined_attribute["Date"];
}

const std::string& S3BucketMetadata::get_location_constraint() {
  return system_defined_attribute["LocationConstraint"];
}

const std::string& S3BucketMetadata::get_owner_id() {
  return system_defined_attribute["Owner-User-id"];
}

const std::string& S3BucketMetadata::get_owner_name() {
  return system_defined_attribute["Owner-User"];
}

const std::string& S3BucketMetadata::get_bucket_owner_account_id() {
  return system_defined_attribute["Owner-Account-id"];
}

const std::string& S3BucketMetadata::get_owner_canonical_id() {
  return system_defined_attribute["Owner-Canonical-id"];
}

const struct m0_uint128& S3BucketMetadata::get_multipart_index_oid() const {
  return multipart_index_oid;
}

const struct m0_uint128& S3BucketMetadata::get_object_list_index_oid() const {
  return object_list_index_oid;
}

const struct m0_uint128& S3BucketMetadata::get_objects_version_list_index_oid()
    const {
  return objects_version_list_index_oid;
}

void S3BucketMetadata::set_multipart_index_oid(struct m0_uint128 oid) {
  multipart_index_oid = oid;
}

void S3BucketMetadata::set_object_list_index_oid(struct m0_uint128 oid) {
  object_list_index_oid = oid;
}

void S3BucketMetadata::set_objects_version_list_index_oid(
    struct m0_uint128 oid) {
  objects_version_list_index_oid = oid;
}

void S3BucketMetadata::set_location_constraint(std::string location) {
  system_defined_attribute["LocationConstraint"] = location;
}

void S3BucketMetadata::setpolicy(std::string policy_str) {
  bucket_policy = std::move(policy_str);
}

void S3BucketMetadata::set_tags(
    const std::map<std::string, std::string>& tags_as_map) {
  bucket_tags = tags_as_map;
}

// Streaming to json
std::string S3BucketMetadata::to_json() {
  s3_log(S3_LOG_DEBUG, request_id, "Called\n");
  Json::Value root;
  root["Bucket-Name"] = bucket_name;

  for (auto sit : system_defined_attribute) {
    root["System-Defined"][sit.first] = sit.second;
  }
  for (auto uit : user_defined_attribute) {
    root["User-Defined"][uit.first] = uit.second;
  }
  if (encoded_acl.empty()) {
    encoded_acl = request->get_default_acl();
  }
  root["ACL"] = encoded_acl;

  root["Policy"] = base64_encode((const unsigned char*)bucket_policy.c_str(),
                                 bucket_policy.size());

  for (const auto& tag : bucket_tags) {
    root["User-Defined-Tags"][tag.first] = tag.second;
  }

  root["motr_object_list_index_oid"] =
      S3M0Uint128Helper::to_string(object_list_index_oid);

  root["motr_multipart_index_oid"] =
      S3M0Uint128Helper::to_string(multipart_index_oid);

  root["motr_objects_version_list_index_oid"] =
      S3M0Uint128Helper::to_string(objects_version_list_index_oid);

  S3DateTime current_time;
  current_time.init_current_time();
  root["create_timestamp"] = current_time.get_isoformat_string();

  Json::FastWriter fastWriter;
  return fastWriter.write(root);
}

int S3BucketMetadata::from_json(std::string content) {
  s3_log(S3_LOG_DEBUG, request_id, "Called\n");
  Json::Value newroot;
  Json::Reader reader;
  bool parsingSuccessful = reader.parse(content.c_str(), newroot);
  if (!parsingSuccessful || s3_fi_is_enabled("bucket_metadata_corrupted")) {
    s3_log(S3_LOG_ERROR, request_id, "Json Parsing failed.\n");
    return -1;
  }

  bucket_name = newroot["Bucket-Name"].asString();

  Json::Value::Members members = newroot["System-Defined"].getMemberNames();
  for (auto it : members) {
    system_defined_attribute[it.c_str()] =
        newroot["System-Defined"][it].asString();
  }
  members = newroot["User-Defined"].getMemberNames();
  for (auto it : members) {
    user_defined_attribute[it.c_str()] = newroot["User-Defined"][it].asString();
  }
  user_name = system_defined_attribute["Owner-User"];
  user_id = system_defined_attribute["Owner-User-id"];
  account_name = system_defined_attribute["Owner-Account"];
  account_id = system_defined_attribute["Owner-Account-id"];
  owner_canonical_id = system_defined_attribute["Owner-Canonical-id"];

  object_list_index_oid = S3M0Uint128Helper::to_m0_uint128(
      newroot["motr_object_list_index_oid"].asString());

  multipart_index_oid = S3M0Uint128Helper::to_m0_uint128(
      newroot["motr_multipart_index_oid"].asString());

  objects_version_list_index_oid = S3M0Uint128Helper::to_m0_uint128(
      newroot["motr_objects_version_list_index_oid"].asString());

  acl_from_json((newroot["ACL"]).asString());
  bucket_policy = base64_decode(newroot["Policy"].asString());

  members = newroot["User-Defined-Tags"].getMemberNames();
  for (const auto& tag : members) {
    bucket_tags[tag] = newroot["User-Defined-Tags"][tag].asString();
  }
  return 0;
}

void S3BucketMetadata::acl_from_json(std::string acl_json_str) {
  s3_log(S3_LOG_DEBUG, "", "Called\n");
  encoded_acl = std::move(acl_json_str);
}

void S3BucketMetadata::deletepolicy() { bucket_policy = ""; }

void S3BucketMetadata::delete_bucket_tags() { bucket_tags.clear(); }

void S3BucketMetadata::setacl(const std::string& acl_str) {
  encoded_acl = acl_str;
}

void S3BucketMetadata::add_system_attribute(std::string key, std::string val) {
  system_defined_attribute[key] = val;
}

void S3BucketMetadata::add_user_defined_attribute(std::string key,
                                                  std::string val) {
  user_defined_attribute[key] = val;
}

const std::string& S3BucketMetadata::get_encoded_bucket_acl() const {
  // base64 acl encoded
  return encoded_acl;
}

std::string S3BucketMetadata::get_acl_as_xml() {
  return base64_decode(encoded_acl);
}

std::string& S3BucketMetadata::get_policy_as_json() { return bucket_policy; }

std::string S3BucketMetadata::get_tags_as_xml() {

  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  std::string user_defined_tags;
  std::string tags_as_xml_str;

  if (bucket_tags.empty()) {
    return tags_as_xml_str;
  } else {
    for (const auto& tag : bucket_tags) {
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
  }
  s3_log(S3_LOG_DEBUG, request_id, "Tags xml: %s\n", tags_as_xml_str.c_str());
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Exit", __func__);
  return tags_as_xml_str;
}

bool S3BucketMetadata::check_bucket_tags_exists() const {
  return !bucket_tags.empty();
}

void S3BucketMetadata::load(std::function<void(void)>,
                            std::function<void(void)>) {
  assert(0);
}

void S3BucketMetadata::save(std::function<void(void)>,
                            std::function<void(void)>) {
  assert(0);
}

void S3BucketMetadata::update(std::function<void(void)>,
                              std::function<void(void)>) {
  assert(0);
}

void S3BucketMetadata::remove(std::function<void(void)>,
                              std::function<void(void)>) {
  assert(0);
}
