
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
  system_defined_attribute["x-amz-server-side​-encryption​-customer-key"] = "";
  system_defined_attribute["x-amz-server-side​-encryption​-customer-key-MD5"] = "";
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
  clovis_kv_reader->get_keyval(bucket_name, object_name, std::bind( &S3ObjectMetadata::load_successful, this), std::bind( &S3ObjectMetadata::load_failed, this));
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
  state = S3ObjectMetadataState::absent;  // todo Check error
  this->handler_on_failed();
}

void S3ObjectMetadata::save(std::function<void(void)> on_success, std::function<void(void)> on_failed) {
  printf("Called S3ObjectMetadata::save\n");

  this->handler_on_success = on_success;
  this->handler_on_failed  = on_failed;

  // Set up system attributes
  system_defined_attribute["Owner-User"] = user_name;
  system_defined_attribute["Owner-User-id"] = user_id;
  system_defined_attribute["Owner-Account"] = account_name;
  system_defined_attribute["Owner-Account-id"] = account_id;

  clovis_kv_writer = std::make_shared<S3ClovisKVSWriter>(request);
  clovis_kv_writer->put_keyval(bucket_name, object_name, this->to_json(), std::bind( &S3ObjectMetadata::save_successful, this), std::bind( &S3ObjectMetadata::save_failed, this));
}

void S3ObjectMetadata::save_successful() {
  printf("Called S3ObjectMetadata::save_successful\n");
  state = S3ObjectMetadataState::saved;
  this->handler_on_success();
}

void S3ObjectMetadata::save_failed() {
  // TODO - do anything more for failure?
  printf("Called S3ObjectMetadata::save_failed\n");
  state = S3ObjectMetadataState::failed;
  this->handler_on_failed();
}

// Streaming to json
std::string S3ObjectMetadata::to_json() {
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
    printf("System-Defined [%s] = %s\n", it.c_str(), newroot["System-Defined"][it].asString().c_str());
  }
  members = newroot["User-Defined"].getMemberNames();
  for(auto it : members) {
    user_defined_attribute[it.c_str()] = newroot["User-Defined"][it].asString().c_str();
    printf("User-Defined [%s] = %s\n", it.c_str(), newroot["User-Defined"][it].asString().c_str());
  }
  object_ACL.from_json(newroot["ACL"].asString());
}
