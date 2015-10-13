
#include <json/json.h>

#include "s3_bucket_metadata.h"
#include "s3_datetime.h"

S3BucketMetadata::S3BucketMetadata(std::shared_ptr<S3RequestObject> req) : request(req) {
  account_name = request->get_account_name();
  user_name = request->get_user_name();
  bucket_name = request->get_bucket_name();
  state = S3BucketMetadataState::empty;

  // Set the defaults
  S3DateTime current_time;
  current_time.init_current_time();
  system_defined_attribute["Date"] = current_time.get_GMT_string();
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
  printf("Called S3BucketMetadata::load\n");

  this->handler_on_success = on_success;
  this->handler_on_failed  = on_failed;

  load_account_bucket();
}

void S3BucketMetadata::load_account_bucket() {
  printf("Called S3BucketMetadata::load_account_bucket\n");
  // Mark missing as we initiate fetch, in case it fails to load due to missing.
  state = S3BucketMetadataState::missing;

  clovis_kv_reader = std::make_shared<S3ClovisKVSReader>(request);
  clovis_kv_reader->get_keyval(get_account_index_name(), bucket_name, std::bind( &S3BucketMetadata::load_account_bucket_successful, this), std::bind( &S3BucketMetadata::load_account_bucket_failed, this));
}

void S3BucketMetadata::load_account_bucket_successful() {
  printf("Called S3BucketMetadata::load_account_bucket_successful\n");
  this->from_json(clovis_kv_reader->get_value());
  state = S3BucketMetadataState::present;
  this->handler_on_success();
}

void S3BucketMetadata::load_account_bucket_failed() {
  // TODO - do anything more for failure?
  printf("Called S3BucketMetadata::load_account_bucket_failed\n");
  if (clovis_kv_reader->get_state() == S3ClovisKVSReaderOpState::missing) {
    state = S3BucketMetadataState::missing;
  } else {
    state = S3BucketMetadataState::failed;
  }
  this->handler_on_failed();
}

void S3BucketMetadata::load_user_bucket() {
  printf("Called S3BucketMetadata::load_user_bucket\n");

  // Mark missing as we initiate fetch, in case it fails to load due to missing.
  state = S3BucketMetadataState::missing;

  clovis_kv_reader = std::make_shared<S3ClovisKVSReader>(request);
  clovis_kv_reader->get_keyval(get_account_user_index_name(), bucket_name, std::bind( &S3BucketMetadata::load_user_bucket_successful, this), std::bind( &S3BucketMetadata::load_user_bucket_failed, this));
}

void S3BucketMetadata::load_user_bucket_successful() {
  printf("Called S3BucketMetadata::load_user_bucket_successful\n");
  this->from_json(clovis_kv_reader->get_value());
  state = S3BucketMetadataState::present;
  this->handler_on_success();
}

void S3BucketMetadata::load_user_bucket_failed() {
  printf("Called S3BucketMetadata::load_user_bucket_failed\n");
  if (clovis_kv_reader->get_state() == S3ClovisKVSReaderOpState::missing) {
    state = S3BucketMetadataState::missing;
  } else {
    state = S3BucketMetadataState::failed;
  }
  this->handler_on_failed();
}

void S3BucketMetadata::save(std::function<void(void)> on_success, std::function<void(void)> on_failed) {
  printf("Called S3BucketMetadata::save\n");

  this->handler_on_success = on_success;
  this->handler_on_failed  = on_failed;

  // TODO create only if it does not exists.
  create_account_bucket_index();
}

void S3BucketMetadata::create_account_bucket_index() {
  printf("Called S3BucketMetadata::create_account_bucket_index\n");
  // Mark missing as we initiate write, in case it fails to write.
  state = S3BucketMetadataState::missing;

  clovis_kv_writer = std::make_shared<S3ClovisKVSWriter>(request);
  clovis_kv_writer->create_index(get_account_index_name(), std::bind( &S3BucketMetadata::create_account_bucket_index_successful, this), std::bind( &S3BucketMetadata::create_account_bucket_index_failed, this));
}

void S3BucketMetadata::create_account_bucket_index_successful() {
  printf("Called S3BucketMetadata::create_account_bucket_index_successful\n");
  create_account_user_bucket_index();
}

void S3BucketMetadata::create_account_bucket_index_failed() {
  printf("Called S3BucketMetadata::create_account_bucket_index_failed\n");
  if (clovis_kv_writer->get_state() == S3ClovisKVSWriterOpState::exists) {
    // We need to create index only once.
    create_account_user_bucket_index();
  } else {
    state = S3BucketMetadataState::failed;
    this->handler_on_failed();
  }
}

void S3BucketMetadata::create_account_user_bucket_index() {
  printf("Called S3BucketMetadata::create_account_user_bucket_index\n");
  // Mark missing as we initiate write, in case it fails to write.
  state = S3BucketMetadataState::missing;

  clovis_kv_writer = std::make_shared<S3ClovisKVSWriter>(request);
  clovis_kv_writer->create_index(get_account_user_index_name(), std::bind( &S3BucketMetadata::create_account_user_bucket_index_successful, this), std::bind( &S3BucketMetadata::create_account_user_bucket_index_failed, this));
}

void S3BucketMetadata::create_account_user_bucket_index_successful() {
  printf("Called S3BucketMetadata::create_account_user_bucket_index_successful\n");
  save_account_bucket();
}

void S3BucketMetadata::create_account_user_bucket_index_failed() {
  printf("Called S3BucketMetadata::create_account_user_bucket_index_failed\n");
  if (clovis_kv_writer->get_state() == S3ClovisKVSWriterOpState::exists) {
    // We need to create index only once.
    save_account_bucket();
  } else {
    state = S3BucketMetadataState::failed;
    this->handler_on_failed();
  }
}

void S3BucketMetadata::save_account_bucket() {
  printf("Called S3BucketMetadata::save_account_bucket\n");
  // Mark missing as we initiate write, in case it fails to write.
  state = S3BucketMetadataState::missing;

  // Set up system attributes
  system_defined_attribute["Owner-User"] = user_name;
  system_defined_attribute["Owner-User-id"] = request->get_user_id();
  system_defined_attribute["Owner-Account"] = account_name;
  system_defined_attribute["Owner-Account-id"] = request->get_account_id();

  clovis_kv_writer = std::make_shared<S3ClovisKVSWriter>(request);
  clovis_kv_writer->put_keyval(get_account_index_name(), bucket_name, this->to_json(), std::bind( &S3BucketMetadata::save_account_bucket_successful, this), std::bind( &S3BucketMetadata::save_account_bucket_failed, this));
}

void S3BucketMetadata::save_account_bucket_successful() {
  printf("Called S3BucketMetadata::save_account_bucket_successful\n");
  save_user_bucket();
}

void S3BucketMetadata::save_account_bucket_failed() {
  // TODO - do anything more for failure?
  printf("Called S3BucketMetadata::save_account_bucket_failed\n");
  state = S3BucketMetadataState::failed;
  this->handler_on_failed();
}

void S3BucketMetadata::save_user_bucket() {
  printf("Called S3BucketMetadata::save_user_bucket\n");
  // Mark missing as we initiate write, in case it fails to write.
  state = S3BucketMetadataState::missing;

  clovis_kv_writer = std::make_shared<S3ClovisKVSWriter>(request);
  clovis_kv_writer->put_keyval(get_account_user_index_name(), bucket_name, this->to_json(), std::bind( &S3BucketMetadata::save_user_bucket_successful, this), std::bind( &S3BucketMetadata::save_user_bucket_failed, this));
}

void S3BucketMetadata::save_user_bucket_successful() {
  printf("Called S3BucketMetadata::save_user_bucket_successful\n");
  state = S3BucketMetadataState::saved;
  this->handler_on_success();
}

void S3BucketMetadata::save_user_bucket_failed() {
  // TODO - do anything more for failure?
  printf("Called S3BucketMetadata::save_user_bucket_failed\n");
  state = S3BucketMetadataState::failed;
  this->handler_on_failed();
}

void S3BucketMetadata::remove(std::function<void(void)> on_success, std::function<void(void)> on_failed) {
  printf("Called S3BucketMetadata::remove\n");

  this->handler_on_success = on_success;
  this->handler_on_failed  = on_failed;

  remove_account_bucket();
}

void S3BucketMetadata::remove_account_bucket() {
  printf("Called S3BucketMetadata::remove_account_bucket\n");

  clovis_kv_writer = std::make_shared<S3ClovisKVSWriter>(request);
  clovis_kv_writer->delete_keyval(get_account_index_name(), bucket_name, std::bind( &S3BucketMetadata::remove_account_bucket_successful, this), std::bind( &S3BucketMetadata::remove_account_bucket_failed, this));
}

void S3BucketMetadata::remove_account_bucket_successful() {
  printf("Called S3BucketMetadata::remove_account_bucket_successful\n");
  remove_user_bucket();
}

void S3BucketMetadata::remove_account_bucket_failed() {
  // TODO - do anything more for failure?
  printf("Called S3BucketMetadata::remove_account_bucket_failed\n");
  state = S3BucketMetadataState::failed;
  this->handler_on_failed();
}

void S3BucketMetadata::remove_user_bucket() {
  printf("Called S3BucketMetadata::remove_user_bucket\n");

  clovis_kv_writer = std::make_shared<S3ClovisKVSWriter>(request);
  clovis_kv_writer->delete_keyval(get_account_user_index_name(), bucket_name, std::bind( &S3BucketMetadata::remove_user_bucket_successful, this), std::bind( &S3BucketMetadata::remove_user_bucket_failed, this));
}

void S3BucketMetadata::remove_user_bucket_successful() {
  printf("Called S3BucketMetadata::remove_user_bucket_successful\n");
  state = S3BucketMetadataState::deleted;
  this->handler_on_success();
}

void S3BucketMetadata::remove_user_bucket_failed() {
  // TODO - do anything more for failure?
  printf("Called S3BucketMetadata::remove_user_bucket_failed\n");
  state = S3BucketMetadataState::failed;
  this->handler_on_failed();
}

// Streaming to json
std::string S3BucketMetadata::to_json() {
  printf("Called S3BucketMetadata::to_json\n");
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
  printf("Called S3BucketMetadata::from_json\n");
  Json::Value newroot;
  Json::Reader reader;
  bool parsingSuccessful = reader.parse(content.c_str(), newroot);
  if (!parsingSuccessful)
  {
    printf("Json Parsing failed.\n");
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
