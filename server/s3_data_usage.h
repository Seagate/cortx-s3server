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
  static std::unique_ptr<S3DataUsageCache> singleton;
  static std::string generate_cache_key(
      std::shared_ptr<S3BucketMetadata> bkt_md);

  std::map<std::string, std::shared_ptr<DataUsageItem> > items;
  std::list<DataUsageItem *> inactive_items;
  size_t max_cache_size;

  S3DataUsageCache() {}
  std::shared_ptr<DataUsageItem> get_item(
      std::shared_ptr<RequestObject> req,
      std::shared_ptr<S3BucketMetadata> bkt_md);
  bool shrink();
  void item_state_changed(DataUsageItem *item, DataUsageItemState prev_state,
                          DataUsageItemState new_state);

 public:
  static S3DataUsageCache *get_instance();

  S3DataUsageCache(const S3DataUsageCache &) = delete;
  S3DataUsageCache &operator=(const S3DataUsageCache &) = delete;

  void set_max_cache_size(size_t max_size);
  void update_data_usage(std::shared_ptr<RequestObject> req,
                         std::shared_ptr<S3BucketMetadata> src,
                         int64_t objects_count_increment,
                         int64_t bytes_count_increment,
                         std::function<void()> on_success,
                         std::function<void()> on_failure);
  virtual ~S3DataUsageCache() {}
};

class DataUsageItem {
  std::string request_id;
  std::string motr_key;
  std::string cache_key;
  std::shared_ptr<S3MotrKVSReaderFactory> motr_kv_reader_factory;
  std::shared_ptr<S3MotrKVSWriterFactory> motr_kv_writer_factory;
  std::shared_ptr<MotrAPI> s3_motr_api;
  std::shared_ptr<RequestObject> request;
  std::shared_ptr<S3MotrKVSWriter> motr_kv_writer;
  std::shared_ptr<S3MotrKVSReader> motr_kv_reader;

  int64_t objects_count;
  int64_t bytes_written;

  struct IncCallbackPair {
    IncCallbackPair(std::function<void()> s, std::function<void()> f)
        : success{s}, failure{f} {};
    std::function<void()> success;
    std::function<void()> failure;
  };

  int64_t current_objects_increment;
  int64_t current_bytes_increment;
  std::list<std::shared_ptr<struct IncCallbackPair> > current_callbacks;

  int64_t pending_objects_increment;
  int64_t pending_bytes_increment;
  std::list<std::shared_ptr<struct IncCallbackPair> > pending_callbacks;

  // Used to report to caller.
  std::function<void()> handler_on_success;
  std::function<void()> handler_on_failure;
  std::function<void(DataUsageItem *, DataUsageItemState, DataUsageItemState)>
      state_notify;

  DataUsageItemState state;
  std::list<DataUsageItem *>::iterator ptr_inactive;

  void set_state(DataUsageItemState new_state);
  void do_kvs_read();
  void kvs_read_success();
  void kvs_read_failure();
  void do_kvs_write();
  void kvs_write_success();
  void kvs_write_failure();
  void run_successful_callbacks();
  void run_failure_callbacks();
  void fail_all();
  std::string to_json();
  int from_json(std::string content);

 public:
  using DataUsageStateNotifyCb = std::function<
      void(DataUsageItem *, DataUsageItemState, DataUsageItemState)>;
  DataUsageItem(std::shared_ptr<RequestObject> req, std::string key_in_cache,
                DataUsageStateNotifyCb subscriber,
                std::shared_ptr<S3MotrKVSReaderFactory> kv_reader_factory =
                    nullptr,
                std::shared_ptr<S3MotrKVSWriterFactory> kv_writer_factory =
                    nullptr,
                std::shared_ptr<MotrAPI> motr_api = nullptr);
  void save(int64_t objects_count_increment, int64_t bytes_count_increment,
            std::function<void()> on_success, std::function<void()> on_failure);

  friend S3DataUsageCache;
};
