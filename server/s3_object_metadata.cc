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

#include <stdlib.h>
#include <json/json.h>
#include "base64.h"

#include "s3_object_metadata.h"
#include "s3_datetime.h"
#include "s3_log.h"

S3ObjectMetadata::S3ObjectMetadata(std::shared_ptr<S3RequestObject> req, bool ismultipart, std::string uploadid) : request(req) {
  s3_log(S3_LOG_DEBUG, "Constructor\n");

  account_name = request->get_account_name();
  account_id = request->get_account_id();
  user_name = request->get_user_name();
  user_id = request->get_user_id();
  bucket_name = request->get_bucket_name();
  object_name = request->get_object_name();
  state = S3ObjectMetadataState::empty;
  is_multipart = ismultipart;
  upload_id = uploadid;
  oid = M0_CLOVIS_ID_APP;

  s3_clovis_api = std::make_shared<ConcreteClovisAPI>();

  object_key_uri = bucket_name + "\\" + object_name;

  // Set the defaults
  S3DateTime current_time;
  current_time.init_current_time();
  system_defined_attribute["Date"] = current_time.get_isoformat_string();
  system_defined_attribute["Content-Length"] = "";
  system_defined_attribute["Last-Modified"] = current_time.get_isoformat_string();  // TODO
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

std::string S3ObjectMetadata::get_object_name() {
  return object_name;
}

std::string S3ObjectMetadata::get_user_id() {
  return user_id;
}

std::string S3ObjectMetadata::get_upload_id() {
  return upload_id;
}

std::string S3ObjectMetadata::get_user_name() {
  return user_name;
}

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

std::string S3ObjectMetadata::get_storage_class() {
  return system_defined_attribute["x-amz-storage-class"];
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

void S3ObjectMetadata::set_oid(struct m0_uint128 id) {
  oid = id;

  mero_oid_u_hi_str = base64_encode((unsigned char const*)&oid.u_hi, sizeof(oid.u_hi));
  mero_oid_u_lo_str = base64_encode((unsigned char const*)&oid.u_lo, sizeof(oid.u_lo));
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
  s3_log(S3_LOG_DEBUG, "Entering\n");

  this->handler_on_success = on_success;
  this->handler_on_failed  = on_failed;

  clovis_kv_reader = std::make_shared<S3ClovisKVSReader>(request);
  if(is_multipart) {
    clovis_kv_reader->get_keyval(get_multipart_index_name(), object_name, std::bind( &S3ObjectMetadata::load_successful, this), std::bind( &S3ObjectMetadata::load_failed, this));
  } else {
    clovis_kv_reader->get_keyval(get_bucket_index_name(), object_name, std::bind( &S3ObjectMetadata::load_successful, this), std::bind( &S3ObjectMetadata::load_failed, this));
  }
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ObjectMetadata::load_successful() {
  s3_log(S3_LOG_DEBUG, "Object metadata load successful\n");
  this->from_json(clovis_kv_reader->get_value());
  state = S3ObjectMetadataState::present;
  this->handler_on_success();
}

void S3ObjectMetadata::load_failed() {
  if (clovis_kv_reader->get_state() == S3ClovisKVSReaderOpState::missing) {
    s3_log(S3_LOG_DEBUG, "Object metadata missing\n");
    state = S3ObjectMetadataState::missing;  // Missing
  } else {
    s3_log(S3_LOG_WARN, "Object metadata load failed\n");
    state = S3ObjectMetadataState::failed;
  }
  this->handler_on_failed();
}

void S3ObjectMetadata::save(std::function<void(void)> on_success, std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, "Saving Object metadata\n");

  this->handler_on_success = on_success;
  this->handler_on_failed  = on_failed;

  create_bucket_index();
}

void S3ObjectMetadata::create_bucket_index() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  std::string index_name;
  // Mark missing as we initiate write, in case it fails to write.
  state = S3ObjectMetadataState::missing;

  clovis_kv_writer = std::make_shared<S3ClovisKVSWriter>(request, s3_clovis_api);
  if(is_multipart) {
    index_name = get_multipart_index_name();
  } else {
    index_name = get_bucket_index_name();
  }
  clovis_kv_writer->create_index(index_name, std::bind( &S3ObjectMetadata::create_bucket_index_successful, this), std::bind( &S3ObjectMetadata::create_bucket_index_failed, this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ObjectMetadata::create_bucket_index_successful() {
  s3_log(S3_LOG_DEBUG, "Object metadata bucket index created.\n");
  save_metadata();
}

void S3ObjectMetadata::create_bucket_index_failed() {
  if (clovis_kv_writer->get_state() == S3ClovisKVSWriterOpState::exists) {
    s3_log(S3_LOG_DEBUG, "Object metadata bucket index present.\n");
    // We need to create index only once.
    save_metadata();
  } else {
    s3_log(S3_LOG_DEBUG, "Object metadata create bucket index failed.\n");
    state = S3ObjectMetadataState::failed;  // todo Check error
    this->handler_on_failed();
  }
}

void S3ObjectMetadata::save_metadata() {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  std::string index_name;
  std::string key;
  // Set up system attributes
  system_defined_attribute["Owner-User"] = user_name;
  system_defined_attribute["Owner-User-id"] = user_id;
  system_defined_attribute["Owner-Account"] = account_name;
  system_defined_attribute["Owner-Account-id"] = account_id;
  if( is_multipart ) {
    system_defined_attribute["Upload-ID"] = upload_id;
    index_name = get_multipart_index_name();
  } else {
    index_name = get_bucket_index_name();
  }

  clovis_kv_writer = std::make_shared<S3ClovisKVSWriter>(request, s3_clovis_api);
  clovis_kv_writer->put_keyval(index_name, object_name, this->to_json(), std::bind( &S3ObjectMetadata::save_metadata_successful, this), std::bind( &S3ObjectMetadata::save_metadata_failed, this));
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ObjectMetadata::save_metadata(std::function<void(void)> on_success, std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, "Entering\n");
  std::string index_name;
  std::string key;
  this->handler_on_success = on_success;
  this->handler_on_failed  = on_failed;
  save_metadata();
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}

void S3ObjectMetadata::save_metadata_successful() {
  s3_log(S3_LOG_DEBUG, "Object metadata saved for Object [%s].\n", object_name.c_str());
  state = S3ObjectMetadataState::saved;
  this->handler_on_success();
}

void S3ObjectMetadata::save_metadata_failed() {
  s3_log(S3_LOG_ERROR, "Object metadata save failed for Object [%s].\n", object_name.c_str());
  state = S3ObjectMetadataState::failed;
  this->handler_on_failed();
}

void S3ObjectMetadata::remove(std::function<void(void)> on_success, std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, "Deleting Object metadata for Object [%s].\n", object_name.c_str());

  std::string index_name;

  this->handler_on_success = on_success;
  this->handler_on_failed  = on_failed;

  if( is_multipart ) {
    index_name = get_multipart_index_name();
  } else {
    index_name = get_bucket_index_name();
  }

  clovis_kv_writer = std::make_shared<S3ClovisKVSWriter>(request, s3_clovis_api);
  clovis_kv_writer->delete_keyval(index_name, object_name, std::bind( &S3ObjectMetadata::remove_successful, this), std::bind( &S3ObjectMetadata::remove_failed, this));
}

void S3ObjectMetadata::remove_successful() {
  s3_log(S3_LOG_DEBUG, "Deleted Object metadata for Object [%s].\n", object_name.c_str());
  state = S3ObjectMetadataState::deleted;
  this->handler_on_success();
}

void S3ObjectMetadata::remove_failed() {
  s3_log(S3_LOG_DEBUG, "Delete Object metadata failed for Object [%s].\n", object_name.c_str());
  state = S3ObjectMetadataState::failed;
  this->handler_on_failed();
}

std::string S3ObjectMetadata::create_default_acl() {
  std::string acl_str;
  acl_str =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
         "<AccessControlPolicy xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">\n"
         "  <Owner>\n"
         "    <ID>" + request->get_account_id() + "</ID>\n"
         "      <DisplayName>" + request->get_account_name() + "</DisplayName>\n"
         "  </Owner>\n"
         "  <AccessControlList>\n"
         "    <Grant>\n"
         "      <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:type=\"CanonicalUser\">\n"
         "        <ID>" + request->get_account_id() + "</ID>\n"
         "        <DisplayName>" + request->get_account_name() + "</DisplayName>\n"
         "      </Grantee>\n"
         "      <Permission>FULL_CONTROL</Permission>\n"
         "    </Grant>\n"
         "  </AccessControlList>\n"
         "</AccessControlPolicy>\n";
  return acl_str;
}

// Streaming to json
std::string S3ObjectMetadata::to_json() {
  s3_log(S3_LOG_DEBUG, "Called\n");
  Json::Value root;
  root["Bucket-Name"] = bucket_name;
  root["Object-Name"] = object_name;
  root["Object-URI"] = object_key_uri;
  if(is_multipart) {
    root["Upload-ID"] = upload_id;
  }

  root["mero_oid_u_hi"] = mero_oid_u_hi_str;
  root["mero_oid_u_lo"] = mero_oid_u_lo_str;

  for (auto sit: system_defined_attribute) {
    root["System-Defined"][sit.first] = sit.second;
  }
  for (auto uit: user_defined_attribute) {
    root["User-Defined"][uit.first] = uit.second;
  }
  //root["ACL"] = object_ACL.to_json();
  std::string xml_acl = object_ACL.get_xml_str();
  if (xml_acl == "") {
    xml_acl = create_default_acl();
  }
  root["ACL"] = base64_encode((const unsigned char*)xml_acl.c_str(), xml_acl.size());

  Json::FastWriter fastWriter;
  return fastWriter.write(root);;
}

void S3ObjectMetadata::from_json(std::string content) {
  s3_log(S3_LOG_DEBUG, "Called\n");
  Json::Value newroot;
  Json::Reader reader;
  bool parsingSuccessful = reader.parse(content.c_str(), newroot);
  if (!parsingSuccessful)
  {
    s3_log(S3_LOG_ERROR,"Json Parsing failed\n");
    return;
  }

  bucket_name = newroot["Bucket-Name"].asString();
  object_name = newroot["Object-Name"].asString();
  object_key_uri = newroot["Object-URI"].asString();
  upload_id = newroot["Upload-ID"].asString();

  mero_oid_u_hi_str = newroot["mero_oid_u_hi"].asString();
  mero_oid_u_lo_str = newroot["mero_oid_u_lo"].asString();

  std::string dec_oid_u_hi_str = base64_decode(mero_oid_u_hi_str);
  std::string dec_oid_u_lo_str = base64_decode(mero_oid_u_lo_str);

  // std::string decoded_oid_str = base64_decode(oid_str);
  memcpy((void*)&oid.u_hi, dec_oid_u_hi_str.c_str(), dec_oid_u_hi_str.length());
  memcpy((void*)&oid.u_lo, dec_oid_u_lo_str.c_str(), dec_oid_u_lo_str.length());

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

std::string& S3ObjectMetadata::get_encoded_object_acl() {
  // base64 encoded acl
  return object_ACL.get_acl_metadata();
}

void S3ObjectMetadata::setacl(std::string & input_acl_str) {
  std::string input_acl = input_acl_str;
  input_acl = object_ACL.insert_display_name(input_acl);
  object_ACL.set_acl_xml_metadata(input_acl);
}

std::string& S3ObjectMetadata::get_acl_as_xml() {
  return object_ACL.get_xml_str();
}
