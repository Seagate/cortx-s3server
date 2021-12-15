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

extern struct s3_motr_idx_layout data_usage_accounts_index_layout;

std::unique_ptr<S3DataUsageCache> S3DataUsageCache::singleton;

S3DataUsageCache *S3DataUsageCache::get_instance() {
  s3_log(S3_LOG_DEBUG, "", "%s Entry\n", __func__);
  if (!singleton) {
    s3_log(S3_LOG_INFO, "", "%s Create a new DataUsageCache", __func__);
    singleton.reset(new S3DataUsageCache());
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit\n", __func__);
  return singleton.get();
}

void S3DataUsageCache::set_max_cache_size(size_t max_size) {
  s3_log(S3_LOG_DEBUG, "", "%s Entry\n", __func__);
  s3_log(S3_LOG_INFO, "", "Set data usage cache size to %zu", max_size);
  max_cache_size = max_size;
  s3_log(S3_LOG_DEBUG, "", "%s Exit\n", __func__);
}

std::string S3DataUsageCache::generate_cache_key(
    std::shared_ptr<RequestObject> req) {
  const std::string &req_id = req->get_stripped_request_id();
  s3_log(S3_LOG_DEBUG, req_id, "%s Entry\n", __func__);
  s3_log(S3_LOG_DEBUG, req_id, "%s Exit\n", __func__);
  return req->get_account_name();
}

std::string get_server_id() {
  s3_log(S3_LOG_DEBUG, "", "%s Entry\n", __func__);
  extern struct m0_config motr_conf;
  // static std::string s3server_ID = motr_conf.mc_process_fid;
  // FID is not properly integrated in dev env, so will be using Motr EP
  // address:
  static std::string s3server_ID = motr_conf.mc_local_addr;
  s3_log(S3_LOG_DEBUG, "", "%s Exit, ret %s\n", __func__, s3server_ID.c_str());
  return s3server_ID;
}

std::shared_ptr<DataUsageItem> S3DataUsageCache::get_item(
    std::shared_ptr<RequestObject> req) {
  const std::string &req_id = req->get_stripped_request_id();
  s3_log(S3_LOG_DEBUG, req_id, "%s Entry", __func__);

  std::string key_in_cache = generate_cache_key(req);
  const bool f_new = (items.end() == items.find(key_in_cache));

  if (items.size() + f_new > max_cache_size) {
    s3_log(S3_LOG_WARN, req_id, "Data usage cache is full");
    if (!shrink(req_id)) {
      s3_log(S3_LOG_WARN, req_id, "Failed to shrink the data usage cache");
      return nullptr;
    }
  }

  if (!f_new) {
    s3_log(S3_LOG_INFO, req_id, "Found entry in Cache for %s",
           key_in_cache.c_str());
    s3_log(S3_LOG_DEBUG, req_id, "%s Exit", __func__);
    return items[key_in_cache];
  }

  s3_log(S3_LOG_INFO, req_id, "Entry not found in Cache for %s",
         key_in_cache.c_str());
  auto subscriber = std::bind(&S3DataUsageCache::item_state_changed, this,
                              std::placeholders::_1, std::placeholders::_2,
                              std::placeholders::_3);
  std::shared_ptr<DataUsageItem> item(
      new DataUsageItem(req, key_in_cache, subscriber));
  items.emplace(key_in_cache, std::move(item));

  s3_log(S3_LOG_DEBUG, req_id, "%s Exit", __func__);
  return items[key_in_cache];
}

bool S3DataUsageCache::shrink(const std::string &request_id) {
  s3_log(S3_LOG_DEBUG, request_id, "%s Entry", __func__);
  if (inactive_items.size() == 0) {
    s3_log(S3_LOG_WARN, request_id,
           "%s Cannot shrink cache: all items are active", __func__);
    s3_log(S3_LOG_DEBUG, request_id, "%s Exit\n", __func__);
    return false;
  }

  // The front item of the list is the LRU one.
  DataUsageItem *item = inactive_items.front();
  const std::string cache_key = item->cache_key;

  s3_log(S3_LOG_INFO, request_id,
         "Data usage entry for %s will be removed from the cache as inactive",
         cache_key.c_str());
  inactive_items.pop_front();
  items.erase(cache_key);
  s3_log(S3_LOG_DEBUG, request_id, "%s Exit\n", __func__);
  return true;
}

void S3DataUsageCache::item_state_changed(DataUsageItem *item,
                                          DataUsageItemState prev_state,
                                          DataUsageItemState new_state) {
  // Copy the "request id" string, because the item might be deleted here.
  std::string req_id = item->get_item_request_id();
  s3_log(S3_LOG_DEBUG, req_id, "%s Entry", __func__);
  if (prev_state == DataUsageItemState::inactive) {
    s3_log(S3_LOG_DEBUG, req_id, "Data usage item for key %s is activated",
           item->cache_key.c_str());
    inactive_items.erase(item->ptr_inactive);
    item->ptr_inactive = inactive_items.end();
  }
  if (new_state == DataUsageItemState::inactive) {
    s3_log(S3_LOG_DEBUG, req_id, "Data usage item for key %s becomes inactive",
           item->cache_key.c_str());
    item->ptr_inactive = inactive_items.insert(inactive_items.end(), item);
  } else if (new_state == DataUsageItemState::failed) {
    s3_log(S3_LOG_INFO, req_id,
           "Data usage item for key=%s will be removed from cache as failed",
           item->cache_key.c_str());
    items.erase(item->cache_key);
  }
  s3_log(S3_LOG_DEBUG, req_id, "%s Exit\n", __func__);
}

void S3DataUsageCache::update_data_usage(std::shared_ptr<RequestObject> req,
                                         std::shared_ptr<S3BucketMetadata> src,
                                         int64_t objects_count_increment,
                                         int64_t bytes_count_increment,
                                         std::function<void()> on_success,
                                         std::function<void()> on_failure) {
  const std::string &req_id = req->get_stripped_request_id();
  s3_log(S3_LOG_DEBUG, req_id, "%s Entry with object_count_increment=%" PRId64
                               ", bytes_count_increment=%" PRId64,
         __func__, objects_count_increment, bytes_count_increment);
  S3DataUsageCache *cache = S3DataUsageCache::get_instance();
  std::shared_ptr<DataUsageItem> item(cache->get_item(req));
  if (item) {
    item->save(req, objects_count_increment, bytes_count_increment, on_success,
               on_failure);
  } else {
    s3_log(S3_LOG_DEBUG, req_id,
           "Failed to retrieve/create a Data usage cache item");
    on_failure();
  }

  s3_log(S3_LOG_DEBUG, req_id, "%s Exit", __func__);
}

DataUsageItem::DataUsageItem(
    std::shared_ptr<RequestObject> req, const std::string &key_in_cache,
    DataUsageStateNotifyCb subscriber,
    std::shared_ptr<S3MotrKVSReaderFactory> kvs_reader_factory,
    std::shared_ptr<S3MotrKVSWriterFactory> kvs_writer_factory,
    std::shared_ptr<MotrAPI> motr_api) {
  s3_log(S3_LOG_DEBUG, "", "%s Entry with key_in_cache %s\n", __func__,
         key_in_cache.c_str());
  current_request = std::move(req);
  pending_request = nullptr;
  cache_key = std::move(key_in_cache);
  motr_key = cache_key + "/" + get_server_id();
  state_cb = subscriber;
  state = DataUsageItemState::empty;
  current_objects_increment = 0;
  current_bytes_increment = 0;
  pending_objects_increment = 0;
  pending_bytes_increment = 0;

  if (motr_api) {
    motr_kv_api = std::move(motr_api);
  } else {
    motr_kv_api = std::make_shared<ConcreteMotrAPI>();
  }
  if (kvs_reader_factory) {
    motr_kv_reader_factory = std::move(kvs_reader_factory);
  } else {
    motr_kv_reader_factory = std::make_shared<S3MotrKVSReaderFactory>();
  }
  if (kvs_writer_factory) {
    motr_kv_writer_factory = std::move(kvs_writer_factory);
  } else {
    motr_kv_writer_factory = std::make_shared<S3MotrKVSWriterFactory>();
  }
  motr_kv_reader = nullptr;
  motr_kv_writer = nullptr;

  s3_log(S3_LOG_DEBUG, "", "%s Exit\n", __func__);
}

std::string DataUsageItem::get_item_request_id() {
  return current_request ? current_request->get_stripped_request_id() : "";
}

void DataUsageItem::set_state(DataUsageItemState new_state) {
  std::string req_id = get_item_request_id();
  s3_log(S3_LOG_DEBUG, req_id, "%s Entry\n", __func__);
  if (new_state != state) {
    DataUsageItemState old_state = state;
    state = new_state;
    state_cb(this, old_state, new_state);
  }
  s3_log(S3_LOG_DEBUG, req_id, "%s Exit\n", __func__);
}

void DataUsageItem::save(std::shared_ptr<RequestObject> req,
                         int64_t objects_count_increment,
                         int64_t bytes_count_increment,
                         std::function<void(void)> on_success,
                         std::function<void(void)> on_failure) {
  const std::string &req_id = req->get_stripped_request_id();
  s3_log(S3_LOG_DEBUG, req_id, "%s Entry\n", __func__);
  s3_log(S3_LOG_INFO, req_id, "%s Object count increment %" PRId64
                              "; bytes count increment %" PRId64,
         __func__, objects_count_increment, bytes_count_increment);

  pending_objects_increment += objects_count_increment;
  pending_bytes_increment += bytes_count_increment;
  pending_callbacks.push_back(std::make_shared<struct IncCallbackPair>(
      req->get_stripped_request_id(), on_success, on_failure));

  // The first request that makes the cache item active is tracked by own id.
  // Pending requests are tracked by the id of the first pending request.
  if (state != DataUsageItemState::active) {
    assert(current_request == nullptr);
    current_request = std::move(req);
    s3_log(S3_LOG_INFO, req_id, "The request can be tracked by its own ID");
  } else {
    if (!pending_request) {
      pending_request = std::move(req);
      s3_log(S3_LOG_INFO, req_id,
             "The request can be tracked by its own ID after request %s is "
             "complete",
             get_item_request_id().c_str());
    } else {
      s3_log(S3_LOG_INFO, req_id,
             "The request can be tracked by ID %s after request %s is complete",
             pending_request->get_stripped_request_id().c_str(),
             current_request->get_stripped_request_id().c_str());
    }
  }

  // It's assumed that item in the failed state will be deleted,
  // but it won't hurt to have it here in the condition.
  if (state == DataUsageItemState::empty ||
      state == DataUsageItemState::failed) {
    do_kvs_read();
  } else if (state == DataUsageItemState::inactive) {
    do_kvs_write();
  } else {
    s3_log(S3_LOG_INFO, req_id, "%s The increment is queued for writing",
           __func__);
  }

  s3_log(S3_LOG_DEBUG, req_id, "%s Exit\n", __func__);
}

void DataUsageItem::run_successful_callbacks() {
  std::string req_id = get_item_request_id();
  s3_log(S3_LOG_DEBUG, req_id, "%s Entry\n", __func__);
  for (auto cb : current_callbacks) {
    s3_log(S3_LOG_INFO, req_id, "Data usage update successful for request %s",
           cb->request_id.c_str());
    cb->on_success();
  }
  current_callbacks.clear();
  s3_log(S3_LOG_DEBUG, req_id, "%s Exit\n", __func__);
}

void DataUsageItem::run_failure_callbacks() {
  std::string req_id = get_item_request_id();
  s3_log(S3_LOG_DEBUG, req_id, "%s Entry\n", __func__);
  for (auto cb : current_callbacks) {
    s3_log(S3_LOG_INFO, req_id, "Data usage update failed for request %s",
           cb->request_id.c_str());
    cb->on_failure();
  }
  current_callbacks.clear();
  s3_log(S3_LOG_DEBUG, req_id, "%s Exit\n", __func__);
}

void DataUsageItem::fail_all() {
  std::string req_id = get_item_request_id();
  s3_log(S3_LOG_DEBUG, req_id, "%s Entry\n", __func__);
  current_callbacks.splice(current_callbacks.end(), pending_callbacks);
  run_failure_callbacks();
  current_request = nullptr;
  current_objects_increment = 0;
  current_bytes_increment = 0;
  pending_request = nullptr;
  pending_objects_increment = 0;
  pending_bytes_increment = 0;
  s3_log(S3_LOG_DEBUG, req_id, "%s Exit\n", __func__);
}

void DataUsageItem::do_kvs_read() {
  std::string req_id = get_item_request_id();
  s3_log(S3_LOG_DEBUG, req_id, "%s Entry\n", __func__);
  set_state(DataUsageItemState::active);
  assert(current_request);
  motr_kv_reader = motr_kv_reader_factory->create_motr_kvs_reader(
      current_request, motr_kv_api);
  motr_kv_reader->get_keyval(data_usage_accounts_index_layout, motr_key,
                             std::bind(&DataUsageItem::kvs_read_success, this),
                             std::bind(&DataUsageItem::kvs_read_failure, this));

  s3_log(S3_LOG_DEBUG, req_id, "%s Exit\n", __func__);
}

void DataUsageItem::kvs_read_success() {
  std::string req_id = get_item_request_id();
  s3_log(S3_LOG_DEBUG, req_id, "%s Entry\n", __func__);

  if (this->from_json(motr_kv_reader->get_value()) != 0) {
    s3_log(S3_LOG_ERROR, req_id,
           "Json Parsing failed. Index oid = "
           "%" SCNx64 ":%" SCNx64 ", Key = %s, Value = %s\n",
           data_usage_accounts_index_layout.oid.u_hi,
           data_usage_accounts_index_layout.oid.u_lo, motr_key.c_str(),
           motr_kv_reader->get_value().c_str());
    fail_all();
    set_state(DataUsageItemState::failed);
    s3_log(S3_LOG_DEBUG, req_id, "%s Exit\n", __func__);
    return;
  }

  s3_log(S3_LOG_INFO, req_id, "%s Load complete. objects_count = %" PRId64
                              ", bytes_count = %" PRId64,
         __func__, objects_count, bytes_count);
  do_kvs_write();
  s3_log(S3_LOG_DEBUG, req_id, "%s Exit\n", __func__);
}

void DataUsageItem::kvs_read_failure() {
  std::string req_id = get_item_request_id();
  s3_log(S3_LOG_DEBUG, req_id, "%s Entry\n", __func__);
  if (motr_kv_reader->get_state() == S3MotrKVSReaderOpState::missing) {
    // There is no guarantee that by the first read the index entry for
    // the specific account/server instance will be created.
    // For the missing index entry - set counters to zeroes.
    s3_log(S3_LOG_INFO, req_id,
           "Missing key %s in the index, set counters to 0", motr_key.c_str());
    objects_count = 0;
    bytes_count = 0;
    do_kvs_write();
  } else {
    // Any other read error is fatal for this item, have to do the re-read.
    fail_all();
  }
  s3_log(S3_LOG_DEBUG, req_id, "%s Exit\n", __func__);
}

void DataUsageItem::do_kvs_write() {
  std::string req_id = get_item_request_id();
  s3_log(S3_LOG_DEBUG, req_id, "%s Entry\n", __func__);

  assert(current_objects_increment == 0);
  assert(current_bytes_increment == 0);
  assert(current_callbacks.empty());

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
    s3_log(S3_LOG_INFO, req_id,
           "No KVS PUT is needed, as both increments are zero");
    run_successful_callbacks();
    current_request = nullptr;
    set_state(DataUsageItemState::inactive);
    s3_log(S3_LOG_DEBUG, req_id, "%s Exit\n", __func__);
    return;
  }
  s3_log(S3_LOG_INFO, req_id,
         "Updating %s with objects_count_increment=%" PRId64
         ", bytes_count_increment=%" PRId64,
         motr_key.c_str(), current_objects_increment, current_bytes_increment);
  set_state(DataUsageItemState::active);
  motr_kv_writer = motr_kv_writer_factory->create_motr_kvs_writer(
      current_request, motr_kv_api);
  motr_kv_writer->put_keyval(
      data_usage_accounts_index_layout, motr_key, this->to_json(),
      std::bind(&DataUsageItem::kvs_write_success, this),
      std::bind(&DataUsageItem::kvs_write_failure, this));

  s3_log(S3_LOG_DEBUG, req_id, "%s Exit\n", __func__);
}

void DataUsageItem::kvs_write_success() {
  std::string req_id = get_item_request_id();
  s3_log(S3_LOG_DEBUG, req_id, "%s Entry\n", __func__);

  run_successful_callbacks();
  // Maintain actual counters in memory.
  objects_count += current_objects_increment;
  bytes_count += current_bytes_increment;
  current_objects_increment = 0;
  current_bytes_increment = 0;
  current_request = pending_request;
  pending_request = nullptr;
  // Call unconditionally. do_kvs_write can handle empty pending queue.
  do_kvs_write();

  s3_log(S3_LOG_DEBUG, req_id, "%s Exit\n", __func__);
}

void DataUsageItem::kvs_write_failure() {
  std::string req_id = get_item_request_id();
  s3_log(S3_LOG_DEBUG, req_id, "%s Entry\n", __func__);
  run_failure_callbacks();
  current_objects_increment = 0;
  current_bytes_increment = 0;
  current_request = pending_request;
  pending_request = nullptr;
  // Try to proceed other requests.
  do_kvs_write();
  s3_log(S3_LOG_DEBUG, req_id, "%s Exit\n", __func__);
}

std::string DataUsageItem::to_json() {
  std::string req_id = get_item_request_id();
  s3_log(S3_LOG_DEBUG, req_id, "%s Entry\n", __func__);

  Json::Value root;
  root[JSON_OBJECTS_COUNT] = Json::Value(
      (Json::Value::Int64)(objects_count + current_objects_increment));
  root[JSON_BYTES_COUNT] =
      Json::Value((Json::Value::Int64)(bytes_count + current_bytes_increment));

  Json::FastWriter fastWriter;
  std::string json = fastWriter.write(root);
  s3_log(S3_LOG_DEBUG, req_id, "[%s] Exit, ret %s\n", __func__, json.c_str());
  return json;
}

int DataUsageItem::from_json(std::string content) {
  std::string req_id = get_item_request_id();
  s3_log(S3_LOG_DEBUG, req_id, "%s Entry\n", __func__);
  s3_log(S3_LOG_DEBUG, req_id, "Called with content [%s]\n", content.c_str());
  Json::Value newroot;
  Json::Reader reader;
  bool parsingSuccessful = reader.parse(content.c_str(), newroot);
  if (!parsingSuccessful) {
    s3_log(S3_LOG_ERROR, req_id, "Json parsing failed, %s\n", content.c_str());
    return -1;
  }

  if (!newroot.isMember(JSON_OBJECTS_COUNT) ||
      !newroot.isMember(JSON_BYTES_COUNT)) {
    s3_log(S3_LOG_ERROR, req_id,
           "Invalid Json structure: one or more counters are missing, %s\n",
           content.c_str());
    return -1;
  }

  objects_count = newroot[JSON_OBJECTS_COUNT].asUInt64();
  bytes_count = newroot[JSON_BYTES_COUNT].asUInt64();

  s3_log(S3_LOG_DEBUG, req_id, "[%s] Exit\n", __func__);
  return 0;
}
