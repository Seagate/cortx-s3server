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

#include <json/json.h>

#include "s3_bucket_metadata.h"
#include "s3_datetime.h"

S3BucketMetadata::S3BucketMetadata(std::shared_ptr<S3RequestObject> req) : request(req) {
  s3_log(S3_LOG_DEBUG, "Constructor");
  account_name = request->get_account_name();
  user_name = request->get_user_name();
  bucket_name = request->get_bucket_name();
  state = S3BucketMetadataState::empty;
  s3_clovis_api = std::make_shared<ConcreteClovisAPI>();

  // Set the defaults
  S3DateTime current_time;
  current_time.init_current_time();
  system_defined_attribute["Date"] = current_time.get_isoformat_string();
  system_defined_attribute["LocationConstraint"] = "US";
  system_defined_attribute["Owner-User"] = "";
  system_defined_attribute["Owner-User-id"] = "";
  system_defined_attribute["Owner-Account"] = "";
  system_defined_attribute["Owner-Account-id"] = "";
}

std::string S3BucketMetadata::get_bucket_name() {
  return bucket_name;
}

std::string S3BucketMetadata::get_creation_time() {
  return system_defined_attribute["Date"];
}

std::string S3BucketMetadata::get_location_constraint() {
  return system_defined_attribute["LocationConstraint"];
}

std::string S3BucketMetadata::get_owner_id() {
  return system_defined_attribute["Owner-User"];
}

std::string S3BucketMetadata::get_owner_name() {
  return system_defined_attribute["Owner-User-id"];
}

void S3BucketMetadata::set_location_constraint(std::string location) {
  system_defined_attribute["LocationConstraint"] = location;
}

void S3BucketMetadata::add_system_attribute(std::string key, std::string val) {
  system_defined_attribute[key] = val;
}

void S3BucketMetadata::add_user_defined_attribute(std::string key, std::string val) {
  user_defined_attribute[key] = val;
}

// AWS recommends that all bucket names comply with DNS naming convention
// See Bucket naming restrictions in above link.
void S3BucketMetadata::validate_bucket_name() {
  // TODO
}

void S3BucketMetadata::validate() {
  // TODO
}

void S3BucketMetadata::load(std::function<void(void)> on_success, std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  this->handler_on_success = on_success;
  this->handler_on_failed  = on_failed;

  load_account_bucket();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3BucketMetadata::load_account_bucket() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  // Mark missing as we initiate fetch, in case it fails to load due to missing.
  state = S3BucketMetadataState::missing;

  clovis_kv_reader = std::make_shared<S3ClovisKVSReader>(request);
  clovis_kv_reader->get_keyval(get_account_index_name(), bucket_name, std::bind( &S3BucketMetadata::load_account_bucket_successful, this), std::bind( &S3BucketMetadata::load_account_bucket_failed, this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3BucketMetadata::load_account_bucket_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  this->from_json(clovis_kv_reader->get_value());
  state = S3BucketMetadataState::present;
  this->handler_on_success();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3BucketMetadata::load_account_bucket_failed() {
  // TODO - do anything more for failure?
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (clovis_kv_reader->get_state() == S3ClovisKVSReaderOpState::missing) {
    s3_log(S3_LOG_DEBUG, "Account bucket metadata is missing\n");
    state = S3BucketMetadataState::missing;
  } else {
    s3_log(S3_LOG_ERROR, "Loading of account bucket metadata failed\n");
    state = S3BucketMetadataState::failed;
  }
  this->handler_on_failed();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3BucketMetadata::load_user_bucket() {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  // Mark missing as we initiate fetch, in case it fails to load due to missing.
  state = S3BucketMetadataState::missing;
  clovis_kv_reader = std::make_shared<S3ClovisKVSReader>(request);
  clovis_kv_reader->get_keyval(get_account_user_index_name(), bucket_name, std::bind( &S3BucketMetadata::load_user_bucket_successful, this), std::bind( &S3BucketMetadata::load_user_bucket_failed, this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3BucketMetadata::load_user_bucket_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  this->from_json(clovis_kv_reader->get_value());
  state = S3BucketMetadataState::present;
  this->handler_on_success();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3BucketMetadata::load_user_bucket_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (clovis_kv_reader->get_state() == S3ClovisKVSReaderOpState::missing) {
    s3_log(S3_LOG_DEBUG, "User bucket metadata missing\n");
    state = S3BucketMetadataState::missing;
  } else {
    s3_log(S3_LOG_ERROR, "Loading of user bucket metadata failed\n");
    state = S3BucketMetadataState::failed;
  }
  this->handler_on_failed();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3BucketMetadata::save(std::function<void(void)> on_success, std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  this->handler_on_success = on_success;
  this->handler_on_failed  = on_failed;

  // TODO create only if it does not exists.
  create_account_bucket_index();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3BucketMetadata::create_account_bucket_index() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  // Mark missing as we initiate write, in case it fails to write.
  state = S3BucketMetadataState::missing;
  clovis_kv_writer = std::make_shared<S3ClovisKVSWriter>(request, s3_clovis_api);
  clovis_kv_writer->create_index(get_account_index_name(), std::bind( &S3BucketMetadata::create_account_bucket_index_successful, this), std::bind( &S3BucketMetadata::create_account_bucket_index_failed, this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3BucketMetadata::create_account_bucket_index_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  create_account_user_bucket_index();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3BucketMetadata::create_account_bucket_index_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (clovis_kv_writer->get_state() == S3ClovisKVSWriterOpState::exists) {
    s3_log(S3_LOG_DEBUG, "Account bucket index already exists\n");
    // We need to create index only once.
    create_account_user_bucket_index();
  } else {
    s3_log(S3_LOG_ERROR, "Index creation failed\n");
    state = S3BucketMetadataState::failed;
    this->handler_on_failed();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3BucketMetadata::create_account_user_bucket_index() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  // Mark missing as we initiate write, in case it fails to write.
  state = S3BucketMetadataState::missing;

  clovis_kv_writer = std::make_shared<S3ClovisKVSWriter>(request, s3_clovis_api);
  clovis_kv_writer->create_index(get_account_user_index_name(), std::bind( &S3BucketMetadata::create_account_user_bucket_index_successful, this), std::bind( &S3BucketMetadata::create_account_user_bucket_index_failed, this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3BucketMetadata::create_account_user_bucket_index_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  save_account_bucket();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3BucketMetadata::create_account_user_bucket_index_failed() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  if (clovis_kv_writer->get_state() == S3ClovisKVSWriterOpState::exists) {
    s3_log(S3_LOG_DEBUG, "Account user bucket index already exists\n");
    // We need to create index only once.
    save_account_bucket();
  } else {
    s3_log(S3_LOG_ERROR, "Creation of Account user bucket index failed\n");
    state = S3BucketMetadataState::failed;
    this->handler_on_failed();
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3BucketMetadata::save_account_bucket() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  // Mark missing as we initiate write, in case it fails to write.
  state = S3BucketMetadataState::missing;

  // Set up system attributes
  system_defined_attribute["Owner-User"] = user_name;
  system_defined_attribute["Owner-User-id"] = request->get_user_id();
  system_defined_attribute["Owner-Account"] = account_name;
  system_defined_attribute["Owner-Account-id"] = request->get_account_id();

  clovis_kv_writer = std::make_shared<S3ClovisKVSWriter>(request, s3_clovis_api);
  clovis_kv_writer->put_keyval(get_account_index_name(), bucket_name, this->to_json(), std::bind( &S3BucketMetadata::save_account_bucket_successful, this), std::bind( &S3BucketMetadata::save_account_bucket_failed, this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3BucketMetadata::save_account_bucket_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  save_user_bucket();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3BucketMetadata::save_account_bucket_failed() {
  // TODO - do anything more for failure?
  s3_log(S3_LOG_DEBUG, "Entering\n");
  s3_log(S3_LOG_ERROR, "Saving of account bucket metadata failed\n");
  state = S3BucketMetadataState::failed;
  this->handler_on_failed();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3BucketMetadata::save_user_bucket() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  // Mark missing as we initiate write, in case it fails to write.
  state = S3BucketMetadataState::missing;

  clovis_kv_writer = std::make_shared<S3ClovisKVSWriter>(request, s3_clovis_api);
  clovis_kv_writer->put_keyval(get_account_user_index_name(), bucket_name, this->to_json(), std::bind( &S3BucketMetadata::save_user_bucket_successful, this), std::bind( &S3BucketMetadata::save_user_bucket_failed, this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3BucketMetadata::save_user_bucket_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  state = S3BucketMetadataState::saved;
  this->handler_on_success();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3BucketMetadata::save_user_bucket_failed() {
  // TODO - do anything more for failure?
  s3_log(S3_LOG_DEBUG, "Entering\n");
  s3_log(S3_LOG_ERROR, "Saving of user bucket metadata failed\n");
  state = S3BucketMetadataState::failed;
  this->handler_on_failed();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3BucketMetadata::remove(std::function<void(void)> on_success, std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  this->handler_on_success = on_success;
  this->handler_on_failed  = on_failed;

  remove_account_bucket();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3BucketMetadata::remove_account_bucket() {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  clovis_kv_writer = std::make_shared<S3ClovisKVSWriter>(request, s3_clovis_api);
  clovis_kv_writer->delete_keyval(get_account_index_name(), bucket_name, std::bind( &S3BucketMetadata::remove_account_bucket_successful, this), std::bind( &S3BucketMetadata::remove_account_bucket_failed, this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3BucketMetadata::remove_account_bucket_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  remove_user_bucket();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3BucketMetadata::remove_account_bucket_failed() {
  // TODO - do anything more for failure?
  s3_log(S3_LOG_DEBUG, "Entering\n");
  s3_log(S3_LOG_ERROR, "Removal of account bucket metadata failed\n");
  state = S3BucketMetadataState::failed;
  this->handler_on_failed();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3BucketMetadata::remove_user_bucket() {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  clovis_kv_writer = std::make_shared<S3ClovisKVSWriter>(request, s3_clovis_api);
  clovis_kv_writer->delete_keyval(get_account_user_index_name(), bucket_name, std::bind( &S3BucketMetadata::remove_user_bucket_successful, this), std::bind( &S3BucketMetadata::remove_user_bucket_failed, this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3BucketMetadata::remove_user_bucket_successful() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  state = S3BucketMetadataState::deleted;
  this->handler_on_success();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3BucketMetadata::remove_user_bucket_failed() {
  // TODO - do anything more for failure?
  s3_log(S3_LOG_DEBUG, "Entering\n");
  s3_log(S3_LOG_ERROR, "Removal of user bucket metadata failed\n");
  state = S3BucketMetadataState::failed;
  this->handler_on_failed();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

// Streaming to json
std::string S3BucketMetadata::to_json() {
  s3_log(S3_LOG_DEBUG, "Called\n");
  Json::Value root;
  root["Bucket-Name"] = bucket_name;

  for (auto sit: system_defined_attribute) {
    root["System-Defined"][sit.first] = sit.second;
  }
  for (auto uit: user_defined_attribute) {
    root["User-Defined"][uit.first] = uit.second;
  }
  root["ACL"] = bucket_ACL.to_json();

  Json::FastWriter fastWriter;
  return fastWriter.write(root);;
}

void S3BucketMetadata::from_json(std::string content) {
  s3_log(S3_LOG_DEBUG, "Called\n");
  Json::Value newroot;
  Json::Reader reader;
  bool parsingSuccessful = reader.parse(content.c_str(), newroot);
  if (!parsingSuccessful)
  {
    s3_log(S3_LOG_ERROR, "Json Parsing failed.\n");
    return;
  }

  bucket_name = newroot["Bucket-Name"].asString();

  Json::Value::Members members = newroot["System-Defined"].getMemberNames();
  for(auto it : members) {
    system_defined_attribute[it.c_str()] = newroot["System-Defined"][it].asString();
  }
  members = newroot["User-Defined"].getMemberNames();
  for(auto it : members) {
    user_defined_attribute[it.c_str()] = newroot["User-Defined"][it].asString();
  }
  user_name = system_defined_attribute["Owner-User"];
  account_name = system_defined_attribute["Owner-Account"];

  bucket_ACL.from_json(newroot["ACL"].asString());
}
