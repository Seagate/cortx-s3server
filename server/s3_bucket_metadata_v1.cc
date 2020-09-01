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

#include "s3_bucket_metadata_v1.h"
#include <json/json.h>
#include <string>
#include "base64.h"
#include "s3_datetime.h"
#include "s3_factory.h"
#include "s3_iem.h"
#include "s3_uri_to_motr_oid.h"
#include "s3_common_utilities.h"
#include "s3_stats.h"

extern struct m0_uint128 bucket_metadata_list_index_oid;
extern struct m0_uint128 replica_bucket_metadata_list_index_oid;

S3BucketMetadataV1::S3BucketMetadataV1(
    std::shared_ptr<S3RequestObject> req, std::shared_ptr<MotrAPI> motr_api,
    std::shared_ptr<S3MotrKVSReaderFactory> motr_s3_kvs_reader_factory,
    std::shared_ptr<S3MotrKVSWriterFactory> motr_s3_kvs_writer_factory,
    std::shared_ptr<S3GlobalBucketIndexMetadataFactory>
        s3_global_bucket_index_metadata_factory)
    : S3BucketMetadata(std::move(req), std::move(motr_api),
                       std::move(motr_s3_kvs_reader_factory),
                       std::move(motr_s3_kvs_writer_factory)) {
  s3_log(S3_LOG_DEBUG, request_id, "Constructor");

  if (s3_global_bucket_index_metadata_factory) {
    global_bucket_index_metadata_factory =
        std::move(s3_global_bucket_index_metadata_factory);
  } else {
    global_bucket_index_metadata_factory =
        std::make_shared<S3GlobalBucketIndexMetadataFactory>();
  }
  bucket_owner_account_id = "";
  // name of the index which holds all objects key values within a bucket
  salted_object_list_index_name = get_object_list_index_name();
}

struct m0_uint128 S3BucketMetadataV1::get_bucket_metadata_list_index_oid() {
  return bucket_metadata_list_index_oid;
}

void S3BucketMetadataV1::set_bucket_metadata_list_index_oid(
    struct m0_uint128 oid) {
  bucket_metadata_list_index_oid = oid;
}

void S3BucketMetadataV1::set_state(S3BucketMetadataState state) {
  this->state = state;
}

// Load {B1, A1} in global_bucket_list_index_oid, followed by {A1/B1, md} in
// bucket_metadata_list_index_oid
void S3BucketMetadataV1::load(std::function<void(void)> on_success,
                              std::function<void(void)> on_failed) {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  s3_timer.start();

  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;

  fetch_global_bucket_account_id_info();

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadataV1::fetch_global_bucket_account_id_info() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  if (!global_bucket_index_metadata) {
    global_bucket_index_metadata =
        global_bucket_index_metadata_factory
            ->create_s3_global_bucket_index_metadata(request);
  }
  global_bucket_index_metadata->load(
      std::bind(
          &S3BucketMetadataV1::fetch_global_bucket_account_id_info_success,
          this),
      std::bind(&S3BucketMetadataV1::fetch_global_bucket_account_id_info_failed,
                this));

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadataV1::fetch_global_bucket_account_id_info_success() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  bucket_owner_account_id = global_bucket_index_metadata->get_account_id();
  // Now fetch real bucket metadata
  load_bucket_info();

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadataV1::fetch_global_bucket_account_id_info_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  if (global_bucket_index_metadata->get_state() ==
      S3GlobalBucketIndexMetadataState::missing) {
    state = S3BucketMetadataState::missing;
  } else if (global_bucket_index_metadata->get_state() ==
             S3GlobalBucketIndexMetadataState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id,
           "Failed to fetch account and bucket information from global bucket "
           "list. Please retry after some time\n");
    state = S3BucketMetadataState::failed_to_launch;
  } else {
    s3_log(S3_LOG_ERROR, request_id,
           "Failed to fetch accound and bucket informoation from global bucket "
           "list. Please retry after some time\n");
    state = S3BucketMetadataState::failed;
  }
  this->handler_on_failed();

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadataV1::load_bucket_info() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  motr_kv_reader =
      motr_kvs_reader_factory->create_motr_kvs_reader(request, s3_motr_api);
  // example key "AccountId_1/bucket_x'
  motr_kv_reader->get_keyval(
      bucket_metadata_list_index_oid, get_bucket_metadata_index_key_name(),
      std::bind(&S3BucketMetadataV1::load_bucket_info_successful, this),
      std::bind(&S3BucketMetadataV1::load_bucket_info_failed, this));

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadataV1::load_bucket_info_successful() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (this->from_json(motr_kv_reader->get_value()) != 0) {
    s3_log(S3_LOG_ERROR, request_id,
           "Json Parsing failed. Index oid ="
           "%" SCNx64 " : %" SCNx64 ", Key = %s Value = %s\n",
           bucket_metadata_list_index_oid.u_hi,
           bucket_metadata_list_index_oid.u_lo, bucket_name.c_str(),
           motr_kv_reader->get_value().c_str());
    s3_iem(LOG_ERR, S3_IEM_METADATA_CORRUPTED, S3_IEM_METADATA_CORRUPTED_STR,
           S3_IEM_METADATA_CORRUPTED_JSON);

    json_parsing_error = true;
    load_bucket_info_failed();
  } else {
    s3_timer.stop();
    const auto mss = s3_timer.elapsed_time_in_millisec();
    LOG_PERF("load_bucket_info_ms", request_id.c_str(), mss);
    s3_stats_timing("load_bucket_info", mss);

    state = S3BucketMetadataState::present;
    this->handler_on_success();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadataV1::load_bucket_info_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  if (json_parsing_error) {
    state = S3BucketMetadataState::failed;
  } else if (motr_kv_reader->get_state() == S3MotrKVSReaderOpState::missing) {
    s3_log(S3_LOG_DEBUG, request_id, "Bucket metadata is missing\n");
    state = S3BucketMetadataState::missing;
  } else if (motr_kv_reader->get_state() ==
             S3MotrKVSReaderOpState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id, "Loading of bucket metadata failed\n");
    state = S3BucketMetadataState::failed_to_launch;
  } else {
    s3_log(S3_LOG_ERROR, request_id, "Loading of bucket metadata failed\n");
    state = S3BucketMetadataState::failed;
  }
  this->handler_on_failed();

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

// Create {B1, A1} in global_bucket_list_index_oid, followed by {A1/B1, md} in
// bucket_metadata_list_index_oid
void S3BucketMetadataV1::save(std::function<void(void)> on_success,
                              std::function<void(void)> on_failed) {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  assert(state == S3BucketMetadataState::missing);

  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;

  global_bucket_index_metadata.reset();
  save_global_bucket_account_id_info();

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadataV1::update(std::function<void(void)> on_success,
                                std::function<void(void)> on_failed) {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  assert(state == S3BucketMetadataState::present);

  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;

  save_bucket_info();

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadataV1::save_global_bucket_account_id_info() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (!global_bucket_index_metadata) {
    global_bucket_index_metadata =
        global_bucket_index_metadata_factory
            ->create_s3_global_bucket_index_metadata(request);
  }
  assert(!(global_bucket_index_metadata->get_account_id().empty()));
  assert(!(global_bucket_index_metadata->get_account_name().empty()));

  // set location_constraint attributes & save
  global_bucket_index_metadata->set_location_constraint(
      system_defined_attribute["LocationConstraint"]);
  // save account and bucket information
  // will be retrived from request object, no need to pass
  global_bucket_index_metadata->save(
      std::bind(
          &S3BucketMetadataV1::save_global_bucket_account_id_info_successful,
          this),
      std::bind(&S3BucketMetadataV1::save_global_bucket_account_id_info_failed,
                this));

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadataV1::save_global_bucket_account_id_info_successful() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  // update bucket_owner_account_id
  bucket_owner_account_id = global_bucket_index_metadata->get_account_id();
  collision_attempt_count = 0;
  create_object_list_index();

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadataV1::save_global_bucket_account_id_info_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  s3_log(S3_LOG_ERROR, request_id,
         "Saving of Bucket list index oid metadata failed\n");
  if (global_bucket_index_metadata->get_state() ==
      S3GlobalBucketIndexMetadataState::failed_to_launch) {
    state = S3BucketMetadataState::failed_to_launch;
  } else {
    state = S3BucketMetadataState::failed;
  }
  this->handler_on_failed();

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadataV1::create_object_list_index() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (!motr_kv_writer) {
    motr_kv_writer =
        motr_kvs_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  S3UriToMotrOID(s3_motr_api, salted_object_list_index_name.c_str(), request_id,
                 &object_list_index_oid, S3MotrEntityType::index);

  motr_kv_writer->create_index_with_oid(
      object_list_index_oid,
      std::bind(&S3BucketMetadataV1::create_object_list_index_successful, this),
      std::bind(&S3BucketMetadataV1::create_object_list_index_failed, this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadataV1::create_object_list_index_successful() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  collision_attempt_count = 0;
  create_multipart_list_index();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadataV1::create_object_list_index_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (motr_kv_writer->get_state() == S3MotrKVSWriterOpState::exists) {
    // create_object_list_index is called when there is no bucket,
    // Hence if motr returned its present, then its due to collision.
    handle_collision(
        get_object_list_index_name(), salted_object_list_index_name,
        std::bind(&S3BucketMetadataV1::create_object_list_index, this));
  } else if (motr_kv_writer->get_state() ==
             S3MotrKVSWriterOpState::failed_to_launch) {
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

void S3BucketMetadataV1::create_multipart_list_index() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (!motr_kv_writer) {
    motr_kv_writer =
        motr_kvs_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }

  S3UriToMotrOID(s3_motr_api, salted_multipart_list_index_name.c_str(),
                 request_id, &multipart_index_oid, S3MotrEntityType::index);

  motr_kv_writer->create_index_with_oid(
      multipart_index_oid,
      std::bind(&S3BucketMetadataV1::create_multipart_list_index_successful,
                this),
      std::bind(&S3BucketMetadataV1::create_multipart_list_index_failed, this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadataV1::create_multipart_list_index_successful() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  create_objects_version_list_index();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadataV1::create_multipart_list_index_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (motr_kv_writer->get_state() == S3MotrKVSWriterOpState::exists) {
    // create_multipart_list_index is called when there is no bucket,
    // Hence if motr returned its present, then its due to collision.
    handle_collision(
        get_multipart_index_name(), salted_multipart_list_index_name,
        std::bind(&S3BucketMetadataV1::create_multipart_list_index, this));
  } else if (motr_kv_writer->get_state() ==
             S3MotrKVSWriterOpState::failed_to_launch) {
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

void S3BucketMetadataV1::create_objects_version_list_index() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (!motr_kv_writer) {
    motr_kv_writer =
        motr_kvs_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  // version list index oid
  S3UriToMotrOID(s3_motr_api, salted_objects_version_list_index_name.c_str(),
                 request_id, &objects_version_list_index_oid,
                 S3MotrEntityType::index);

  motr_kv_writer->create_index_with_oid(
      objects_version_list_index_oid,
      std::bind(
          &S3BucketMetadataV1::create_objects_version_list_index_successful,
          this),
      std::bind(&S3BucketMetadataV1::create_objects_version_list_index_failed,
                this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadataV1::create_objects_version_list_index_successful() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  collision_attempt_count = 0;
  save_bucket_info();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadataV1::create_objects_version_list_index_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (motr_kv_writer->get_state() == S3MotrKVSWriterOpState::exists) {
    // create_objects_version_list_index is called when there is no bucket,
    // Hence if motr returned its present then its due to collision.
    handle_collision(
        get_version_list_index_name(), salted_objects_version_list_index_name,
        std::bind(&S3BucketMetadataV1::create_objects_version_list_index,
                  this));
  } else if (motr_kv_writer->get_state() ==
             S3MotrKVSWriterOpState::failed_to_launch) {
    s3_log(S3_LOG_ERROR, request_id,
           "Object version list Index creation failed.\n");
    state = S3BucketMetadataState::failed_to_launch;
    this->handler_on_failed();
  } else {
    s3_log(S3_LOG_ERROR, request_id,
           "Object version list Index creation failed.\n");
    state = S3BucketMetadataState::failed;
    this->handler_on_failed();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadataV1::save_bucket_info() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  assert(!bucket_owner_account_id.empty());
  assert(!user_name.empty());
  assert(!user_id.empty());
  assert(!account_name.empty());
  assert(!account_id.empty());
  // Set up system attributes
  system_defined_attribute["Owner-User"] = user_name;
  system_defined_attribute["Owner-User-id"] = user_id;
  system_defined_attribute["Owner-Account"] = account_name;
  system_defined_attribute["Owner-Account-id"] = account_id;

  if (!motr_kv_writer) {
    motr_kv_writer =
        motr_kvs_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->put_keyval(
      bucket_metadata_list_index_oid, get_bucket_metadata_index_key_name(),
      this->to_json(),
      std::bind(&S3BucketMetadataV1::save_bucket_info_successful, this),
      std::bind(&S3BucketMetadataV1::save_bucket_info_failed, this));

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadataV1::save_bucket_info_successful() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  // attempt to save the KV in replica bucket metadata index
  if (!motr_kv_writer) {
    motr_kv_writer =
        motr_kvs_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->put_keyval(
      replica_bucket_metadata_list_index_oid,
      get_bucket_metadata_index_key_name(), this->to_json(),
      std::bind(&S3BucketMetadataV1::save_replica, this),
      std::bind(&S3BucketMetadataV1::save_replica, this));

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadataV1::save_replica() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  // PUT Operation passes even if we failed to put KV in replica index.
  if (motr_kv_writer->get_state() != S3MotrKVSWriterOpState::created) {
    s3_log(S3_LOG_ERROR, request_id, "Failed to save KV in replica index.\n");

    s3_iem_syslog(LOG_INFO, S3_IEM_METADATA_CORRUPTED,
                  "Failed to save metadata in replica index for bucket: %s",
                  get_bucket_metadata_index_key_name().c_str());
  }
  this->handler_on_success();

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadataV1::save_bucket_info_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  s3_log(S3_LOG_ERROR, request_id, "Saving of Bucket metadata failed\n");
  if (motr_kv_writer->get_state() == S3MotrKVSWriterOpState::failed_to_launch) {
    state = S3BucketMetadataState::failed_to_launch;
  } else {
    state = S3BucketMetadataState::failed;
  }
  this->handler_on_failed();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

// Delete {A1/B1} from bucket_metadata_list_index_oid followed by {B1, A1} from
// global_bucket_list_index_oid
void S3BucketMetadataV1::remove(std::function<void(void)> on_success,
                                std::function<void(void)> on_failed) {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;

  assert(state == S3BucketMetadataState::present);
  remove_bucket_info();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadataV1::remove_bucket_info() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  if (!motr_kv_writer) {
    motr_kv_writer =
        motr_kvs_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }

  motr_kv_writer->delete_keyval(
      bucket_metadata_list_index_oid, get_bucket_metadata_index_key_name(),
      std::bind(&S3BucketMetadataV1::remove_bucket_info_successful, this),
      std::bind(&S3BucketMetadataV1::remove_bucket_info_failed, this));

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadataV1::remove_bucket_info_successful() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  // Attempt to remove KV from replica bucket metadata index
  if (!motr_kv_writer) {
    motr_kv_writer =
        motr_kvs_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->delete_keyval(
      replica_bucket_metadata_list_index_oid,
      get_bucket_metadata_index_key_name(),
      std::bind(&S3BucketMetadataV1::remove_replica, this),
      std::bind(&S3BucketMetadataV1::remove_replica, this));

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadataV1::remove_replica() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  // delete KV from replia index is not fatal error
  if (motr_kv_writer->get_state() != S3MotrKVSWriterOpState::deleted) {
    s3_log(S3_LOG_ERROR, request_id,
           "Removal of Bucket metadata from replica index failed\n");
    s3_iem_syslog(LOG_INFO, S3_IEM_METADATA_CORRUPTED,
                  "Failed to remove KV from replica index of bucket: %s",
                  get_bucket_metadata_index_key_name().c_str());
  }
  // If FI: 'kv_delete_failed_from_global_index' is set, then do not remove KV
  // from global_bucket_list_index_oid. This is to simulate possible "partial"
  // delete bucket action, where the KV got deleted from
  // bucket_metadata_list_index_oid, but not from global_bucket_list_index_oid.
  // If FI not set, then remove KV from global_bucket_list_index_oid as well.
  if (!s3_fi_is_enabled("kv_delete_failed_from_global_index")) {
    remove_global_bucket_account_id_info();
  }

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadataV1::remove_bucket_info_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  s3_log(S3_LOG_ERROR, request_id, "Removal of Bucket metadata failed\n");
  if (motr_kv_writer->get_state() == S3MotrKVSWriterOpState::failed_to_launch) {
    state = S3BucketMetadataState::failed_to_launch;
  } else {
    state = S3BucketMetadataState::failed;
  }
  this->handler_on_failed();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadataV1::remove_global_bucket_account_id_info() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  global_bucket_index_metadata->remove(
      std::bind(
          &S3BucketMetadataV1::remove_global_bucket_account_id_info_successful,
          this),
      std::bind(
          &S3BucketMetadataV1::remove_global_bucket_account_id_info_failed,
          this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadataV1::remove_global_bucket_account_id_info_successful() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  this->handler_on_success();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3BucketMetadataV1::remove_global_bucket_account_id_info_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (global_bucket_index_metadata->get_state() ==
      S3GlobalBucketIndexMetadataState::failed_to_launch) {
    state = S3BucketMetadataState::failed_to_launch;
  } else {
    state = S3BucketMetadataState::failed;
  }

  this->handler_on_failed();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}
