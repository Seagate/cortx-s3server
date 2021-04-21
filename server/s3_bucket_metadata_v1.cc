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

#include "base64.h"
#include "s3_bucket_metadata_v1.h"
#include "s3_common_utilities.h"
#include "s3_datetime.h"
#include "s3_factory.h"
#include "s3_global_bucket_index_metadata.h"
#include "s3_iem.h"
#include "s3_log.h"
#include "s3_request_object.h"
#include "s3_stats.h"
#include "s3_uri_to_motr_oid.h"

extern struct m0_uint128 bucket_metadata_list_index_oid;

S3BucketMetadataV1::S3BucketMetadataV1(
    const S3BucketMetadata& tmplt, std::shared_ptr<MotrAPI> motr_api,
    std::shared_ptr<S3MotrKVSReaderFactory> motr_s3_kvs_reader_factory,
    std::shared_ptr<S3MotrKVSWriterFactory> motr_s3_kvs_writer_factory,
    std::shared_ptr<S3GlobalBucketIndexMetadataFactory>
        s3_global_bucket_index_metadata_factory)
    : S3BucketMetadata(tmplt) {

  s3_log(S3_LOG_DEBUG, request_id, "%s Ctor\n", __func__);

  if (motr_api) {
    s3_motr_api = std::move(motr_api);
  } else {
    s3_motr_api = std::make_shared<ConcreteMotrAPI>();
  }
  if (motr_s3_kvs_reader_factory) {
    motr_kvs_reader_factory = std::move(motr_s3_kvs_reader_factory);
  } else {
    motr_kvs_reader_factory = std::make_shared<S3MotrKVSReaderFactory>();
  }
  if (motr_s3_kvs_writer_factory) {
    motr_kvs_writer_factory = std::move(motr_s3_kvs_writer_factory);
  } else {
    motr_kvs_writer_factory = std::make_shared<S3MotrKVSWriterFactory>();
  }
  if (s3_global_bucket_index_metadata_factory) {
    global_bucket_index_metadata_factory =
        std::move(s3_global_bucket_index_metadata_factory);
  } else {
    global_bucket_index_metadata_factory =
        std::make_shared<S3GlobalBucketIndexMetadataFactory>();
  }
  salted_object_list_index_name = get_object_list_index_name();
  salted_multipart_list_index_name = get_multipart_index_name();
  salted_objects_version_list_index_name = get_version_list_index_name();
  extended_metadata_index_name = get_extended_metadata_index_name();
  collision_salt = "index_salt_";
}

S3BucketMetadataV1::~S3BucketMetadataV1() = default;

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

S3GlobalBucketIndexMetadata*
S3BucketMetadataV1::get_global_bucket_index_metadata() {
  if (!global_bucket_index_metadata) {
    global_bucket_index_metadata =
        global_bucket_index_metadata_factory
            ->create_s3_global_bucket_index_metadata(request, bucket_name,
                                                     account_id, account_name);
  }
  return global_bucket_index_metadata.get();
}

// Load {B1, A1} in global_bucket_list_index_oid, followed by {A1/B1, md} in
// bucket_metadata_list_index_oid
void S3BucketMetadataV1::load(const S3BucketMetadata& src,
                              CallbackHandlerType on_load) {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_timer.start();

  static_cast<S3BucketMetadata&>(*this) = src;
  callback = std::move(on_load);

  fetch_global_bucket_account_id_info();

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataV1::fetch_global_bucket_account_id_info() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  get_global_bucket_index_metadata()->load(
      std::bind(
          &S3BucketMetadataV1::fetch_global_bucket_account_id_info_success,
          this),
      std::bind(&S3BucketMetadataV1::fetch_global_bucket_account_id_info_failed,
                this));

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataV1::fetch_global_bucket_account_id_info_success() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  account_id = global_bucket_index_metadata->get_account_id();
  account_name = global_bucket_index_metadata->get_account_name();

  // Now fetch real bucket metadata
  load_bucket_info();

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataV1::fetch_global_bucket_account_id_info_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

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
  this->callback(state);

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataV1::load_bucket_info() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  motr_kv_reader =
      motr_kvs_reader_factory->create_motr_kvs_reader(request, s3_motr_api);
  // example key "AccountId_1/bucket_x'
  motr_kv_reader->get_keyval(
      bucket_metadata_list_index_oid, get_bucket_metadata_index_key_name(),
      std::bind(&S3BucketMetadataV1::load_bucket_info_successful, this),
      std::bind(&S3BucketMetadataV1::load_bucket_info_failed, this));

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataV1::load_bucket_info_successful() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  s3_timer.stop();
  const auto mss = s3_timer.elapsed_time_in_millisec();
  LOG_PERF("load_bucket_info_ms", request_id.c_str(), mss);
  s3_stats_timing("load_bucket_info", mss);

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
    state = S3BucketMetadataState::present;
    callback(state);
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataV1::load_bucket_info_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

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
  this->callback(state);

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

// Create {B1, A1} in global_bucket_list_index_oid, followed by {A1/B1, md} in
// bucket_metadata_list_index_oid
void S3BucketMetadataV1::save(const S3BucketMetadata& src,
                              CallbackHandlerType on_save) {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  static_cast<S3BucketMetadata&>(*this) = src;
  callback = std::move(on_save);
  assert(state == S3BucketMetadataState::missing);

  save_global_bucket_account_id_info();

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataV1::update(const S3BucketMetadata& src,
                                CallbackHandlerType on_update) {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  static_cast<S3BucketMetadata&>(*this) = src;
  callback = std::move(on_update);
  assert(state == S3BucketMetadataState::present);

  save_bucket_info(false);

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataV1::save_global_bucket_account_id_info() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  (void)get_global_bucket_index_metadata();

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

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataV1::save_global_bucket_account_id_info_successful() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  collision_attempt_count = 0;
  create_object_list_index();

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataV1::save_global_bucket_account_id_info_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  s3_log(S3_LOG_ERROR, request_id,
         "Saving of Bucket list index oid metadata failed\n");
  if (global_bucket_index_metadata->get_state() ==
      S3GlobalBucketIndexMetadataState::failed_to_launch) {
    state = S3BucketMetadataState::failed_to_launch;
  } else {
    state = S3BucketMetadataState::failed;
  }
  this->callback(state);

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataV1::create_object_list_index() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
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
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataV1::create_object_list_index_successful() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  collision_attempt_count = 0;
  create_multipart_list_index();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataV1::regenerate_new_index_name(
    const std::string& base_index_name, std::string& salted_index_name) {
  salted_index_name = base_index_name + collision_salt +
                      std::to_string(collision_attempt_count);
}

void S3BucketMetadataV1::handle_collision(const std::string& base_index_name,
                                          std::string& salted_index_name,
                                          std::function<void()> cb) {
  if (collision_attempt_count < MAX_COLLISION_RETRY_COUNT) {
    s3_log(S3_LOG_INFO, stripped_request_id,
           "Index ID collision happened for index %s\n",
           salted_index_name.c_str());
    // Handle Collision
    regenerate_new_index_name(base_index_name, salted_index_name);
    collision_attempt_count++;
    if (collision_attempt_count > 5) {
      s3_log(S3_LOG_INFO, stripped_request_id,
             "Index ID collision happened %d times for index %s\n",
             collision_attempt_count, salted_index_name.c_str());
    }
    cb();
  } else {
    if (collision_attempt_count >= MAX_COLLISION_RETRY_COUNT) {
      s3_log(S3_LOG_ERROR, request_id,
             "Failed to resolve index id collision %d times for index %s\n",
             collision_attempt_count, salted_index_name.c_str());
      // s3_iem(LOG_ERR, S3_IEM_COLLISION_RES_FAIL,
      // S3_IEM_COLLISION_RES_FAIL_STR,
      //     S3_IEM_COLLISION_RES_FAIL_JSON);
    }
    state = S3BucketMetadataState::failed;
    this->callback(state);
  }
}

void S3BucketMetadataV1::create_object_list_index_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (motr_kv_writer->get_state() == S3MotrKVSWriterOpState::exists) {
    // create_object_list_index is called when there is no bucket,
    // Hence if motr returned its present, then its due to collision.
    handle_collision(
        get_object_list_index_name(), salted_object_list_index_name,
        std::bind(&S3BucketMetadataV1::create_object_list_index, this));
  } else {
    s3_log(S3_LOG_ERROR, request_id, "Object list Index creation failed.\n");
    cleanup_on_create_err_global_bucket_account_id_info(
        motr_kv_writer->get_state());
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataV1::create_multipart_list_index() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
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
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataV1::create_multipart_list_index_successful() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  create_objects_version_list_index();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataV1::create_multipart_list_index_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (motr_kv_writer->get_state() == S3MotrKVSWriterOpState::exists) {
    // create_multipart_list_index is called when there is no bucket,
    // Hence if motr returned its present, then its due to collision.
    handle_collision(
        get_multipart_index_name(), salted_multipart_list_index_name,
        std::bind(&S3BucketMetadataV1::create_multipart_list_index, this));
  } else {
    s3_log(S3_LOG_ERROR, request_id, "Multipart list Index creation failed.\n");
    cleanup_on_create_err_global_bucket_account_id_info(
        motr_kv_writer->get_state());
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataV1::create_objects_version_list_index() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
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
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataV1::create_objects_version_list_index_successful() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  create_extended_metadata_index();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataV1::create_objects_version_list_index_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (motr_kv_writer->get_state() == S3MotrKVSWriterOpState::exists) {
    // create_objects_version_list_index is called when there is no bucket,
    // Hence if motr returned its present then its due to collision.
    handle_collision(
        get_version_list_index_name(), salted_objects_version_list_index_name,
        std::bind(&S3BucketMetadataV1::create_objects_version_list_index,
                  this));
  } else {
    s3_log(S3_LOG_ERROR, request_id,
           "Object version list Index creation failed.\n");
    cleanup_on_create_err_global_bucket_account_id_info(
        motr_kv_writer->get_state());
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataV1::create_extended_metadata_index() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (!motr_kv_writer) {
    motr_kv_writer =
        motr_kvs_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  // extended metadata list index oid
  S3UriToMotrOID(s3_motr_api, extended_metadata_index_name.c_str(), request_id,
                 &extended_metadata_index_oid, S3MotrEntityType::index);

  motr_kv_writer->create_index_with_oid(
      extended_metadata_index_oid,
      std::bind(&S3BucketMetadataV1::create_extended_metadata_index_successful,
                this),
      std::bind(&S3BucketMetadataV1::create_extended_metadata_index_failed,
                this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataV1::create_extended_metadata_index_successful() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  save_bucket_info(true);
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataV1::create_extended_metadata_index_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_log(S3_LOG_ERROR, request_id,
         "Extended metadata index creation failed.\n");
  cleanup_on_create_err_global_bucket_account_id_info(
      motr_kv_writer->get_state());
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataV1::save_bucket_info(bool clean_glob_on_err) {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  assert(!account_id.empty());
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
  should_cleanup_global_idx = clean_glob_on_err;
  motr_kv_writer->put_keyval(
      bucket_metadata_list_index_oid, get_bucket_metadata_index_key_name(),
      this->to_json(),
      std::bind(&S3BucketMetadataV1::save_bucket_info_successful, this),
      std::bind(&S3BucketMetadataV1::save_bucket_info_failed, this));

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataV1::save_bucket_info_successful() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  state = S3BucketMetadataState::present;
  this->callback(state);

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataV1::save_bucket_info_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_log(S3_LOG_ERROR, request_id, "Saving of Bucket metadata failed\n");
  auto kv_state = motr_kv_writer->get_state();
  if (!should_cleanup_global_idx) {
    if (kv_state == S3MotrKVSWriterOpState::failed_to_launch) {
      state = S3BucketMetadataState::failed_to_launch;
    } else {
      state = S3BucketMetadataState::failed;
    }
    this->callback(state);
  } else {
    cleanup_on_create_err_global_bucket_account_id_info(kv_state);
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

// Delete {A1/B1} from bucket_metadata_list_index_oid followed by {B1, A1} from
// global_bucket_list_index_oid
void S3BucketMetadataV1::remove(CallbackHandlerType on_remove) {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  assert(state == S3BucketMetadataState::present);
  callback = std::move(on_remove);

  remove_bucket_info();
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataV1::remove_bucket_info() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  if (!motr_kv_writer) {
    motr_kv_writer =
        motr_kvs_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }

  motr_kv_writer->delete_keyval(
      bucket_metadata_list_index_oid, get_bucket_metadata_index_key_name(),
      std::bind(&S3BucketMetadataV1::remove_bucket_info_successful, this),
      std::bind(&S3BucketMetadataV1::remove_bucket_info_failed, this));

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataV1::remove_bucket_info_successful() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  // If FI: 'kv_delete_failed_from_global_index' is set, then do not remove KV
  // from global_bucket_list_index_oid. This is to simulate possible "partial"
  // delete bucket action, where the KV got deleted from
  // bucket_metadata_list_index_oid, but not from global_bucket_list_index_oid.
  // If FI not set, then remove KV from global_bucket_list_index_oid as well.

  if (s3_fi_is_enabled("kv_delete_failed_from_global_index")) {
    remove_bucket_info_failed();
  } else {
    remove_global_bucket_account_id_info();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataV1::remove_bucket_info_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_log(S3_LOG_ERROR, request_id, "Removal of Bucket metadata failed\n");

  if (motr_kv_writer->get_state() == S3MotrKVSWriterOpState::failed_to_launch) {
    state = S3BucketMetadataState::failed_to_launch;
  } else {
    state = S3BucketMetadataState::failed;
  }
  this->callback(state);
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataV1::remove_global_bucket_account_id_info() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  get_global_bucket_index_metadata()->remove(
      std::bind(
          &S3BucketMetadataV1::remove_global_bucket_account_id_info_successful,
          this),
      std::bind(
          &S3BucketMetadataV1::remove_global_bucket_account_id_info_failed,
          this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataV1::remove_global_bucket_account_id_info_successful() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  state = S3BucketMetadataState::missing;
  this->callback(state);

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataV1::remove_global_bucket_account_id_info_failed() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  if (global_bucket_index_metadata->get_state() ==
      S3GlobalBucketIndexMetadataState::failed_to_launch) {
    state = S3BucketMetadataState::failed_to_launch;
  } else {
    state = S3BucketMetadataState::failed;
  }

  this->callback(state);
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataV1::cleanup_on_create_err_global_bucket_account_id_info(
    S3MotrKVSWriterOpState op_state) {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);

  if (op_state == S3MotrKVSWriterOpState::failed_to_launch) {
    on_cleanup_state = S3BucketMetadataState::failed_to_launch;
  } else {
    on_cleanup_state = S3BucketMetadataState::failed;
  }
  get_global_bucket_index_metadata()->remove(
      std::bind(
          &S3BucketMetadataV1::
               cleanup_on_create_err_global_bucket_account_id_info_fini_cb,
          this),
      std::bind(
          &S3BucketMetadataV1::
               cleanup_on_create_err_global_bucket_account_id_info_fini_cb,
          this));
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataV1::
    cleanup_on_create_err_global_bucket_account_id_info_fini_cb() {
  s3_log(S3_LOG_INFO, stripped_request_id, "%s Entry\n", __func__);
  s3_log(S3_LOG_ERROR, request_id,
         "cleanup_on_create_err_global_bucket_account_id_info status %d\n",
         static_cast<int>(global_bucket_index_metadata->get_state()));

  // Restore state before cleanup called
  state = on_cleanup_state;

  this->callback(state);
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}
