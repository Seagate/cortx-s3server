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

#include <utility>

#include "s3_factory.h"
#include "s3_iem.h"
#include "s3_log.h"
#include "s3_m0_uint128_helper.h"
#include "s3_motr_kvs_writer.h"
#include "s3_motr_writer.h"
#include "s3_option.h"
#include "s3_probable_delete_record.h"
#include "s3_put_object_action_base.h"
#include "s3_uri_to_motr_oid.h"

extern struct m0_uint128 global_probable_dead_object_list_index_oid;

S3PutObjectActionBase::S3PutObjectActionBase(
    std::shared_ptr<S3RequestObject> s3_request_object,
    std::shared_ptr<S3BucketMetadataFactory> bucket_meta_factory,
    std::shared_ptr<S3ObjectMetadataFactory> object_meta_factory,
    std::shared_ptr<MotrAPI> motr_api,
    std::shared_ptr<S3MotrWriterFactory> motr_s3_factory,
    std::shared_ptr<S3MotrKVSWriterFactory> kv_writer_factory)
    : S3ObjectAction(std::move(s3_request_object),
                     std::move(bucket_meta_factory),
                     std::move(object_meta_factory)) {

  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");

  if (motr_api) {
    s3_motr_api = std::move(motr_api);
  } else {
    s3_motr_api = std::make_shared<ConcreteMotrAPI>();
  }
  if (motr_s3_factory) {
    motr_writer_factory = std::move(motr_s3_factory);
  } else {
    motr_writer_factory = std::make_shared<S3MotrWriterFactory>();
  }
  if (kv_writer_factory) {
    mote_kv_writer_factory = std::move(kv_writer_factory);
  } else {
    mote_kv_writer_factory = std::make_shared<S3MotrKVSWriterFactory>();
  }
}

void S3PutObjectActionBase::_set_layout_id(int layout_id) {
  assert(layout_id > 0 && layout_id < 15);
  this->layout_id = layout_id;

  motr_write_payload_size =
      S3Option::get_instance()->get_motr_write_payload_size(layout_id);
  assert(motr_write_payload_size > 0);

  motr_unit_size =
      S3MotrLayoutMap::get_instance()->get_unit_size_for_layout(layout_id);
  assert(motr_unit_size > 0);
}

void S3PutObjectActionBase::fetch_bucket_info_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  s3_put_action_state = S3PutObjectActionState::validationFailed;

  switch (bucket_metadata->get_state()) {
    case S3BucketMetadataState::missing:
      s3_log(S3_LOG_DEBUG, request_id, "Bucket not found");
      set_s3_error("NoSuchBucket");
      break;
    case S3BucketMetadataState::failed_to_launch:
      s3_log(S3_LOG_ERROR, request_id,
             "Bucket metadata load operation failed due to pre launch failure");
      set_s3_error("ServiceUnavailable");
      break;
    default:
      s3_log(S3_LOG_DEBUG, request_id, "Bucket metadata fetch failed");
      set_s3_error("InternalError");
  }
  send_response_to_s3_client();

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectActionBase::fetch_object_info_success() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  const auto metadata_state = object_metadata->get_state();

  if (metadata_state == S3ObjectMetadataState::missing) {
    s3_log(S3_LOG_DEBUG, request_id, "Destination object is absent");
    next();
  } else if (metadata_state == S3ObjectMetadataState::present) {
    s3_log(S3_LOG_DEBUG, request_id, "Destination object already exists");

    old_object_oid = object_metadata->get_oid();
    old_oid_str = S3M0Uint128Helper::to_string(old_object_oid);
    old_layout_id = object_metadata->get_layout_id();

    create_new_oid(old_object_oid);
    next();
  } else {
    s3_put_action_state = S3PutObjectActionState::validationFailed;

    if (metadata_state == S3ObjectMetadataState::failed_to_launch) {
      s3_log(S3_LOG_ERROR, request_id,
             "Object metadata load operation failed due to pre launch failure");
      set_s3_error("ServiceUnavailable");
    } else {
      s3_log(S3_LOG_ERROR, request_id, "Failed to look up metadata.");
      set_s3_error("InternalError");
    }
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectActionBase::fetch_object_info_failed() {
  // Proceed to to next task, object metadata doesnt exist, will create now
  struct m0_uint128 object_list_oid =
      bucket_metadata->get_object_list_index_oid();
  if (!(object_list_oid.u_hi | object_list_oid.u_lo) ||
      !(objects_version_list_oid.u_hi | objects_version_list_oid.u_lo)) {
    // Rare/unlikely: Motr KVS data corruption:
    // object_list_oid/objects_version_list_oid is null only when bucket
    // metadata is corrupted.
    // user has to delete and recreate the bucket again to make it work.
    s3_log(S3_LOG_ERROR, request_id, "Bucket(%s) metadata is corrupted.\n",
           request->get_bucket_name().c_str());
    s3_iem(LOG_ERR, S3_IEM_METADATA_CORRUPTED, S3_IEM_METADATA_CORRUPTED_STR,
           S3_IEM_METADATA_CORRUPTED_JSON);
    s3_put_action_state = S3PutObjectActionState::validationFailed;
    set_s3_error("MetaDataCorruption");
    send_response_to_s3_client();
  } else {
    next();
  }
}

void S3PutObjectActionBase::create_object() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  if (!tried_count) {
    motr_writer = motr_writer_factory->create_motr_writer(request);
  }
  _set_layout_id(S3MotrLayoutMap::get_instance()->get_layout_for_object_size(
      total_data_to_stream));

  motr_writer->create_object(
      std::bind(&S3PutObjectActionBase::create_object_successful, this),
      std::bind(&S3PutObjectActionBase::create_object_failed, this),
      new_object_oid, layout_id);

  // for shutdown testcases, check FI and set shutdown signal
  S3_CHECK_FI_AND_SET_SHUTDOWN_SIGNAL(
      "put_object_action_create_object_shutdown_fail");
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectActionBase::create_object_successful() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  s3_put_action_state = S3PutObjectActionState::newObjOidCreated;

  // New Object or overwrite, create new metadata and release old.
  new_object_metadata = object_metadata_factory->create_object_metadata_obj(
      request, bucket_metadata->get_object_list_index_oid());
  new_object_metadata->set_objects_version_list_index_oid(
      bucket_metadata->get_objects_version_list_index_oid());

  new_oid_str = S3M0Uint128Helper::to_string(new_object_oid);

  // Generate a version id for the new object.
  new_object_metadata->regenerate_version_id();
  new_object_metadata->set_oid(motr_writer->get_oid());
  new_object_metadata->set_layout_id(layout_id);

  add_object_oid_to_probable_dead_oid_list();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectActionBase::create_object_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, "", "Exiting\n");
    return;
  }
  if (motr_writer->get_state() == S3MotrWiterOpState::exists) {
    collision_detected();
  } else {
    s3_put_action_state = S3PutObjectActionState::newObjOidCreationFailed;

    if (motr_writer->get_state() == S3MotrWiterOpState::failed_to_launch) {
      s3_log(S3_LOG_ERROR, request_id, "Create object failed.\n");
      set_s3_error("ServiceUnavailable");
    } else {
      s3_log(S3_LOG_WARN, request_id, "Create object failed.\n");
      // Any other error report failure.
      set_s3_error("InternalError");
    }
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectActionBase::collision_detected() {
  if (check_shutdown_and_rollback()) {
    s3_log(S3_LOG_DEBUG, NULL, "Shutdown or rollback");

  } else if (tried_count < MAX_COLLISION_RETRY_COUNT) {
    s3_log(S3_LOG_INFO, request_id, "Object ID collision happened for uri %s\n",
           request->get_object_uri().c_str());
    // Handle Collision
    create_new_oid(new_object_oid);
    ++tried_count;
    if (tried_count > 5) {
      s3_log(S3_LOG_INFO, request_id,
             "Object ID collision happened %d times for uri %s\n", tried_count,
             request->get_object_uri().c_str());
    }
    create_object();
  } else {
    s3_log(S3_LOG_ERROR, request_id,
           "Exceeded maximum collision retry attempts."
           "Collision occurred %d times for uri %s\n",
           tried_count, request->get_object_uri().c_str());
    s3_iem(LOG_ERR, S3_IEM_COLLISION_RES_FAIL, S3_IEM_COLLISION_RES_FAIL_STR,
           S3_IEM_COLLISION_RES_FAIL_JSON);
    s3_put_action_state = S3PutObjectActionState::newObjOidCreationFailed;
    set_s3_error("InternalError");
    send_response_to_s3_client();
  }
  s3_log(S3_LOG_DEBUG, NULL, "Exiting");
}

void S3PutObjectActionBase::create_new_oid(struct m0_uint128 current_oid) {
  unsigned salt_counter = 0;
  std::string salted_uri;

  do {
    salted_uri = request->get_object_uri() + "uri_salt_" +
                 std::to_string(salt_counter) + std::to_string(tried_count);
    S3UriToMotrOID(s3_motr_api, salted_uri.c_str(), request_id,
                   &new_object_oid);
    ++salt_counter;
  } while ((new_object_oid.u_hi == current_oid.u_hi) &&
           (new_object_oid.u_lo == current_oid.u_lo));
}

void S3PutObjectActionBase::add_object_oid_to_probable_dead_oid_list() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  std::map<std::string, std::string> probable_oid_list;
  assert(!new_oid_str.empty());

  // store old object oid
  if (old_object_oid.u_hi | old_object_oid.u_lo) {
    assert(!old_oid_str.empty());

    // key = oldoid + "-" + newoid
    std::string old_oid_rec_key = old_oid_str + '-' + new_oid_str;
    s3_log(S3_LOG_DEBUG, request_id,
           "Adding old_probable_del_rec with key [%s]\n",
           old_oid_rec_key.c_str());
    old_probable_del_rec.reset(new S3ProbableDeleteRecord(
        old_oid_rec_key, {0, 0}, object_metadata->get_object_name(),
        old_object_oid, old_layout_id,
        bucket_metadata->get_object_list_index_oid(),
        bucket_metadata->get_objects_version_list_index_oid(),
        object_metadata->get_version_key_in_index(), false /* force_delete */));

    probable_oid_list[old_oid_rec_key] = old_probable_del_rec->to_json();
  }

  s3_log(S3_LOG_DEBUG, request_id,
         "Adding new_probable_del_rec with key [%s]\n", new_oid_str.c_str());
  new_probable_del_rec.reset(new S3ProbableDeleteRecord(
      new_oid_str, old_object_oid, new_object_metadata->get_object_name(),
      new_object_oid, layout_id, bucket_metadata->get_object_list_index_oid(),
      bucket_metadata->get_objects_version_list_index_oid(),
      new_object_metadata->get_version_key_in_index(),
      false /* force_delete */));

  // store new oid, key = newoid
  probable_oid_list[new_oid_str] = new_probable_del_rec->to_json();

  if (!motr_kv_writer) {
    motr_kv_writer =
        mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->put_keyval(
      global_probable_dead_object_list_index_oid, probable_oid_list,
      std::bind(&S3PutObjectActionBase::next, this),
      std::bind(&S3PutObjectActionBase::
                     add_object_oid_to_probable_dead_oid_list_failed,
                this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectActionBase::add_object_oid_to_probable_dead_oid_list_failed() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  s3_put_action_state = S3PutObjectActionState::probableEntryRecordFailed;
  if (motr_kv_writer->get_state() == S3MotrKVSWriterOpState::failed_to_launch) {
    set_s3_error("ServiceUnavailable");
  } else {
    set_s3_error("InternalError");
  }
  // Clean up will be done after response.
  send_response_to_s3_client();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectActionBase::startcleanup() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  // TODO: Perf - all/some of below tasks can be done in parallel
  // Any of the following steps fail, backgrounddelete will be able to perform
  // cleanups.
  // Clear task list and setup cleanup task list
  clear_tasks();
  cleanup_started = true;

  // Success conditions
  if (s3_put_action_state == S3PutObjectActionState::completed) {
    s3_log(S3_LOG_DEBUG, request_id, "Cleanup old Object\n");
    if (old_object_oid.u_hi || old_object_oid.u_lo) {
      // mark old OID for deletion in overwrite case, this optimizes
      // backgrounddelete decisions.
      ACTION_TASK_ADD(S3PutObjectActionBase::mark_old_oid_for_deletion, this);
    }
    // remove new oid from probable delete list.
    ACTION_TASK_ADD(S3PutObjectActionBase::remove_new_oid_probable_record,
                    this);
    if (old_object_oid.u_hi || old_object_oid.u_lo) {
      // Object overwrite case, old object exists, delete it.
      ACTION_TASK_ADD(S3PutObjectActionBase::delete_old_object, this);
      // If delete object is successful, attempt to delete old probable record
    }
  } else if (s3_put_action_state == S3PutObjectActionState::newObjOidCreated ||
             s3_put_action_state == S3PutObjectActionState::writeFailed ||
             s3_put_action_state ==
                 S3PutObjectActionState::md5ValidationFailed ||
             s3_put_action_state ==
                 S3PutObjectActionState::metadataSaveFailed) {
    // PUT is assumed to be failed with a need to rollback
    s3_log(S3_LOG_DEBUG, request_id,
           "Cleanup new Object: s3_put_action_state[%d]\n",
           s3_put_action_state);
    // Mark new OID for deletion, this optimizes backgrounddelete decisionss.
    ACTION_TASK_ADD(S3PutObjectActionBase::mark_new_oid_for_deletion, this);
    if (old_object_oid.u_hi || old_object_oid.u_lo) {
      // remove old oid from probable delete list.
      ACTION_TASK_ADD(S3PutObjectActionBase::remove_old_oid_probable_record,
                      this);
    }
    ACTION_TASK_ADD(S3PutObjectActionBase::delete_new_object, this);
    // If delete object is successful, attempt to delete new probable record
  } else {
    s3_log(S3_LOG_DEBUG, request_id,
           "No Cleanup required: s3_put_action_state[%d]\n",
           s3_put_action_state);
    assert(s3_put_action_state == S3PutObjectActionState::empty ||
           s3_put_action_state == S3PutObjectActionState::validationFailed ||
           s3_put_action_state ==
               S3PutObjectActionState::probableEntryRecordFailed ||
           s3_put_action_state ==
               S3PutObjectActionState::newObjOidCreationFailed);
    // Nothing to undo
  }

  // Start running the cleanup task list
  start();
}

void S3PutObjectActionBase::mark_new_oid_for_deletion() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  assert(!new_oid_str.empty());

  // update new oid, key = newoid, force_del = true
  new_probable_del_rec->set_force_delete(true);

  if (!motr_kv_writer) {
    motr_kv_writer =
        mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->put_keyval(global_probable_dead_object_list_index_oid,
                             new_oid_str, new_probable_del_rec->to_json(),
                             std::bind(&S3PutObjectActionBase::next, this),
                             std::bind(&S3PutObjectActionBase::next, this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectActionBase::mark_old_oid_for_deletion() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  assert(!old_oid_str.empty());
  assert(!new_oid_str.empty());

  // key = oldoid + "-" + newoid
  std::string old_oid_rec_key = old_oid_str + '-' + new_oid_str;

  // update old oid, force_del = true
  old_probable_del_rec->set_force_delete(true);

  if (!motr_kv_writer) {
    motr_kv_writer =
        mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->put_keyval(global_probable_dead_object_list_index_oid,
                             old_oid_rec_key, old_probable_del_rec->to_json(),
                             std::bind(&S3PutObjectActionBase::next, this),
                             std::bind(&S3PutObjectActionBase::next, this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectActionBase::remove_old_oid_probable_record() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  assert(!old_oid_str.empty());
  assert(!new_oid_str.empty());

  // key = oldoid + "-" + newoid
  std::string old_oid_rec_key = old_oid_str + '-' + new_oid_str;

  if (!motr_kv_writer) {
    motr_kv_writer =
        mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->delete_keyval(global_probable_dead_object_list_index_oid,
                                old_oid_rec_key,
                                std::bind(&S3PutObjectActionBase::next, this),
                                std::bind(&S3PutObjectActionBase::next, this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectActionBase::remove_new_oid_probable_record() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  assert(!new_oid_str.empty());

  if (!motr_kv_writer) {
    motr_kv_writer =
        mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }
  motr_kv_writer->delete_keyval(global_probable_dead_object_list_index_oid,
                                new_oid_str,
                                std::bind(&S3PutObjectActionBase::next, this),
                                std::bind(&S3PutObjectActionBase::next, this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectActionBase::delete_old_object() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  // If PUT is success, we delete old object if present
  assert(old_object_oid.u_hi != 0ULL || old_object_oid.u_lo != 0ULL);

  motr_writer->delete_object(
      std::bind(&S3PutObjectActionBase::remove_old_object_version_metadata,
                this),
      std::bind(&S3PutObjectActionBase::next, this), old_object_oid,
      old_layout_id, object_metadata->get_pvid());

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectActionBase::remove_old_object_version_metadata() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");

  object_metadata->remove_version_metadata(
      std::bind(&S3PutObjectActionBase::remove_old_oid_probable_record, this),
      std::bind(&S3PutObjectActionBase::next, this));
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectActionBase::delete_new_object() {
  s3_log(S3_LOG_INFO, request_id, "Entering\n");
  // If PUT failed, then clean new object.
  assert(s3_put_action_state != S3PutObjectActionState::completed);
  assert(new_object_oid.u_hi != 0ULL || new_object_oid.u_lo != 0ULL);

  motr_writer->delete_object(
      std::bind(&S3PutObjectActionBase::remove_new_oid_probable_record, this),
      std::bind(&S3PutObjectActionBase::next, this), new_object_oid, layout_id);

  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}

void S3PutObjectActionBase::set_authorization_meta() {
  s3_log(S3_LOG_DEBUG, request_id, "Entering\n");
  auth_client->set_acl_and_policy(bucket_metadata->get_encoded_bucket_acl(),
                                  bucket_metadata->get_policy_as_json());
  next();
  s3_log(S3_LOG_DEBUG, "", "Exiting\n");
}
