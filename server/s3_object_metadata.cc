
#include <stdlib.h>
#include <json/json.h>

#include "s3_object_metadata.h"

S3ObjectMetadata::S3ObjectMetadata(std::shared_ptr<S3RequestObject> req) : request(req) {
  account_name = request->get_account_name();
  account_id = request->get_account_id();
  user_name = request->get_user_name();
  user_id = request->get_user_id();
  bucket_name = request->get_bucket_name();
  object_name = request->get_object_name();
  state = S3ObjectMetadataState::empty;

  object_key_uri = bucket_name + "\\" + object_name;

  // Set the defaults
  system_defined_attribute["Date"] = "currentdate";  // TODO
  system_defined_attribute["Content-Length"] = "";
  system_defined_attribute["Last-Modified"] = "currentdate";  // TODO
  system_defined_attribute["Content-MD5"] = "";
  system_defined_attribute["Owner-User"] = "";
  system_defined_attribute["Owner-User-id"] = "";
  system_defined_attribute["Owner-Account"] = "";
  system_defined_attribute["Owner-Account-id"] = "";

  system_defined_attribute["x-amz-server-side-encryption"] = "None"; // Valid values aws:kms, AES256
  system_defined_attribute["x-amz-version-id"] = "";  // Generate if versioning enabled
  system_defined_attribute["x-amz-storage-class"] = "STANDARD";  // Valid Values: STANDARD | STANDARD_IA | REDUCED_REDUNDANCY
  system_defined_attribute["x-amz-website-redirect-location"] = "None";
  system_defined_attribute["x-amz-server-side-encryption-aws-kms-key-id"] = "";
  system_defined_attribute["x-amz-server-side-encryption-customer-algorithm"] = "";
  system_defined_attribute["x-amz-server-side-encryption-customer-key"] = "";
  system_defined_attribute["x-amz-server-side-encryption-customer-key-MD5"] = "";
}

void S3ObjectMetadata::set_content_length(std::string length) {
  system_defined_attribute["Content-Length"] = length;
}

size_t S3ObjectMetadata::get_content_length() {
  return atol(system_defined_attribute["Content-Length"].c_str());
}

std::string S3ObjectMetadata::get_content_length_str() {
  return system_defined_attribute["Content-Length"];
}

void S3ObjectMetadata::set_md5(std::string md5) {
  system_defined_attribute["Content-MD5"] = md5;
}

std::string S3ObjectMetadata::get_md5() {
  return system_defined_attribute["Content-MD5"];
}

void S3ObjectMetadata::add_system_attribute(std::string key, std::string val) {
  system_defined_attribute[key] = val;
}

void S3ObjectMetadata::add_user_defined_attribute(std::string key, std::string val) {
  user_defined_attribute[key] = val;
}

void S3ObjectMetadata::validate() {
  // TODO
}

void S3ObjectMetadata::load(std::function<void(void)> on_success, std::function<void(void)> on_failed) {
  printf("Called S3ObjectMetadata::load\n");

  this->handler_on_success = on_success;
  this->handler_on_failed  = on_failed;

  clovis_kv_reader = std::make_shared<S3ClovisKVSReader>(request);
  clovis_kv_reader->get_keyval(get_bucket_index_name(), object_name, std::bind( &S3ObjectMetadata::load_successful, this), std::bind( &S3ObjectMetadata::load_failed, this));
}

void S3ObjectMetadata::load_successful() {
  printf("Called S3ObjectMetadata::load_successful\n");
  this->from_json(clovis_kv_reader->get_value());
  state = S3ObjectMetadataState::present;
  this->handler_on_success();
}

void S3ObjectMetadata::load_failed() {
  // TODO - do anything more for failure?
  printf("Called S3ObjectMetadata::load_failed\n");
  if (clovis_kv_reader->get_state() == S3ClovisKVSReaderOpState::missing) {
    state = S3ObjectMetadataState::missing;  // Missing
  } else {
    state = S3ObjectMetadataState::failed;
  }
  this->handler_on_failed();
}

void S3ObjectMetadata::save(std::function<void(void)> on_success, std::function<void(void)> on_failed) {
  printf("Called S3ObjectMetadata::save\n");

  this->handler_on_success = on_success;
  this->handler_on_failed  = on_failed;

  create_bucket_index();
}

void S3ObjectMetadata::create_bucket_index() {
  printf("Called S3ObjectMetadata::create_bucket_index\n");
  // Mark missing as we initiate write, in case it fails to write.
  state = S3ObjectMetadataState::missing;

  clovis_kv_writer = std::make_shared<S3ClovisKVSWriter>(request);
  clovis_kv_writer->create_index(get_bucket_index_name(), std::bind( &S3ObjectMetadata::create_bucket_index_successful, this), std::bind( &S3ObjectMetadata::create_bucket_index_failed, this));
}

void S3ObjectMetadata::create_bucket_index_successful() {
  printf("Called S3ObjectMetadata::create_bucket_index_successful\n");
  save_metadata();
}

void S3ObjectMetadata::create_bucket_index_failed() {
  printf("Called S3ObjectMetadata::create_bucket_index_failed\n");
  if (clovis_kv_writer->get_state() == S3ClovisKVSWriterOpState::exists) {
    // We need to create index only once.
    save_metadata();
  } else {
    state = S3ObjectMetadataState::failed;  // todo Check error
    this->handler_on_failed();
  }
}

void S3ObjectMetadata::save_metadata() {
  // Set up system attributes
  system_defined_attribute["Owner-User"] = user_name;
  system_defined_attribute["Owner-User-id"] = user_id;
  system_defined_attribute["Owner-Account"] = account_name;
  system_defined_attribute["Owner-Account-id"] = account_id;

  clovis_kv_writer = std::make_shared<S3ClovisKVSWriter>(request);
  clovis_kv_writer->put_keyval(get_bucket_index_name(), object_name, this->to_json(), std::bind( &S3ObjectMetadata::save_metadata_successful, this), std::bind( &S3ObjectMetadata::save_metadata_failed, this));
}

void S3ObjectMetadata::save_metadata_successful() {
  printf("Called S3ObjectMetadata::save_metadata_successful\n");
  state = S3ObjectMetadataState::saved;
  this->handler_on_success();
}

void S3ObjectMetadata::save_metadata_failed() {
  // TODO - do anything more for failure?
  printf("Called S3ObjectMetadata::save_metadata_failed\n");
  state = S3ObjectMetadataState::failed;
  this->handler_on_failed();
}

void S3ObjectMetadata::remove(std::function<void(void)> on_success, std::function<void(void)> on_failed) {
  printf("Called S3ObjectMetadata::remove\n");

  this->handler_on_success = on_success;
  this->handler_on_failed  = on_failed;

  clovis_kv_writer = std::make_shared<S3ClovisKVSWriter>(request);
  clovis_kv_writer->delete_keyval(get_bucket_index_name(), object_name, std::bind( &S3ObjectMetadata::remove_successful, this), std::bind( &S3ObjectMetadata::remove_failed, this));
}

void S3ObjectMetadata::remove_successful() {
  printf("Called S3ObjectMetadata::remove_successful\n");
  state = S3ObjectMetadataState::deleted;
  this->handler_on_success();
}

void S3ObjectMetadata::remove_failed() {
  printf("Called S3ObjectMetadata::remove_failed\n");
  state = S3ObjectMetadataState::failed;
  this->handler_on_failed();
}

// Streaming to json
std::string S3ObjectMetadata::to_json() {
  printf("Called S3ObjectMetadata::to_json\n");
  Json::Value root;
  root["Bucket-Name"] = bucket_name;
  root["Object-Name"] = object_name;
  root["Object-URI"] = object_key_uri;

  for (auto sit: system_defined_attribute) {
    root["System-Defined"][sit.first] = sit.second;
  }
  for (auto uit: user_defined_attribute) {
    root["User-Defined"][uit.first] = uit.second;
  }
  root["ACL"] = object_ACL.to_json();

  Json::FastWriter fastWriter;
  return fastWriter.write(root);;
}

void S3ObjectMetadata::from_json(std::string content) {
  printf("Called S3ObjectMetadata::from_json\n");
  Json::Value newroot;
  Json::Reader reader;
  bool parsingSuccessful = reader.parse(content.c_str(), newroot);
  if (!parsingSuccessful)
  {
    printf("Json Parsing failed.\n");
    return;
  }

  Json::Value::Members members = newroot["System-Defined"].getMemberNames();
  for(auto it : members) {
    system_defined_attribute[it.c_str()] = newroot["System-Defined"][it].asString().c_str();
  }
  members = newroot["User-Defined"].getMemberNames();
  for(auto it : members) {
    user_defined_attribute[it.c_str()] = newroot["User-Defined"][it].asString().c_str();
  }
  object_ACL.from_json(newroot["ACL"].asString());
}
