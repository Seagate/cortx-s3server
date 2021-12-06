/*
 * Copyright (c) 2021 Seagate Technology LLC and/or its Affiliates
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

#include <json/json.h>

#include "s3_data_usage.h"
#include "s3_log.h"

#define JSON_OBJECTS_COUNT "objects_count"
#define JSON_BYTES_COUNT "bytes_count"

extern struct s3_motr_idx_layout bucket_data_usage_index_layout;

std::unique_ptr<S3DataUsageCache> S3DataUsageCache::singleton;

S3DataUsageCache *S3DataUsageCache::get_instance() {
  s3_log(S3_LOG_INFO, "", "%s Entry\n", __func__);
  if (!singleton) {
    s3_log(S3_LOG_INFO, "", "%s Create a new DataUsageCache", __func__);
    singleton.reset(new S3DataUsageCache());
  }
  s3_log(S3_LOG_INFO, "", "%s Exit\n", __func__);
  return singleton.get();
}

void S3DataUsageCache::set_max_cache_size(size_t max_size) {
  s3_log(S3_LOG_INFO, "", "%s Entry\n", __func__);
  s3_log(S3_LOG_INFO, "", "Set data usage cache size to %zu", max_size);
  max_cache_size = max_size;
  s3_log(S3_LOG_INFO, "", "%s Exit\n", __func__);
}

std::string get_cache_key(std::shared_ptr<S3BucketMetadata> bkt_md) {
  s3_log(S3_LOG_INFO, "", "%s Entry\n", __func__);
  s3_log(S3_LOG_INFO, "", "%s Exit\n", __func__);
  return bkt_md->get_bucket_name();
}

std::string generate_unique_id() {
  s3_log(S3_LOG_INFO, "", "%s Entry\n", __func__);
  s3_log(S3_LOG_INFO, "", "%s Exit\n", __func__);
  return "1";
}

std::shared_ptr<DataUsageItem> S3DataUsageCache::get_item(
    std::shared_ptr<RequestObject> req,
    std::shared_ptr<S3BucketMetadata> bkt_md) {
  s3_log(S3_LOG_INFO, bkt_md->get_stripped_request_id(), "%s Entry", __func__);

  std::string key_in_cache = get_cache_key(bkt_md);
  const bool f_new = (items.end() == items.find(key_in_cache));

  if (items.size() + f_new > max_cache_size) {
    s3_log(S3_LOG_DEBUG, bkt_md->get_stripped_request_id(),
           "Data usage cache is full");
    if (!shrink()) {
      s3_log(S3_LOG_DEBUG, bkt_md->get_stripped_request_id(),
             "Failed to shrink the data usage cache");
      return nullptr;
    }
  }

  if (!f_new) {
    std::shared_ptr<DataUsageItem> item(items[key_in_cache]);
    s3_log(S3_LOG_INFO, bkt_md->get_stripped_request_id(),
           "Found entry in Cache for %s", key_in_cache.c_str());
    s3_log(S3_LOG_INFO, bkt_md->get_stripped_request_id(), "%s Exit", __func__);
    return items[key_in_cache];
  }

  s3_log(S3_LOG_INFO, bkt_md->get_stripped_request_id(),
         "Entry not found in Cache for %s", key_in_cache.c_str());
  auto subscriber = std::bind(&S3DataUsageCache::item_state_changed, this,
                              std::placeholders::_1, std::placeholders::_2,
                              std::placeholders::_3);
  std::shared_ptr<DataUsageItem> item(
      new DataUsageItem(req, bkt_md, subscriber));
  // Do not check the retcode (bool): no chance for a collision,
  // because the inserted key is new.
  std::pair<std::map<std::string, std::shared_ptr<DataUsageItem> >::iterator,
            bool> return_value =
      items.insert(std::pair<std::string, std::shared_ptr<DataUsageItem> >(
          key_in_cache, std::move(item)));

  s3_log(S3_LOG_INFO, bkt_md->get_stripped_request_id(), "%s Exit", __func__);
  return return_value.first->second;
}

bool S3DataUsageCache::shrink() {
  s3_log(S3_LOG_INFO, "", "%s Entry", __func__);
  if (inactive_items.size() == 0) {
    s3_log(S3_LOG_INFO, "", "%s Cannot shrink cache: all items are active",
           __func__);
    s3_log(S3_LOG_INFO, "", "%s Exit\n", __func__);
    return false;
  }

  // The front item of the list is the LRU one.
  DataUsageItem *item = inactive_items.front();
  std::string cache_key = get_cache_key(item->bucket_metadata);

  s3_log(S3_LOG_INFO, "",
         "%s Data usage for \"%s\" will be removed from the cache as inactive",
         __func__, cache_key.c_str());
  inactive_items.pop_front();
  items.erase(cache_key);
  s3_log(S3_LOG_INFO, "", "%s Exit\n", __func__);
  return true;
}

void S3DataUsageCache::item_state_changed(DataUsageItem *item,
                                          DataUsageItemState prev_state,
                                          DataUsageItemState new_state) {
  s3_log(S3_LOG_INFO, "", "%s Entry", __func__);
  if (prev_state == DataUsageItemState::inactive) {
    inactive_items.erase(item->ptr_inactive);
  }
  if (new_state == DataUsageItemState::inactive) {
    item->ptr_inactive = inactive_items.insert(inactive_items.end(), item);
  } else if (new_state == DataUsageItemState::failed) {
    std::string cache_key = get_cache_key(item->bucket_metadata);
    s3_log(S3_LOG_INFO, "",
           "%s Data usage for \"%s\" will be removed from the cache as failed",
           __func__, cache_key.c_str());
    items.erase(cache_key);
  }
  s3_log(S3_LOG_INFO, "", "%s Exit\n", __func__);
}

void S3DataUsageCache::update_data_usage(std::shared_ptr<RequestObject> req,
                                         std::shared_ptr<S3BucketMetadata> src,
                                         int64_t objects_count_increment,
                                         int64_t bytes_count_increment,
                                         std::function<void()> on_success,
                                         std::function<void()> on_failure) {
  s3_log(S3_LOG_INFO, src->get_stripped_request_id(), "%s Entry", __func__);

  std::shared_ptr<DataUsageItem> item(get_item(req, src));
  if (item) {
    item->save(objects_count_increment, bytes_count_increment, on_success,
               on_failure);
  } else {
    on_failure();
  }

  s3_log(S3_LOG_INFO, src->get_stripped_request_id(), "%s Exit", __func__);
}

DataUsageItem::DataUsageItem(
    std::shared_ptr<RequestObject> req,
    std::shared_ptr<S3BucketMetadata> bkt_md, DataUsageStateNotifyCb subscriber,
    std::shared_ptr<S3MotrKVSReaderFactory> kv_reader_factory,
    std::shared_ptr<S3MotrKVSWriterFactory> kv_writer_factory,
    std::shared_ptr<MotrAPI> motr_api) {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);
  request = std::move(req);
  bucket_metadata = std::move(bkt_md);
  request_id = bucket_metadata->get_request_id();
  bucket_name = bucket_metadata->get_bucket_name();
  motr_key = bucket_name + "/" + generate_unique_id();
  state_notify = subscriber;
  state = DataUsageItemState::empty;
  current_objects_increment = 0;
  current_bytes_increment = 0;
  pending_objects_increment = 0;
  pending_bytes_increment = 0;

  if (motr_api) {
    s3_motr_api = std::move(motr_api);
  } else {
    s3_motr_api = std::make_shared<ConcreteMotrAPI>();
  }
  if (kv_reader_factory) {
    motr_kv_reader_factory = std::move(kv_reader_factory);
  } else {
    motr_kv_reader_factory = std::make_shared<S3MotrKVSReaderFactory>();
  }
  if (kv_writer_factory) {
    motr_kv_writer_factory = std::move(kv_writer_factory);
  } else {
    motr_kv_writer_factory = std::make_shared<S3MotrKVSWriterFactory>();
  }

  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
}

void DataUsageItem::set_state(DataUsageItemState new_state) {
  s3_log(S3_LOG_INFO, "", "%s Entry\n", __func__);
  if (new_state != state) {
    DataUsageItemState old_state = state;
    state = new_state;
    state_notify(this, old_state, new_state);
  }
  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
}

void DataUsageItem::save(int64_t objects_count_increment,
                         int64_t bytes_count_increment,
                         std::function<void(void)> on_success,
                         std::function<void(void)> on_failure) {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);

  // object_list_index_layout should be set before using this method
  // assert(non_zero(bucket_data_usage_index_layout.oid));
  s3_log(S3_LOG_INFO, request_id,
         "%s Object count increment %ld; bytes count increment %ld", __func__,
         objects_count_increment, bytes_count_increment);

  pending_objects_increment += objects_count_increment;
  pending_bytes_increment += bytes_count_increment;
  pending_callbacks.push_back(
      std::make_shared<struct IncCallbackPair>(on_success, on_failure));

  // It's assumed that item in the failed state will be deleted,
  // but it won't hurt to have it here in the condition.
  if (state == DataUsageItemState::empty ||
      state == DataUsageItemState::failed) {
    do_kvs_read();
  } else if (state == DataUsageItemState::inactive) {
    do_kvs_write();
  } else {
    s3_log(S3_LOG_INFO, request_id, "%s The increment is queued for writing",
           __func__);
  }

  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
}

void DataUsageItem::run_successful_callbacks() {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);
  for (auto cb : current_callbacks) {
    cb->success();
  }
  current_callbacks.clear();
  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
}

void DataUsageItem::run_failure_callbacks() {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);
  for (auto cb : current_callbacks) {
    cb->failure();
  }
  current_callbacks.clear();
  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
}

void DataUsageItem::fail_all() {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);
  current_callbacks.splice(current_callbacks.end(), pending_callbacks);
  run_failure_callbacks();
  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
}

void DataUsageItem::kvs_read_success() {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);

  if (this->from_json(motr_kv_reader->get_value()) != 0) {
    s3_log(S3_LOG_ERROR, request_id,
           "Json Parsing failed. Index oid = "
           "%" SCNx64 ":%" SCNx64 ", Key = %s, Value = %s\n",
           bucket_data_usage_index_layout.oid.u_hi,
           bucket_data_usage_index_layout.oid.u_lo, motr_key.c_str(),
           motr_kv_reader->get_value().c_str());
    fail_all();
    set_state(DataUsageItemState::failed);
    return;
  }

  do_kvs_write();

  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
}

void DataUsageItem::kvs_read_failure() {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);
  fail_all();
  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
}

void DataUsageItem::do_kvs_read() {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);

  if (!motr_kv_reader) {
    motr_kv_reader =
        motr_kv_reader_factory->create_motr_kvs_reader(request, s3_motr_api);
  }

  motr_kv_reader->get_keyval(bucket_data_usage_index_layout, motr_key,
                             std::bind(&DataUsageItem::kvs_read_success, this),
                             std::bind(&DataUsageItem::kvs_read_failure, this));

  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
}

void DataUsageItem::do_kvs_write() {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);

  if (pending_callbacks.size() == 0) {
    set_state(DataUsageItemState::inactive);
    return;
  }

  current_objects_increment = pending_objects_increment;
  pending_objects_increment = 0;
  current_bytes_increment = pending_bytes_increment;
  pending_bytes_increment = 0;
  current_callbacks.swap(pending_callbacks);

  // Annihilation check:
  // If agregated value of pending increments is zero,
  // there is no need for PUT KVS operation,
  // thus success callbacks could be called immediately (if there are any).
  if (current_objects_increment == 0 && current_bytes_increment == 0) {
    run_successful_callbacks();
    set_state(DataUsageItemState::inactive);
    return;
  }
  set_state(DataUsageItemState::active);

  if (!motr_kv_writer) {
    motr_kv_writer =
        motr_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }

  motr_kv_writer->put_keyval(
      bucket_data_usage_index_layout, motr_key, this->to_json(),
      std::bind(&DataUsageItem::kvs_write_success, this),
      std::bind(&DataUsageItem::kvs_write_failure, this));

  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
}

void DataUsageItem::kvs_write_success() {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);

  // This is so that we dont have to call
  // getkeyval everytime. We only do it once
  // then we maintain things in memory.
  // this may be time efficient.
  objects_count += current_objects_increment;
  bytes_written += current_bytes_increment;
  run_successful_callbacks();

  do_kvs_write();

  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
}

void DataUsageItem::kvs_write_failure() {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);
  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
  run_failure_callbacks();
  // Try to proceed other requests.
  do_kvs_write();
}

std::string DataUsageItem::to_json() {
  s3_log(S3_LOG_DEBUG, request_id, "Called\n");

  Json::Value root;
  root[JSON_OBJECTS_COUNT] = Json::Value(
      (Json::Value::UInt64)(objects_count + current_objects_increment));
  root[JSON_BYTES_COUNT] = Json::Value(
      (Json::Value::UInt64)(bytes_written + current_bytes_increment));

  Json::FastWriter fastWriter;
  return fastWriter.write(root);
}

int DataUsageItem::from_json(std::string content) {
  s3_log(S3_LOG_DEBUG, request_id, "Called with content [%s]\n",
         content.c_str());
  Json::Value newroot;
  Json::Reader reader;
  bool parsingSuccessful = reader.parse(content.c_str(), newroot);
  if (!parsingSuccessful) {
    s3_log(S3_LOG_ERROR, request_id, "Json Parsing failed\n");
    return -1;
  }

  objects_count = newroot[JSON_OBJECTS_COUNT].asUInt64();
  bytes_written = newroot[JSON_BYTES_COUNT].asUInt64();

  s3_log(S3_LOG_INFO, request_id,
         "%s Load complete. objects_count = %lu, bytes_written = %lu\n",
         __func__, objects_count, bytes_written);

  s3_log(S3_LOG_DEBUG, request_id, "[%s] Exit\n", __func__);
  return 0;
}
