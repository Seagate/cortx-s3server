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
 * Original author:  Rajesh Nambiar  <rajesh.nambiar@seagate.com>
 * Original creation date: 21-Jan-2016
 */

#include <stdlib.h>
#include <json/json.h>

#include "s3_part_metadata.h"
#include "s3_datetime.h"

S3PartMetadata::S3PartMetadata(std::shared_ptr<S3RequestObject> req, std::string uploadid, int part) : request(req) {
  bucket_name = request->get_bucket_name();
  object_name = request->get_object_name();
  state = S3PartMetadataState::empty;
  upload_id = uploadid;
  part_number = std::to_string(part);
  put_metadata = true;

  s3_clovis_api = std::make_shared<ConcreteClovisAPI>();

  // Set the defaults
  S3DateTime current_time;
  current_time.init_current_time();
  system_defined_attribute["Date"] = current_time.get_isoformat_string();  system_defined_attribute["Content-Length"] = "";
  system_defined_attribute["Last-Modified"] = current_time.get_isoformat_string();  // TODO
  system_defined_attribute["Content-MD5"] = "";

  system_defined_attribute["x-amz-server-side-encryption"] = "None"; // Valid values aws:kms, AES256
  system_defined_attribute["x-amz-version-id"] = "";  // Generate if versioning enabled
  system_defined_attribute["x-amz-storage-class"] = "STANDARD";  // Valid Values: STANDARD | STANDARD_IA | REDUCED_REDUNDANCY
  system_defined_attribute["x-amz-website-redirect-location"] = "None";
  system_defined_attribute["x-amz-server-side-encryption-aws-kms-key-id"] = "";
  system_defined_attribute["x-amz-server-side-encryption-customer-algorithm"] = "";
  system_defined_attribute["x-amz-server-side-encryption-customer-key"] = "";
  system_defined_attribute["x-amz-server-side-encryption-customer-key-MD5"] = "";
}

std::string S3PartMetadata::get_object_name() {
  return object_name;
}

std::string S3PartMetadata::get_upload_id() {
  return upload_id;
}

std::string S3PartMetadata::get_part_number() {
  return part_number;
}

std::string S3PartMetadata::get_last_modified() {
  return system_defined_attribute["Last-Modified"];
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

void S3PartMetadata::add_system_attribute(std::string key, std::string val) {
  system_defined_attribute[key] = val;
}

void S3PartMetadata::add_user_defined_attribute(std::string key, std::string val) {
  user_defined_attribute[key] = val;
}

void S3PartMetadata::load(std::function<void(void)> on_success, std::function<void(void)> on_failed, int part_num = 1) {
  printf("Called S3PartMetadata::load\n");
  std::string str_part_num = "";
  if(part_num > 0) {
    str_part_num = std::to_string(part_num);
  }
  this->handler_on_success = on_success;
  this->handler_on_failed  = on_failed;

  clovis_kv_reader = std::make_shared<S3ClovisKVSReader>(request);
  clovis_kv_reader->get_keyval(get_part_index_name(), str_part_num, std::bind( &S3PartMetadata::load_successful, this),
                                   std::bind( &S3PartMetadata::load_failed, this));
}

void S3PartMetadata::load_successful() {
  printf("Called S3PartMetadata::load_successful\n");
  this->from_json(clovis_kv_reader->get_value());
  state = S3PartMetadataState::present;
  this->handler_on_success();
}

void S3PartMetadata::load_failed() {
  // TODO - do anything more for failure?
  printf("Called S3PartMetadata::load_failed\n");
  if (clovis_kv_reader->get_state() == S3ClovisKVSReaderOpState::missing) {
    state = S3PartMetadataState::missing;  // Missing
  } else {
    state = S3PartMetadataState::failed;
  }
  this->handler_on_failed();
}

void S3PartMetadata::create_index(std::function<void(void)> on_success, std::function<void(void)> on_failed) {
  printf("Called S3PartMetadata::create_index\n");

  this->handler_on_success = on_success;
  this->handler_on_failed  = on_failed;
  put_metadata = false;
  create_part_index();
}


void S3PartMetadata::save(std::function<void(void)> on_success, std::function<void(void)> on_failed) {
  printf("Called S3PartMetadata::save\n");

  this->handler_on_success = on_success;
  this->handler_on_failed  = on_failed;
  put_metadata = true;
  save_metadata();
}

void S3PartMetadata::create_part_index() {
  printf("Called S3PartMetadata::create_bucket_index\n");
  // Mark missing as we initiate write, in case it fails to write.
  state = S3PartMetadataState::missing;

  clovis_kv_writer = std::make_shared<S3ClovisKVSWriter>(request, s3_clovis_api);
  clovis_kv_writer->create_index(get_part_index_name(), std::bind( &S3PartMetadata::create_part_index_successful, this), std::bind( &S3PartMetadata::create_part_index_failed, this));
}

void S3PartMetadata::create_part_index_successful() {
  printf("Called S3PartMetadata::create_part_index_successful\n");
  if(put_metadata) {
    save_metadata();
  } else {
    state = S3PartMetadataState::store_created;
    this->handler_on_success();
  }
}

void S3PartMetadata::create_part_index_failed() {
  printf("Called S3PartMetadata::create_part_index_failed\n");
  if (clovis_kv_writer->get_state() == S3ClovisKVSWriterOpState::exists) {
    // We need to create index only once, do logging with log level as warning
    printf("Warning: Index %s already exist", get_part_index_name().c_str());
    state = S3PartMetadataState::present;
    this->handler_on_success();
  } else {
      state = S3PartMetadataState::failed;  // todo Check error
      this->handler_on_failed();
  }
}

void S3PartMetadata::save_metadata() {
  std::string index_name;
  std::string key;
  // Set up system attributes
  system_defined_attribute["Upload-ID"] = upload_id;

  clovis_kv_writer = std::make_shared<S3ClovisKVSWriter>(request, s3_clovis_api);
  clovis_kv_writer->put_keyval(get_part_index_name(), part_number, this->to_json(), std::bind( &S3PartMetadata::save_metadata_successful, this), std::bind( &S3PartMetadata::save_metadata_failed, this));
}

void S3PartMetadata::save_metadata_successful() {
  printf("Called S3PartMetadata::save_metadata_successful\n");
  state = S3PartMetadataState::saved;
  this->handler_on_success();
}

void S3PartMetadata::save_metadata_failed() {
  // TODO - do anything more for failure?
  printf("Called S3PartMetadata::save_metadata_failed\n");
  state = S3PartMetadataState::failed;
  this->handler_on_failed();
}

void S3PartMetadata::remove(std::function<void(void)> on_success, std::function<void(void)> on_failed, int remove_part = 0) {
  printf("Called S3PartMetadata::remove\n");
  std::string part_removal = std::to_string(remove_part);

  this->handler_on_success = on_success;
  this->handler_on_failed  = on_failed;

  clovis_kv_writer = std::make_shared<S3ClovisKVSWriter>(request, s3_clovis_api);
  clovis_kv_writer->delete_keyval(get_part_index_name(), part_removal, std::bind( &S3PartMetadata::remove_successful, this), std::bind( &S3PartMetadata::remove_failed, this));
}

void S3PartMetadata::remove_successful() {
  printf("Called S3PartMetadata::remove_successful\n");
  state = S3PartMetadataState::deleted;
  this->handler_on_success();
}

void S3PartMetadata::remove_failed() {
  printf("Called S3PartMetadata::remove_failed\n");
  state = S3PartMetadataState::failed;
  this->handler_on_failed();
}


void S3PartMetadata::remove_index(std::function<void(void)> on_success, std::function<void(void)> on_failed) {
  printf("Called S3PartMetadata::remove_index\n");

  this->handler_on_success = on_success;
  this->handler_on_failed  = on_failed;

  clovis_kv_writer = std::make_shared<S3ClovisKVSWriter>(request, s3_clovis_api);
  clovis_kv_writer->delete_index(get_part_index_name(), std::bind( &S3PartMetadata::remove_index_successful, this), std::bind( &S3PartMetadata::remove_index_failed, this));
}

void S3PartMetadata::remove_index_successful() {
  printf("Called S3PartMetadata::remove_index_successful\n");
  state = S3PartMetadataState::index_deleted;
  this->handler_on_success();
}

void S3PartMetadata::remove_index_failed() {
  printf("Called S3PartMetadata::remove_index_failed\n");
  if(clovis_kv_writer->get_state() == S3ClovisKVSWriterOpState::failed) {
    state = S3PartMetadataState::failed;
  }
  this->handler_on_failed();
}

// Streaming to json
std::string S3PartMetadata::to_json() {
  printf("Called S3PartMetadata::to_json\n");
  Json::Value root;
  root["Bucket-Name"] = bucket_name;
  root["Object-Name"] = object_name;
  root["Upload-ID"] = upload_id;
  root["Part-Num"] = part_number;

  for (auto sit: system_defined_attribute) {
    root["System-Defined"][sit.first] = sit.second;
  }
  for (auto uit: user_defined_attribute) {
    root["User-Defined"][uit.first] = uit.second;
  }
  Json::FastWriter fastWriter;
  return fastWriter.write(root);;
}

void S3PartMetadata::from_json(std::string content) {
  printf("Called S3PartMetadata::from_json\n");
  Json::Value newroot;
  Json::Reader reader;
  bool parsingSuccessful = reader.parse(content.c_str(), newroot);
  if (!parsingSuccessful)
  {
    printf("Json Parsing failed.\n");
    return;
  }

  bucket_name = newroot["Bucket-Name"].asString();
  object_name = newroot["Object-Name"].asString();
  upload_id = newroot["Upload-ID"].asString();
  part_number = newroot["Part-Num"].asString();

  Json::Value::Members members = newroot["System-Defined"].getMemberNames();
  for(auto it : members) {
    system_defined_attribute[it.c_str()] = newroot["System-Defined"][it].asString().c_str();
  }
  members = newroot["User-Defined"].getMemberNames();
  for(auto it : members) {
    user_defined_attribute[it.c_str()] = newroot["User-Defined"][it].asString().c_str();
  }
}

