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

#include "s3_bucket_metadata.h"
#include <json/json.h>
#include <string>
#include "base64.h"
#include "s3_datetime.h"
#include "s3_factory.h"
#include "s3_iem.h"
#include "s3_uri_to_mero_oid.h"
#include "s3_common_utilities.h"

S3BucketMetadata::S3BucketMetadata(
    std::shared_ptr<S3RequestObject> req, std::shared_ptr<ClovisAPI> clovis_api,
    std::shared_ptr<S3ClovisKVSReaderFactory> clovis_s3_kvs_reader_factory,
    std::shared_ptr<S3ClovisKVSWriterFactory> clovis_s3_kvs_writer_factory,
    std::shared_ptr<S3AccountUserIdxMetadataFactory>
        s3_account_user_idx_metadata_factory)
    : request(req), json_parsing_error(false) {
  request_id = request->get_request_id();
  s3_log(S3_LOG_DEBUG, request_id, "Constructor");
  account_name = request->get_account_name();
  account_id = request->get_account_id();
  user_name = request->get_user_name();
  user_id = request->get_user_id();
  bucket_name = request->get_bucket_name();
  salted_bucket_list_index_name = get_account_index_id();
  salted_multipart_list_index_name = get_multipart_index_name();
  state = S3BucketMetadataState::empty;
  current_op = S3BucketMetadataCurrentOp::none;
  if (clovis_api) {
    s3_clovis_api = clovis_api;
  } else {
    s3_clovis_api = std::make_shared<ConcreteClovisAPI>();
  }

  if (clovis_s3_kvs_reader_factory) {
    clovis_kvs_reader_factory = clovis_s3_kvs_reader_factory;
  } else {
    clovis_kvs_reader_factory = std::make_shared<S3ClovisKVSReaderFactory>();
  }

  if (clovis_s3_kvs_writer_factory) {
    clovis_kvs_writer_factory = clovis_s3_kvs_writer_factory;
  } else {
    clovis_kvs_writer_factory = std::make_shared<S3ClovisKVSWriterFactory>();
  }

  if (s3_account_user_idx_metadata_factory) {
    account_user_index_metadata_factory = s3_account_user_idx_metadata_factory;
  } else {
    account_user_index_metadata_factory =
        std::make_shared<S3AccountUserIdxMetadataFactory>();
  }

  bucket_policy = "";

  collision_salt = "index_salt_";
  collision_attempt_count = 0;

  // name of the index which holds all objects key values within a bucket
  salted_object_list_index_name = get_object_list_index_name();
  bucket_list_index_oid = {0ULL, 0ULL};
  object_list_index_oid = {0ULL, 0ULL};  // Object List index default id
  multipart_index_oid = {0ULL, 0ULL};    // Multipart index default id

  // Set the defaults
  S3DateTime current_time;
  current_time.init_current_time();
  system_defined_attribute["Date"] = current_time.get_isoformat_string();
  system_defined_attribute["LocationConstraint"] = "us-west-2";
  system_defined_attribute["Owner-User"] = "";
  system_defined_attribute["Owner-User-id"] = "";
  system_defined_attribute["Owner-Account"] = "";
  system_defined_attribute["Owner-Account-id"] = "";
}

std::string S3BucketMetadata::get_bucket_name() { return bucket_name; }

std::string S3BucketMetadata::get_creation_time() {
  return system_defined_attribute["Date"];
}

std::string S3BucketMetadata::get_location_constraint() {
  return system_defined_attribute["LocationConstraint"];
}

std::string S3BucketMetadata::get_owner_id() {
  return system_defined_attribute["Owner-User-id"];
}

std::string S3BucketMetadata::get_owner_name() {
  return system_defined_attribute["Owner-User"];
}

struct m0_uint128 S3BucketMetadata::get_bucket_list_index_oid() {
  return bucket_list_index_oid;
}

struct m0_uint128 S3BucketMetadata::get_multipart_index_oid() {
  return multipart_index_oid;
}

struct m0_uint128 S3BucketMetadata::get_object_list_index_oid() {
  return object_list_index_oid;
}

std::string S3BucketMetadata::get_object_list_index_oid_u_hi_str() {
  return object_list_index_oid_u_hi_str;
}

std::string S3BucketMetadata::get_object_list_index_oid_u_lo_str() {
  return object_list_index_oid_u_lo_str;
}

void S3BucketMetadata::set_bucket_list_index_oid(struct m0_uint128 oid) {
  bucket_list_index_oid = oid;
}

void S3BucketMetadata::set_multipart_index_oid(struct m0_uint128 oid) {
  multipart_index_oid = oid;
}

void S3BucketMetadata::set_object_list_index_oid(struct m0_uint128 oid) {
  object_list_index_oid = oid;
}

void S3BucketMetadata::set_location_constraint(std::string location) {
  system_defined_attribute["LocationConstraint"] = location;
}

void S3BucketMetadata::setpolicy(std::string& policy_str) {
  bucket_policy = policy_str;
}

void S3BucketMetadata::set_tags(
    const std::map<std::string, std::string>& tags_as_map) {
  bucket_tags = tags_as_map;
}

void S3BucketMetadata::deletepolicy() { bucket_policy = ""; }

void S3BucketMetadata::delete_bucket_tags() { bucket_tags.clear(); }

void S3BucketMetadata::setacl(std::string& acl_str) {
  std::string input_acl = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  input_acl += acl_str;
  bucket_ACL.set_display_name(get_owner_name());
  input_acl = bucket_ACL.insert_display_name(input_acl);
  bucket_ACL.set_acl_xml_metadata(input_acl);
}

void S3BucketMetadata::add_system_attribute(std::string key, std::string val) {
  system_defined_attribute[key] = val;
}

void S3BucketMetadata::add_user_defined_attribute(std::string key,
                                                  std::string val) {
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

void S3BucketMetadata::load(std::function<void(void)> on_success,
                            std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;

  current_op = S3BucketMetadataCurrentOp::fetching;
  fetch_bucket_list_index_oid();

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadata::fetch_bucket_list_index_oid() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

  if (!account_user_index_metadata) {
    account_user_index_metadata =
        account_user_index_metadata_factory
            ->create_s3_account_user_idx_metadata(request);
  }
  account_user_index_metadata->load(
      std::bind(&S3BucketMetadata::fetch_bucket_list_index_oid_success, this),
      std::bind(&S3BucketMetadata::fetch_bucket_list_index_oid_failed, this));

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadata::fetch_bucket_list_index_oid_success() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  bucket_list_index_oid =
      account_user_index_metadata->get_bucket_list_index_oid();
  if (current_op == S3BucketMetadataCurrentOp::saving) {
    save_bucket_info();
  } else if (current_op == S3BucketMetadataCurrentOp::fetching) {
    load_bucket_info();
  } else if (current_op == S3BucketMetadataCurrentOp::deleting) {
    remove_bucket_info();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadata::fetch_bucket_list_index_oid_failed() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

  if (account_user_index_metadata->get_state() ==
      S3AccountUserIdxMetadataState::missing) {
    state = S3BucketMetadataState::missing;
    if (current_op == S3BucketMetadataCurrentOp::saving ||
        current_op == S3BucketMetadataCurrentOp::fetching) {
      collision_attempt_count = 0;
      create_bucket_list_index();
    } else {
      this->handler_on_failed();
    }
  } else if (account_user_index_metadata->get_state() ==
             S3AccountUserIdxMetadataState::failed_to_launch) {
    s3_log(
        S3_LOG_ERROR, request_id,
        "Failed to fetch Bucket List index oid from Account User index. Please "
        "retry after some time\n");
    state = S3BucketMetadataState::failed_to_launch;
    this->handler_on_failed();
  } else {
    s3_log(
        S3_LOG_ERROR, request_id,
        "Failed to fetch Bucket List index oid from Account User index. Please "
        "retry after some time\n");
    state = S3BucketMetadataState::failed;
    this->handler_on_failed();
  }

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadata::load_bucket_info() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

  clovis_kv_reader = clovis_kvs_reader_factory->create_clovis_kvs_reader(
      request, s3_clovis_api);
  clovis_kv_reader->get_keyval(
      bucket_list_index_oid, bucket_name,
      std::bind(&S3BucketMetadata::load_bucket_info_successful, this),
      std::bind(&S3BucketMetadata::load_bucket_info_failed, this));

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadata::load_bucket_info_successful() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if (this->from_json(clovis_kv_reader->get_value()) != 0) {
    s3_log(S3_LOG_ERROR, request_id,
           "Json Parsing failed. Index oid ="
           "%" SCNx64 " : %" SCNx64 ", Key = %s Value = %s\n",
           bucket_list_index_oid.u_hi, bucket_list_index_oid.u_lo,
           bucket_name.c_str(), clovis_kv_reader->get_value().c_str());
    s3_iem(LOG_ERR, S3_IEM_METADATA_CORRUPTED, S3_IEM_METADATA_CORRUPTED_STR,
           S3_IEM_METADATA_CORRUPTED_JSON);

    json_parsing_error = true;
    load_bucket_info_failed();
  } else {
    state = S3BucketMetadataState::present;
    this->handler_on_success();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadata::load_bucket_info_failed() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

  if (json_parsing_error) {
    state = S3BucketMetadataState::failed;
  } else if (clovis_kv_reader->get_state() ==
             S3ClovisKVSReaderOpState::missing) {
    s3_log(S3_LOG_DEBUG, request_id, "Bucket metadata is missing\n");
    state = S3BucketMetadataState::missing;
  } else if (clovis_kv_reader->get_state() ==
             S3ClovisKVSReaderOpState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id, "Loading of bucket metadata failed\n");
    state = S3BucketMetadataState::failed_to_launch;
  } else {
    s3_log(S3_LOG_ERROR, request_id, "Loading of bucket metadata failed\n");
    state = S3BucketMetadataState::failed;
  }
  this->handler_on_failed();

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadata::save(std::function<void(void)> on_success,
                            std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;
  current_op = S3BucketMetadataCurrentOp::saving;
  if (bucket_list_index_oid.u_lo == 0ULL &&
      bucket_list_index_oid.u_hi == 0ULL) {
    // If there is no index oid then read it
    fetch_bucket_list_index_oid();
  } else {
    if (object_list_index_oid.u_lo == 0ULL &&
        object_list_index_oid.u_hi == 0ULL) {
      // create object list index, multipart index and then save bucket info
      collision_attempt_count = 0;
      create_object_list_index();
    } else {
      save_bucket_info();
    }
  }
}

void S3BucketMetadata::create_bucket_list_index() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

  if (collision_attempt_count == 0) {
    clovis_kv_writer = clovis_kvs_writer_factory->create_clovis_kvs_writer(
        request, s3_clovis_api);
  }
  clovis_kv_writer->create_index(
      salted_bucket_list_index_name,
      std::bind(&S3BucketMetadata::create_bucket_list_index_successful, this),
      std::bind(&S3BucketMetadata::create_bucket_list_index_failed, this));

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadata::create_bucket_list_index_successful() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  bucket_list_index_oid = clovis_kv_writer->get_oid();
  save_bucket_list_index_oid();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadata::create_bucket_list_index_failed() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if (clovis_kv_writer->get_state() == S3ClovisKVSWriterOpState::exists) {
    // create_bucket_list_index is called when there is no id for that index,
    // Hence if clovis returned its present then its due to collision.
    handle_collision(
        get_account_index_id(), salted_bucket_list_index_name,
        std::bind(&S3BucketMetadata::create_bucket_list_index, this));
  } else if (clovis_kv_writer->get_state() ==
             S3ClovisKVSWriterOpState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id, "Bucket list index creation failed.\n");
    state = S3BucketMetadataState::failed_to_launch;
    this->handler_on_failed();
  } else {
    s3_log(S3_LOG_ERROR, request_id, "Index creation failed.\n");
    state = S3BucketMetadataState::failed;
    this->handler_on_failed();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadata::handle_collision(std::string base_index_name,
                                        std::string& salted_index_name,
                                        std::function<void()> callback) {
  if (collision_attempt_count < MAX_COLLISION_RETRY_COUNT) {
    s3_log(S3_LOG_INFO, request_id,
           "Index ID collision happened for index %s\n",
           salted_index_name.c_str());
    // Handle Collision
    regenerate_new_index_name(base_index_name, salted_index_name);
    collision_attempt_count++;
    if (collision_attempt_count > 5) {
      s3_log(S3_LOG_INFO, request_id,
             "Index ID collision happened %d times for index %s\n",
             collision_attempt_count, salted_index_name.c_str());
    }
    callback();
  } else {
    if (collision_attempt_count >= MAX_COLLISION_RETRY_COUNT) {
      s3_log(S3_LOG_ERROR, request_id,
             "Failed to resolve index id collision %d times for index %s\n",
             collision_attempt_count, salted_index_name.c_str());
      s3_iem(LOG_ERR, S3_IEM_COLLISION_RES_FAIL, S3_IEM_COLLISION_RES_FAIL_STR,
             S3_IEM_COLLISION_RES_FAIL_JSON);
    }
    state = S3BucketMetadataState::failed;
    this->handler_on_failed();
  }
}

void S3BucketMetadata::regenerate_new_index_name(
    std::string base_index_name, std::string& salted_index_name) {
  salted_index_name = base_index_name + collision_salt +
                      std::to_string(collision_attempt_count);
}

void S3BucketMetadata::save_bucket_list_index_oid() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if (!account_user_index_metadata) {
    account_user_index_metadata =
        account_user_index_metadata_factory
            ->create_s3_account_user_idx_metadata(request);
  }
  account_user_index_metadata->set_bucket_list_index_oid(bucket_list_index_oid);

  account_user_index_metadata->save(
      std::bind(&S3BucketMetadata::save_bucket_list_index_oid_successful, this),
      std::bind(&S3BucketMetadata::save_bucket_list_index_oid_failed, this));

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadata::save_bucket_list_index_oid_successful() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if (current_op == S3BucketMetadataCurrentOp::fetching &&
      state == S3BucketMetadataState::missing) {
    // We just created bucket list container, so bucket metadata is missing
    // hence call failure callback
    this->handler_on_failed();
  } else if (current_op == S3BucketMetadataCurrentOp::saving) {
    collision_attempt_count = 0;
    create_object_list_index();
  } else {
    this->handler_on_success();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadata::save_bucket_list_index_oid_failed() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  s3_log(S3_LOG_ERROR, request_id,
         "Saving of Bucket list index oid metadata failed\n");
  if (account_user_index_metadata->get_state() ==
      S3AccountUserIdxMetadataState::failed_to_launch) {
    state = S3BucketMetadataState::failed_to_launch;
  } else {
    state = S3BucketMetadataState::failed;
  }
  this->handler_on_failed();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadata::save_bucket_info() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

  // Set up system attributes
  system_defined_attribute["Owner-User"] = user_name;
  system_defined_attribute["Owner-User-id"] = user_id;
  system_defined_attribute["Owner-Account"] = account_name;
  system_defined_attribute["Owner-Account-id"] = account_id;

  if (!clovis_kv_writer) {
    clovis_kv_writer = clovis_kvs_writer_factory->create_clovis_kvs_writer(
        request, s3_clovis_api);
  }
  clovis_kv_writer->put_keyval(
      bucket_list_index_oid, bucket_name, this->to_json(),
      std::bind(&S3BucketMetadata::save_bucket_info_successful, this),
      std::bind(&S3BucketMetadata::save_bucket_info_failed, this));

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadata::save_bucket_info_successful() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  this->handler_on_success();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadata::create_object_list_index() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if (!clovis_kv_writer) {
    clovis_kv_writer = clovis_kvs_writer_factory->create_clovis_kvs_writer(
        request, s3_clovis_api);
  }
  S3UriToMeroOID(s3_clovis_api, salted_object_list_index_name.c_str(),
                 request_id, &object_list_index_oid, S3ClovisEntityType::index);

  clovis_kv_writer->create_index_with_oid(
      object_list_index_oid,
      std::bind(&S3BucketMetadata::create_object_list_index_successful, this),
      std::bind(&S3BucketMetadata::create_object_list_index_failed, this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadata::create_multipart_list_index() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if (!clovis_kv_writer) {
    clovis_kv_writer = clovis_kvs_writer_factory->create_clovis_kvs_writer(
        request, s3_clovis_api);
  }

  S3UriToMeroOID(s3_clovis_api, salted_multipart_list_index_name.c_str(),
                 request_id, &multipart_index_oid, S3ClovisEntityType::index);

  clovis_kv_writer->create_index_with_oid(
      multipart_index_oid,
      std::bind(&S3BucketMetadata::create_multipart_list_index_successful,
                this),
      std::bind(&S3BucketMetadata::create_multipart_list_index_failed, this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadata::create_object_list_index_successful() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  collision_attempt_count = 0;
  create_multipart_list_index();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadata::create_object_list_index_failed() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if (clovis_kv_writer->get_state() == S3ClovisKVSWriterOpState::exists) {
    // create_object_list_index is called when there is no bucket,
    // Hence if clovis returned its present then its due to collision.
    handle_collision(
        get_object_list_index_name(), salted_object_list_index_name,
        std::bind(&S3BucketMetadata::create_object_list_index, this));
  } else if (clovis_kv_writer->get_state() ==
             S3ClovisKVSWriterOpState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id, "Object list Index creation failed.\n");
    state = S3BucketMetadataState::failed_to_launch;
    this->handler_on_failed();
  } else {
    s3_log(S3_LOG_ERROR, request_id, "Object list Index creation failed.\n");
    state = S3BucketMetadataState::failed;
    this->handler_on_failed();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadata::create_multipart_list_index_successful() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  save_bucket_info();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadata::create_multipart_list_index_failed() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  if (clovis_kv_writer->get_state() == S3ClovisKVSWriterOpState::exists) {
    // create_multipart_list_index is called when there is no bucket,
    // Hence if clovis returned its present then its due to collision.
    handle_collision(
        get_multipart_index_name(), salted_multipart_list_index_name,
        std::bind(&S3BucketMetadata::create_multipart_list_index, this));
  } else if (clovis_kv_writer->get_state() ==
             S3ClovisKVSWriterOpState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id, "Multipart list Index creation failed.\n");
    state = S3BucketMetadataState::failed_to_launch;
    this->handler_on_failed();
  } else {
    s3_log(S3_LOG_ERROR, request_id, "Multipart list Index creation failed.\n");
    state = S3BucketMetadataState::failed;
    this->handler_on_failed();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadata::save_bucket_info_failed() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  s3_log(S3_LOG_ERROR, request_id, "Saving of Bucket metadata failed\n");
  if (clovis_kv_writer->get_state() ==
      S3ClovisKVSWriterOpState::failed_to_launch) {
    state = S3BucketMetadataState::failed_to_launch;
  } else {
    state = S3BucketMetadataState::failed;
  }
  this->handler_on_failed();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadata::remove(std::function<void(void)> on_success,
                              std::function<void(void)> on_failed) {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;

  if (state == S3BucketMetadataState::present) {
    remove_bucket_info();
  } else {
    current_op = S3BucketMetadataCurrentOp::deleting;
    fetch_bucket_list_index_oid();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadata::remove_bucket_info() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");

  if (!clovis_kv_writer) {
    clovis_kv_writer = clovis_kvs_writer_factory->create_clovis_kvs_writer(
        request, s3_clovis_api);
  }
  clovis_kv_writer->delete_keyval(
      bucket_list_index_oid, bucket_name,
      std::bind(&S3BucketMetadata::remove_bucket_info_successful, this),
      std::bind(&S3BucketMetadata::remove_bucket_info_failed, this));

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadata::remove_bucket_info_successful() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  this->handler_on_success();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadata::remove_bucket_info_failed() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  s3_log(S3_LOG_ERROR, request_id, "Removal of Bucket metadata failed\n");
  if (clovis_kv_writer->get_state() ==
      S3ClovisKVSWriterOpState::failed_to_launch) {
    state = S3BucketMetadataState::failed_to_launch;
  } else {
    state = S3BucketMetadataState::failed;
  }
  this->handler_on_failed();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

std::string S3BucketMetadata::create_default_acl() {
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
  std::string xml_acl = bucket_ACL.get_xml_str();
  if (xml_acl == "") {
    xml_acl = create_default_acl();
  }
  root["ACL"] =
      base64_encode((const unsigned char*)xml_acl.c_str(), xml_acl.size());
  root["Policy"] = bucket_policy;
  for (const auto& tag : bucket_tags) {
    root["User-Defined-Tags"][tag.first] = tag.second;
  }

  root["mero_object_list_index_oid_u_hi"] = object_list_index_oid_u_hi_str =
      base64_encode((unsigned char const*)&object_list_index_oid.u_hi,
                    sizeof(object_list_index_oid.u_hi));
  root["mero_object_list_index_oid_u_lo"] = object_list_index_oid_u_lo_str =
      base64_encode((unsigned char const*)&object_list_index_oid.u_lo,
                    sizeof(object_list_index_oid.u_lo));

  root["mero_multipart_index_oid_u_hi"] =
      base64_encode((unsigned char const*)&multipart_index_oid.u_hi,
                    sizeof(multipart_index_oid.u_hi));
  root["mero_multipart_index_oid_u_lo"] =
      base64_encode((unsigned char const*)&multipart_index_oid.u_lo,
                    sizeof(multipart_index_oid.u_lo));

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
  object_list_index_oid_u_hi_str =
      newroot["mero_object_list_index_oid_u_hi"].asString();
  object_list_index_oid_u_lo_str =
      newroot["mero_object_list_index_oid_u_lo"].asString();

  std::string dec_object_list_index_oid_u_hi_str =
      base64_decode(object_list_index_oid_u_hi_str);
  std::string dec_object_list_index_oid_u_lo_str =
      base64_decode(object_list_index_oid_u_lo_str);

  std::string dec_multipart_index_oid_u_hi_str =
      base64_decode(newroot["mero_multipart_index_oid_u_hi"].asString());
  std::string dec_multipart_index_oid_u_lo_str =
      base64_decode(newroot["mero_multipart_index_oid_u_lo"].asString());

  memcpy((void*)&object_list_index_oid.u_hi,
         dec_object_list_index_oid_u_hi_str.c_str(),
         dec_object_list_index_oid_u_hi_str.length());
  memcpy((void*)&object_list_index_oid.u_lo,
         dec_object_list_index_oid_u_lo_str.c_str(),
         dec_object_list_index_oid_u_lo_str.length());

  memcpy((void*)&multipart_index_oid.u_hi,
         dec_multipart_index_oid_u_hi_str.c_str(),
         dec_multipart_index_oid_u_hi_str.length());
  memcpy((void*)&multipart_index_oid.u_lo,
         dec_multipart_index_oid_u_lo_str.c_str(),
         dec_multipart_index_oid_u_lo_str.length());

  bucket_ACL.from_json((newroot["ACL"]).asString());
  bucket_policy = newroot["Policy"].asString();

  members = newroot["User-Defined-Tags"].getMemberNames();
  for (const auto& tag : members) {
    bucket_tags[tag] = newroot["User-Defined-Tags"][tag].asString();
  }

  return 0;
}

std::string& S3BucketMetadata::get_encoded_bucket_acl() {
  // base64 acl encoded
  return bucket_ACL.get_acl_metadata();
}

std::string& S3BucketMetadata::get_acl_as_xml() {
  return bucket_ACL.get_xml_str();
}

std::string& S3BucketMetadata::get_policy_as_json() { return bucket_policy; }

std::string S3BucketMetadata::get_tags_as_xml() {

  s3_log(S3_LOG_INFO, request_id, "Entering\n");
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
  s3_log(S3_LOG_INFO, request_id, "Exiting\n");
  return tags_as_xml_str;
}

bool S3BucketMetadata::check_bucket_tags_exists() {
  return !bucket_tags.empty();
}
