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

#pragma once

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <queue>

#include "s3_factory.h"
#include "s3_motr_kvs_reader.h"
#include "s3_motr_kvs_writer.h"

enum class DataUsageItemState {
  empty,
  inactive,
  active,
  failed
};

class DataUsageItem;

class S3DataUsageCache {
 public:
  S3DataUsageCache(const S3DataUsageCache&) = delete;
  S3DataUsageCache& operator=(const S3DataUsageCache&) = delete;

  static S3DataUsageCache* get_instance();
  void set_max_cache_size(size_t max_size);

  void update_data_usage(std::shared_ptr<RequestObject> req,
                         std::shared_ptr<S3BucketMetadata> src,
                         int64_t objects_count_increment,
                         int64_t bytes_count_increment,
                         std::function<void()> on_success,
                         std::function<void()> on_failure);

  virtual ~S3DataUsageCache() {}

 private:
  static std::unique_ptr<S3DataUsageCache> singleton;

  std::map<std::string, std::shared_ptr<DataUsageItem> > items;
  std::list<DataUsageItem*> inactive_items;
  size_t max_cache_size;

  S3DataUsageCache() {}

  std::shared_ptr<DataUsageItem> get_item(
      std::shared_ptr<RequestObject> req,
      std::shared_ptr<S3BucketMetadata> bkt_md);
  bool shrink();
  void item_state_changed(DataUsageItem* item);
};

class DataUsageItem {
 private:
  std::string key;
  std::shared_ptr<S3MotrKVSReaderFactory> motr_kv_reader_factory;
  std::shared_ptr<S3MotrKVSWriterFactory> motr_kv_writer_factory;
  std::shared_ptr<MotrAPI> s3_motr_api;
  std::shared_ptr<RequestObject> request;
  std::shared_ptr<S3MotrKVSWriter> motr_kv_writer;
  std::shared_ptr<S3MotrKVSReader> motr_kv_reader;

  struct IncCallbacks {
    std::string request_id;
    std::function<void()> success;
    std::function<void()> fail;
  };

  int64_t objects_count;
  int64_t bytes_written;

  int64_t current_objects_increment;
  int64_t current_bytes_increment;
  std::queue<struct IncCallbacks> current_callbacks;

  int64_t pending_objects_increment;
  int64_t pending_bytes_increment;
  std::queue<struct IncCallbacks> pending_callbacks;

  // Used to report to caller.
  std::function<void()> handler_on_success;
  std::function<void()> handler_on_failure;
  std::function<void(DataUsageItem*)> state_notify;

 public:
  DataUsageItem(std::shared_ptr<RequestObject> req,
                std::shared_ptr<S3BucketMetadata> bkt_md,
                std::function<void(DataUsageItem*)> subscriber,
                std::shared_ptr<S3MotrKVSReaderFactory> kv_reader_factory =
                    nullptr,
                std::shared_ptr<S3MotrKVSWriterFactory> kv_writer_factory =
                    nullptr,
                std::shared_ptr<MotrAPI> motr_api = nullptr);
  std::string request_id;
  std::string bucket_name;
  std::shared_ptr<S3BucketMetadata> bucket_metadata;
  DataUsageItemState state;
  std::list<DataUsageItem*>::iterator ptr_inactive;

  void set_state(DataUsageItemState new_state);
  void save(int64_t objects_count_increment, int64_t bytes_count_increment,
            std::function<void(void)> on_success,
            std::function<void(void)> on_failure);
  void do_kvs_read();
  void kvs_read_success();
  void kvs_read_failure();
  void do_kvs_write();
  void kvs_write_success();
  void kvs_write_failure();
  std::string to_json();
  int from_json(std::string content);
};
