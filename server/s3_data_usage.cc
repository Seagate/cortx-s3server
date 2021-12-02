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

#include "s3_data_usage.h"

#include <json/json.h>

#include "s3_log.h"

#define JSON_OBJECTS_COUNT "objects_count"
#define JSON_BYTES_COUNT "bytes_count"
#define JSON_DEGRADED_COUNT "degraded_count"

extern struct s3_motr_idx_layout bucket_data_usage_index_layout;

std::unique_ptr<S3DataUsageCache> S3DataUsageCache::singleton;

S3DataUsageCache* S3DataUsageCache::get_instance() {
  if (!singleton) {
    singleton.reset(new S3DataUsageCache());
  }
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

    inactive_items.erase(item->ptr_inactive);
    return items[key_in_cache];
  } else {
    s3_log(S3_LOG_INFO, bkt_md->get_stripped_request_id(),
           "Entry not found in Cache for %s", key_in_cache.c_str());
    std::function<void(DataUsageItem*)> subscriber =
      std::bind(&S3DataUsageCache::item_state_changed, this,
                std::placeholders::_1);
    std::shared_ptr<DataUsageItem> bucket_counter(
      new DataUsageItem(req, bkt_md, subscriber));
    std::pair<std::map<std::string,
                       std::shared_ptr<DataUsageItem> >::iterator,
              bool> return_value =
        items.insert(
            std::pair<std::string, std::shared_ptr<DataUsageItem> >(
                key_in_cache, std::move(bucket_counter)));

    s3_log(S3_LOG_INFO, bkt_md->get_stripped_request_id(), "%s Exit", __func__);
    return return_value.first->second;
  }
}

bool S3DataUsageCache::shrink() {
  if (inactive_items.size() == 0) {
    return false;
  }

  // The front item of the list is the LRU one.
  DataUsageItem *item = inactive_items.front();
  std::string cache_key = get_cache_key(item->bucket_metadata);

  s3_log(S3_LOG_DEBUG, item->bucket_metadata->get_stripped_request_id(),
         "Data usage for \"%s\" will be removed from the cache",
         cache_key.c_str());
  inactive_items.pop_front();
  items.erase(cache_key);
  return true;
}

void S3DataUsageCache::item_state_changed(DataUsageItem *item) {
  if (item->state == DataUsageItemState::empty
      || item->state == DataUsageItemState::failed) {
    std::string cache_key = get_cache_key(item->bucket_metadata);
    items.erase(cache_key);
  }
  else if (item->state == DataUsageItemState::inactive) {
    item->ptr_inactive = inactive_items.insert(
      inactive_items.end(), item);
  }
}

void S3DataUsageCache::update_data_usage(std::shared_ptr<RequestObject> req,
                                         std::shared_ptr<S3BucketMetadata> src,
                                         int64_t objects_count_increment,
                                         int64_t bytes_count_increment,
                                         std::function<void()> on_success,
                                         std::function<void()> on_failure) {
  s3_log(S3_LOG_INFO, src->get_stripped_request_id(), "%s Entry", __func__);

  std::shared_ptr<DataUsageItem> counter(get_item(req, src));
  if (counter) {
    counter->add_inc_object_count(objects_count_increment);
    counter->add_inc_size(bytes_count_increment);
    counter->save(on_success, on_failure);
  }
  else {
    on_failure();
  }

  s3_log(S3_LOG_INFO, src->get_stripped_request_id(), "%s Exit", __func__);
}

DataUsageItem::DataUsageItem(
    std::shared_ptr<RequestObject> req,
    std::shared_ptr<S3BucketMetadata> bkt_md,
    std::function<void(DataUsageItem*)> subscriber,
    std::shared_ptr<S3MotrKVSReaderFactory> kv_reader_factory,
    std::shared_ptr<S3MotrKVSWriterFactory> kv_writer_factory,
    std::shared_ptr<MotrAPI> motr_api) {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);
  request = std::move(req);
  bucket_metadata = std::move(bkt_md);
  request_id = bucket_metadata->get_request_id();
  bucket_name = bucket_metadata->get_bucket_name();
  key = bucket_name + "/" + generate_unique_id();
  state_notify = subscriber;
  inc_object_count = 0;
  inc_total_size = 0;
  inc_degraded_count = 0;
  is_cache_created = false;

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
    mote_kv_writer_factory = std::move(kv_writer_factory);
  } else {
    mote_kv_writer_factory = std::make_shared<S3MotrKVSWriterFactory>();
  }

  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
}

void DataUsageItem::set_state(DataUsageItemState new_state) {
  state = new_state;
  state_notify(this);
}

void DataUsageItem::save(std::function<void(void)> on_success,
                                 std::function<void(void)> on_failed) {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);

  // object_list_index_layout should be set before using this method
  // assert(non_zero(bucket_data_usage_index_layout.oid));

  this->handler_on_success = on_success;
  this->handler_on_failed = on_failed;

  if (!is_cache_created) {
    if (!motr_kv_reader) {
      motr_kv_reader =
          motr_kv_reader_factory->create_motr_kvs_reader(request, s3_motr_api);
    }

    motr_kv_reader->get_keyval(
        bucket_data_usage_index_layout, key,
        std::bind(&DataUsageItem::load_successful, this),
        std::bind(&DataUsageItem::load_failed, this));
  } else {
    save_bucket_counters();
  }

  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
}

void DataUsageItem::load_successful() {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);

  if (this->from_json(motr_kv_reader->get_value()) != 0) {
    s3_log(S3_LOG_ERROR, request_id,
           "Json Parsing failed. Index oid = "
           "%" SCNx64 ":%" SCNx64 ", Key = %s, Value = %s\n",
           bucket_data_usage_index_layout.oid.u_hi,
           bucket_data_usage_index_layout.oid.u_lo, key.c_str(),
           motr_kv_reader->get_value().c_str());

    this->handler_on_failed();
  }

  set_state(DataUsageItemState::active);
  is_cache_created = true;
  save_bucket_counters();

  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
}

void DataUsageItem::load_failed() {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);

  saved_object_count = 0;
  saved_total_size = 0;
  saved_degraded_count = 0;

  // TODO: check if missing entry is possible here.
  set_state(DataUsageItemState::active);

  is_cache_created = true;
  save_bucket_counters();

  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
}

void DataUsageItem::save_bucket_counters() {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);

  if (!motr_kv_writer) {
    motr_kv_writer =
        mote_kv_writer_factory->create_motr_kvs_writer(request, s3_motr_api);
  }

  motr_kv_writer->put_keyval(
      bucket_data_usage_index_layout, key, this->to_json(),
      std::bind(&DataUsageItem::save_counters_successful, this),
      std::bind(&DataUsageItem::save_metadata_failed, this));

  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
}

void DataUsageItem::save_counters_successful() {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);

  // This is so that we dont have to call
  // getkeyval everytime. We only do it once
  // then we maintain things in memory.
  // this may be time efficient.
  saved_object_count += inc_object_count;
  saved_total_size += inc_total_size;
  saved_degraded_count += inc_degraded_count;

  inc_object_count = 0;
  inc_total_size = 0;
  inc_degraded_count = 0;

  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);

  set_state(DataUsageItemState::inactive);

  this->handler_on_success();
}

void DataUsageItem::save_metadata_failed() {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);
  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
  set_state(DataUsageItemState::inactive);
  this->handler_on_failed();
}

std::string DataUsageItem::to_json() {
  s3_log(S3_LOG_DEBUG, request_id, "Called\n");

  Json::Value root;
  root[JSON_OBJECTS_COUNT] =
      Json::Value((Json::Value::UInt64)(saved_object_count + inc_object_count));
  root[JSON_BYTES_COUNT] =
      Json::Value((Json::Value::UInt64)(saved_total_size + inc_total_size));
  root[JSON_DEGRADED_COUNT] = Json::Value(
      (Json::Value::UInt64)(saved_degraded_count + inc_degraded_count));

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

  saved_object_count = newroot[JSON_OBJECTS_COUNT].asUInt64();
  saved_total_size = newroot[JSON_BYTES_COUNT].asUInt64();
  saved_degraded_count = newroot[JSON_DEGRADED_COUNT].asUInt64();

  s3_log(S3_LOG_INFO, request_id,
         "%s Load complete. saved_object_count = %lu, saved_total_size = %lu, "
         "saved_degraded_count = %lu\n",
         __func__, saved_object_count, saved_total_size, saved_degraded_count);

  s3_log(S3_LOG_DEBUG, request_id, "[%s] Exit\n", __func__);
  return 0;
}

void DataUsageItem::add_inc_object_count(int64_t obj_count) {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);
  inc_object_count = obj_count;
  s3_log(S3_LOG_INFO, request_id, "%s inc_object_count = %lu\n", __func__,
         inc_object_count);
  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
}

void DataUsageItem::add_inc_size(int64_t size) {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);
  inc_total_size = size;
  s3_log(S3_LOG_INFO, request_id, "%s inc_total_size = %lu\n", __func__,
         inc_total_size);
  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
}

void DataUsageItem::add_inc_degraded_count(int64_t degraded_count) {
  s3_log(S3_LOG_INFO, request_id, "%s Entry\n", __func__);
  inc_degraded_count = degraded_count;
  s3_log(S3_LOG_INFO, request_id, "%s inc_degraded_count = %lu\n", __func__,
         inc_degraded_count);
  s3_log(S3_LOG_INFO, request_id, "%s Exit\n", __func__);
}
