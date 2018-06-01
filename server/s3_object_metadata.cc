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
#include <stdlib.h>
#include "base64.h"

#include "s3_datetime.h"
#include "s3_factory.h"
#include "s3_iem.h"
#include "s3_log.h"
#include "s3_object_metadata.h"
#include "s3_uri_to_mero_oid.h"

void S3ObjectMetadata::initialize(bool ismultipart, std::string uploadid) {
  json_parsing_error = false;
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
  old_oid = {0ULL, 0ULL};
  layout_id = 0;
  old_layout_id = 0;
  tried_count = 0;
  s3_clovis_api = std::make_shared<ConcreteClovisAPI>();

  object_key_uri = bucket_name + "\\" + object_name;

  // Set the defaults
  S3DateTime current_time;
  current_time.init_current_time();
  system_defined_attribute["Date"] = current_time.get_isoformat_string();
  system_defined_attribute["Content-Length"] = "";
  system_defined_attribute["Last-Modified"] =
      current_time.get_isoformat_string();  // TODO
  system_defined_attribute["Content-MD5"] = "";
  system_defined_attribute["Owner-User"] = "";
  system_defined_attribute["Owner-User-id"] = "";
  system_defined_attribute["Owner-Account"] = "";
  system_defined_attribute["Owner-Account-id"] = "";

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
  if (is_multipart) {
    index_name = get_multipart_index_name();
    system_defined_attribute["Upload-ID"] = upload_id;
  } else {
    index_name = get_bucket_index_name();
  }

  index_oid = {0ULL, 0ULL};
}

S3ObjectMetadata::S3ObjectMetadata(
    std::shared_ptr<S3RequestObject> req, bool ismultipart,
    std::string uploadid,
    std::shared_ptr<S3ClovisKVSReaderFactory> kv_reader_factory,
    std::shared_ptr<S3ClovisKVSWriterFactory> kv_writer_factory,
    std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory)
    : request(req) {
  request_id = request->get_request_id();
  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");
  initialize(ismultipart, uploadid);

  if (bucket_meta_factory) {
    bucket_metadata_factory = bucket_meta_factory;
  } else {
    bucket_metadata_factory = std::make_shared<S3BucketMetadataFactory>();
  }

  if (kv_reader_factory) {
    clovis_kv_reader_factory = kv_reader_factory;
  } else {
    clovis_kv_reader_factory = std::make_shared<S3ClovisKVSReaderFactory>();
  }

  if (kv_writer_factory) {
    clovis_kv_writer_factory = kv_writer_factory;
  } else {
    clovis_kv_writer_factory = std::make_shared<S3ClovisKVSWriterFactory>();
  }
}

S3ObjectMetadata::S3ObjectMetadata(
    std::shared_ptr<S3RequestObject> req, m0_uint128 bucket_idx_oid,
    bool ismultipart, std::string uploadid,
    std::shared_ptr<S3ClovisKVSReaderFactory> kv_reader_factory,
    std::shared_ptr<S3ClovisKVSWriterFactory> kv_writer_factory,
    std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory)
    : request(req) {
  request_id = request->get_request_id();
  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");

  initialize(ismultipart, uploadid);
  index_oid.u_hi = bucket_idx_oid.u_hi;
  index_oid.u_lo = bucket_idx_oid.u_lo;
  if (bucket_meta_factory) {
    bucket_metadata_factory = bucket_meta_factory;
  } else {
    bucket_metadata_factory = std::make_shared<S3BucketMetadataFactory>();
  }

  if (kv_reader_factory) {
    clovis_kv_reader_factory = kv_reader_factory;
  } else {
    clovis_kv_reader_factory = std::make_shared<S3ClovisKVSReaderFactory>();
  }

  if (kv_writer_factory) {
    clovis_kv_writer_factory = kv_writer_factory;
  } else {
    clovis_kv_writer_factory = std::make_shared<S3ClovisKVSWriterFactory>();
  }
}

std::string S3ObjectMetadata::get_owner_id() {
  return system_defined_attribute["Owner-User-id"];
}

std::string S3ObjectMetadata::get_owner_name() {
  return system_defined_attribute["Owner-User"];
}

std::string S3ObjectMetadata::get_object_name() { return object_name; }

struct m0_uint128 S3ObjectMetadata::get_index_oid() {
  return index_oid;
}

std::string S3ObjectMetadata::get_user_id() { return user_id; }

std::string S3ObjectMetadata::get_upload_id() { return upload_id; }

std::string S3ObjectMetadata::get_user_name() { return user_name; }

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

  mero_oid_u_hi_str =
      base64_encode((unsigned char const*)&oid.u_hi, sizeof(oid.u_hi));
  mero_oid_u_lo_str =
      base64_encode((unsigned char const*)&oid.u_lo, sizeof(oid.u_lo));
}

void S3ObjectMetadata::set_old_oid(struct m0_uint128 id) {
  old_oid = id;

  mero_old_oid_u_hi_str =
      base64_encode((unsigned char const*)&old_oid.u_hi, sizeof(old_oid.u_hi));
  mero_old_oid_u_lo_str =
      base64_encode((unsigned char const*)&old_oid.u_lo, sizeof(old_oid.u_lo));
}

void S3ObjectMetadata::set_part_index_oid(struct m0_uint128 id) {
  part_index_oid = id;
  mero_part_oid_u_hi_str = base64_encode(
      (unsigned char const*)&part_index_oid.u_hi, sizeof(part_index_oid.u_hi));
  mero_part_oid_u_lo_str = base64_encode(
      (unsigned char const*)&part_index_oid.u_lo, sizeof(part_index_oid.u_lo));
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
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;

  clovis_kv_reader = clovis_kv_reader_factory->create_clovis_kvs_reader(
      request, s3_clovis_api);
  clovis_kv_reader->get_keyval(
      index_oid, object_name,
      std::bind(&S3ObjectMetadata::load_successful, this),
      std::bind(&S3ObjectMetadata::load_failed, this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3ObjectMetadata::load_successful() {
  s3_log(S3_LOG_DEBUG, request_id, "Object metadata load successful\n");
  if (this->from_json(clovis_kv_reader->get_value()) != 0) {
    s3_log(S3_LOG_ERROR, request_id,
           "Json Parsing failed. Index = %lu %lu, Key = %s, Value = %s\n",
           index_oid.u_hi, index_oid.u_lo, object_name.c_str(),
           clovis_kv_reader->get_value().c_str());
    s3_iem(LOG_ERR, S3_IEM_METADATA_CORRUPTED, S3_IEM_METADATA_CORRUPTED_STR,
           S3_IEM_METADATA_CORRUPTED_JSON);

    json_parsing_error = true;
    load_failed();
  } else {
    state = S3ObjectMetadataState::present;
    this->handler_on_success();
  }
}

void S3ObjectMetadata::load_failed() {
  if (json_parsing_error) {
    state = S3ObjectMetadataState::failed;
  } else if (clovis_kv_reader->get_state() ==
             S3ClovisKVSReaderOpState::missing) {
    s3_log(S3_LOG_DEBUG, request_id, "Object metadata missing for %s\n",
           object_name.c_str());
    state = S3ObjectMetadataState::missing;  // Missing
  } else {
    s3_log(S3_LOG_WARN, request_id, "Object metadata load failed\n");
    state = S3ObjectMetadataState::failed;
  }
  this->handler_on_failed();
}

void S3ObjectMetadata::save(std::function<void(void)> on_success,
                            std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, request_id, "Saving Object metadata\n");

  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;
  if (index_oid.u_lo == 0 && index_oid.u_hi == 0) {
    S3UriToMeroOID(index_name.c_str(), &index_oid, S3ClovisEntityType::index);
    // Index table doesn't exist so create it
    create_bucket_index();
  } else {
    save_metadata();
  }
}

void S3ObjectMetadata::create_bucket_index() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  // Mark missing as we initiate write, in case it fails to write.
  state = S3ObjectMetadataState::missing;
  if (tried_count == 0) {
    clovis_kv_writer = clovis_kv_writer_factory->create_clovis_kvs_writer(
        request, s3_clovis_api);
  }
  clovis_kv_writer->create_index_with_oid(
      index_oid,
      std::bind(&S3ObjectMetadata::create_bucket_index_successful, this),
      std::bind(&S3ObjectMetadata::create_bucket_index_failed, this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3ObjectMetadata::create_bucket_index_successful() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  s3_log(S3_LOG_DEBUG, request_id, "Object metadata bucket index created.\n");
  bucket_metadata =
      bucket_metadata_factory->create_bucket_metadata_obj(request);
  if (is_multipart) {
    bucket_metadata->set_multipart_index_oid(index_oid);
  } else {
    bucket_metadata->set_object_list_index_oid(index_oid);
  }
  bucket_metadata->save(
      std::bind(&S3ObjectMetadata::save_object_list_index_oid_successful, this),
      std::bind(&S3ObjectMetadata::save_object_list_index_oid_failed, this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3ObjectMetadata::save_object_list_index_oid_successful() {
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  save_metadata();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3ObjectMetadata::save_object_list_index_oid_failed() {
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
  s3_log(S3_LOG_DEBUG, request_id,
         "Object metadata create bucket index failed.\n");
  state = S3ObjectMetadataState::failed;  // todo Check error
  this->handler_on_failed();
}

void S3ObjectMetadata::create_bucket_index_failed() {
  if (clovis_kv_writer->get_state() == S3ClovisKVSWriterOpState::exists) {
    s3_log(S3_LOG_DEBUG, request_id, "Object metadata bucket index present.\n");
    // create_bucket_index gets called when bucket index is not there, hence if
    // state is "exists" then it will be due to collision, resolve it.
    collision_detected();
  } else {
    s3_log(S3_LOG_DEBUG, request_id,
           "Object metadata create bucket index failed.\n");
    state = S3ObjectMetadataState::failed;  // todo Check error
    this->handler_on_failed();
  }
}

void S3ObjectMetadata::collision_detected() {
  if (tried_count < MAX_COLLISION_RETRY_COUNT) {
    s3_log(S3_LOG_INFO, request_id,
           "Object ID collision happened for index %s\n", index_name.c_str());
    // Handle Collision
    create_new_oid();
    tried_count++;
    if (tried_count > 5) {
      s3_log(S3_LOG_INFO, request_id,
             "Object ID collision happened %d times for index %s\n",
             tried_count, index_name.c_str());
    }
    create_bucket_index();
  } else {
    if (tried_count >= MAX_COLLISION_RETRY_COUNT) {
      s3_log(S3_LOG_ERROR, request_id,
             "Failed to resolve object id collision %d times for index %s\n",
             tried_count, index_name.c_str());
      s3_iem(LOG_ERR, S3_IEM_COLLISION_RES_FAIL, S3_IEM_COLLISION_RES_FAIL_STR,
             S3_IEM_COLLISION_RES_FAIL_JSON);
    }
    state = S3ObjectMetadataState::failed;
    this->handler_on_failed();
  }
}

void S3ObjectMetadata::create_new_oid() {
  std::string salted_index_name =
      index_name + salt + std::to_string(tried_count);
  S3UriToMeroOID(salted_index_name.c_str(), &index_oid,
                 S3ClovisEntityType::index);
  return;
}

void S3ObjectMetadata::save_metadata() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  // Set up system attributes
  system_defined_attribute["Owner-User"] = user_name;
  system_defined_attribute["Owner-User-id"] = user_id;
  system_defined_attribute["Owner-Account"] = account_name;
  system_defined_attribute["Owner-Account-id"] = account_id;
  clovis_kv_writer = clovis_kv_writer_factory->create_clovis_kvs_writer(
      request, s3_clovis_api);
  clovis_kv_writer->put_keyval(
      index_oid, object_name, this->to_json(),
      std::bind(&S3ObjectMetadata::save_metadata_successful, this),
      std::bind(&S3ObjectMetadata::save_metadata_failed, this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3ObjectMetadata::save_metadata(std::function<void(void)> on_success,
                                     std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;
  save_metadata();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
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
  state = S3ObjectMetadataState::failed;
  this->handler_on_failed();
}

void S3ObjectMetadata::remove(std::function<void(void)> on_success,
                              std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, request_id,
         "Deleting Object metadata for Object [%s].\n", object_name.c_str());

  std::string index_name;

  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;

  if (is_multipart) {
    index_name = get_multipart_index_name();
  } else {
    index_name = get_bucket_index_name();
  }

  clovis_kv_writer = clovis_kv_writer_factory->create_clovis_kvs_writer(
      request, s3_clovis_api);
  clovis_kv_writer->delete_keyval(
      index_oid, object_name,
      std::bind(&S3ObjectMetadata::remove_successful, this),
      std::bind(&S3ObjectMetadata::remove_failed, this));
}

void S3ObjectMetadata::remove_successful() {
  s3_log(S3_LOG_DEBUG, request_id, "Deleted Object metadata for Object [%s].\n",
         object_name.c_str());
  state = S3ObjectMetadataState::deleted;
  this->handler_on_success();
}

void S3ObjectMetadata::remove_failed() {
  s3_log(S3_LOG_DEBUG, request_id,
         "Delete Object metadata failed for Object [%s].\n",
         object_name.c_str());
  state = S3ObjectMetadataState::failed;
  this->handler_on_failed();
}

std::string S3ObjectMetadata::create_default_acl() {
  std::string acl_str;
  acl_str =
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<AccessControlPolicy "
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">\n"
      "  <Owner>\n"
      "    <ID>" +
      get_owner_id() +
      "</ID>\n"
      "      <DisplayName>" +
      get_owner_name() +
      "</DisplayName>\n"
      "  </Owner>\n"
      "  <AccessControlList>\n"
      "    <Grant>\n"
      "      <Grantee xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
      "xsi:type=\"CanonicalUser\">\n"
      "        <ID>" +
      get_owner_id() +
      "</ID>\n"
      "        <DisplayName>" +
      get_owner_name() +
      "</DisplayName>\n"
      "      </Grantee>\n"
      "      <Permission>FULL_CONTROL</Permission>\n"
      "    </Grant>\n"
      "  </AccessControlList>\n"
      "</AccessControlPolicy>\n";
  return acl_str;
}

// Streaming to json
std::string S3ObjectMetadata::to_json() {
  s3_log(S3_LOG_DEBUG, request_id, "Called\n");
  Json::Value root;
  root["Bucket-Name"] = bucket_name;
  root["Object-Name"] = object_name;
  root["Object-URI"] = object_key_uri;
  root["layout_id"] = layout_id;
  if (is_multipart) {
    root["Upload-ID"] = upload_id;
    root["mero_part_oid_u_hi"] = mero_part_oid_u_hi_str;
    root["mero_part_oid_u_lo"] = mero_part_oid_u_lo_str;
    root["mero_old_oid_u_hi"] = mero_old_oid_u_hi_str;
    root["mero_old_oid_u_lo"] = mero_old_oid_u_lo_str;
    root["old_layout_id"] = old_layout_id;
  }

  root["mero_oid_u_hi"] = mero_oid_u_hi_str;
  root["mero_oid_u_lo"] = mero_oid_u_lo_str;

  for (auto sit : system_defined_attribute) {
    root["System-Defined"][sit.first] = sit.second;
  }
  for (auto uit : user_defined_attribute) {
    root["User-Defined"][uit.first] = uit.second;
  }
  // root["ACL"] = object_ACL.to_json();
  std::string xml_acl = object_ACL.get_xml_str();
  if (xml_acl == "") {
    xml_acl = create_default_acl();
  }
  root["ACL"] =
      base64_encode((const unsigned char*)xml_acl.c_str(), xml_acl.size());

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
  if (!parsingSuccessful || s3_fi_is_enabled("object_metadata_corrupted")) {
    s3_log(S3_LOG_ERROR, request_id, "Json Parsing failed\n");
    return -1;
  }

  bucket_name = newroot["Bucket-Name"].asString();
  object_name = newroot["Object-Name"].asString();
  object_key_uri = newroot["Object-URI"].asString();
  upload_id = newroot["Upload-ID"].asString();
  mero_part_oid_u_hi_str = newroot["mero_part_oid_u_hi"].asString();
  mero_part_oid_u_lo_str = newroot["mero_part_oid_u_lo"].asString();

  mero_oid_u_hi_str = newroot["mero_oid_u_hi"].asString();
  mero_oid_u_lo_str = newroot["mero_oid_u_lo"].asString();
  layout_id = newroot["layout_id"].asInt();

  std::string dec_oid_u_hi_str = base64_decode(mero_oid_u_hi_str);
  std::string dec_oid_u_lo_str = base64_decode(mero_oid_u_lo_str);

  // std::string decoded_oid_str = base64_decode(oid_str);
  memcpy((void*)&oid.u_hi, dec_oid_u_hi_str.c_str(), dec_oid_u_hi_str.length());
  memcpy((void*)&oid.u_lo, dec_oid_u_lo_str.c_str(), dec_oid_u_lo_str.length());

  //
  // Old oid is needed to remove the OID when the object already exists
  // during upload, this is needed in case of multipart upload only
  // As upload happens in multiple sessions, so this old oid
  // will be used in post complete action.
  //
  if (is_multipart) {
    mero_old_oid_u_hi_str = newroot["mero_old_oid_u_hi"].asString();
    mero_old_oid_u_lo_str = newroot["mero_old_oid_u_lo"].asString();
    dec_oid_u_hi_str = base64_decode(mero_old_oid_u_hi_str);
    dec_oid_u_lo_str = base64_decode(mero_old_oid_u_lo_str);
    memcpy((void*)&old_oid.u_hi, dec_oid_u_hi_str.c_str(),
           dec_oid_u_hi_str.length());
    memcpy((void*)&old_oid.u_lo, dec_oid_u_lo_str.c_str(),
           dec_oid_u_lo_str.length());
    old_layout_id = newroot["old_layout_id"].asInt();
  }

  dec_oid_u_hi_str = base64_decode(mero_part_oid_u_hi_str);
  dec_oid_u_lo_str = base64_decode(mero_part_oid_u_lo_str);
  memcpy((void*)&part_index_oid.u_hi, dec_oid_u_hi_str.c_str(),
         dec_oid_u_hi_str.length());
  memcpy((void*)&part_index_oid.u_lo, dec_oid_u_lo_str.c_str(),
         dec_oid_u_lo_str.length());

  Json::Value::Members members = newroot["System-Defined"].getMemberNames();
  for (auto it : members) {
    system_defined_attribute[it.c_str()] =
        newroot["System-Defined"][it].asString().c_str();
  }
  user_name = system_defined_attribute["Owner-User"];
  user_id = system_defined_attribute["Owner-User-id"];
  account_name = system_defined_attribute["Owner-Account"];
  account_id = system_defined_attribute["Owner-Account-id"];

  members = newroot["User-Defined"].getMemberNames();
  for (auto it : members) {
    user_defined_attribute[it.c_str()] =
        newroot["User-Defined"][it].asString().c_str();
  }
  object_ACL.from_json(newroot["ACL"].asString());

  return 0;
}

std::string& S3ObjectMetadata::get_encoded_object_acl() {
  // base64 encoded acl
  return object_ACL.get_acl_metadata();
}

void S3ObjectMetadata::setacl(std::string& input_acl_str) {
  std::string input_acl = input_acl_str;
  object_ACL.set_display_name(get_owner_name());
  input_acl = object_ACL.insert_display_name(input_acl);
  object_ACL.set_acl_xml_metadata(input_acl);
}

std::string& S3ObjectMetadata::get_acl_as_xml() {
  return object_ACL.get_xml_str();
}
