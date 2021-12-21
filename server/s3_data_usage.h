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
#include <gtest/gtest_prod.h>

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
class DataUsageItemFactory;

class S3DataUsageCache {
  static std::unique_ptr<S3DataUsageCache> singleton;
  static std::string generate_cache_key(std::shared_ptr<RequestObject> req);

  std::map<std::string, std::shared_ptr<DataUsageItem> > items;
  std::shared_ptr<DataUsageItemFactory> item_factory;
  // The list of items that are not running any Motr command now.
  std::list<DataUsageItem *> inactive_items;
  size_t max_cache_size;

  S3DataUsageCache();
  std::shared_ptr<DataUsageItem> get_item(std::shared_ptr<RequestObject> req);
  bool shrink(const std::string &request_id);
  void item_state_changed(DataUsageItem *item, DataUsageItemState prev_state,
                          DataUsageItemState new_state);

 public:
  static S3DataUsageCache *get_instance();
  static void update_data_usage(std::shared_ptr<RequestObject> req,
                                std::shared_ptr<S3BucketMetadata> src,
                                int64_t objects_count_increment,
                                int64_t bytes_count_increment,
                                std::function<void()> on_success,
                                std::function<void()> on_failure);

  S3DataUsageCache(const S3DataUsageCache &) = delete;
  S3DataUsageCache &operator=(const S3DataUsageCache &) = delete;

  void set_max_cache_size(size_t max_size);
  void set_item_factory(std::shared_ptr<DataUsageItemFactory> factory);
  virtual ~S3DataUsageCache() {}

  friend class S3DataUsageCacheTest;
  FRIEND_TEST(S3DataUsageCacheTest, SetMaxCacheSize);
  FRIEND_TEST(S3DataUsageCacheTest, UpdateWithEmptyCache);
  FRIEND_TEST(S3DataUsageCacheTest, UpdateWithFullCacheEviction);
  FRIEND_TEST(S3DataUsageCacheTest, UpdateWithFullCacheNoEviction);
};

class DataUsageItem {
  // A key to the Motr index that a cache writes into;
  // currenly its "<account name>/<unique S3 server ID>".
  std::string motr_key;
  // Item's key in the data usage cache; currently "<account name>".
  std::string cache_key;
  std::shared_ptr<MotrAPI> motr_kv_api;
  std::shared_ptr<S3MotrKVSReaderFactory> motr_kv_reader_factory;
  std::shared_ptr<S3MotrKVSWriterFactory> motr_kv_writer_factory;
  std::shared_ptr<S3MotrKVSWriter> motr_kv_writer;
  std::shared_ptr<S3MotrKVSReader> motr_kv_reader;

  int64_t objects_count;
  int64_t bytes_count;

  struct IncCallbackPair {
    IncCallbackPair(const std::string &rid, std::function<void()> s,
                    std::function<void()> f)
        : request_id{rid}, on_success{s}, on_failure{f} {};
    std::string request_id;
    std::function<void()> on_success;
    std::function<void()> on_failure;
  };

  // current_* attributes serve the request that is being read/written,
  // i.e. Motr performs I/O operation for it right now.
  std::shared_ptr<RequestObject> current_request;
  int64_t current_objects_increment;
  int64_t current_bytes_increment;
  std::list<std::shared_ptr<struct IncCallbackPair> > current_callbacks;

  // pending_* attributes serve the request that is to be executed.
  std::shared_ptr<RequestObject> pending_request;
  int64_t pending_objects_increment;
  int64_t pending_bytes_increment;
  std::list<std::shared_ptr<struct IncCallbackPair> > pending_callbacks;

  using DataUsageStateNotifyCb = std::function<
      void(DataUsageItem *, DataUsageItemState, DataUsageItemState)>;
  // Calls back to the cache singleton when an item state is changed.
  DataUsageStateNotifyCb state_cb;

  DataUsageItemState state;
  // Item's place in the list of inactive items that the cache maintains
  std::list<DataUsageItem *>::iterator ptr_inactive;

  std::string get_item_request_id();
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
  DataUsageItem(std::shared_ptr<RequestObject> req,
                const std::string &key_in_cache,
                DataUsageStateNotifyCb subscriber,
                std::shared_ptr<S3MotrKVSReaderFactory> kvs_reader_factory =
                    nullptr,
                std::shared_ptr<S3MotrKVSWriterFactory> kvs_writer_factory =
                    nullptr,
                std::shared_ptr<MotrAPI> motr_api = nullptr);
  virtual void save(std::shared_ptr<RequestObject> req,
                    int64_t objects_count_increment,
                    int64_t bytes_count_increment,
                    std::function<void()> on_success,
                    std::function<void()> on_failure);

  friend S3DataUsageCache;
  friend class DataUsageItemTest;
  friend class S3DataUsageCacheTest;

  FRIEND_TEST(DataUsageItemTest, Constructor);
  FRIEND_TEST(DataUsageItemTest, GetItemRequestId);
  FRIEND_TEST(DataUsageItemTest, SetState);
  FRIEND_TEST(DataUsageItemTest, SimpleSuccessfulSave);
  FRIEND_TEST(DataUsageItemTest, SimpleSaveNegativeCounters);
  FRIEND_TEST(DataUsageItemTest, SimpleSaveMissingKey);
  FRIEND_TEST(DataUsageItemTest, SimpleSaveFailedToRead);
  FRIEND_TEST(DataUsageItemTest, SimpleSaveFailedToWrite);
  FRIEND_TEST(DataUsageItemTest, MultipleSaveSuccessful);
  FRIEND_TEST(DataUsageItemTest, MultipleSaveOneWriteFailed);
  FRIEND_TEST(DataUsageItemTest, Annihilation);
  FRIEND_TEST(DataUsageItemTest, MultipleSaveConsequent);

  FRIEND_TEST(S3DataUsageCacheTest, UpdateWithEmptyCache);
  FRIEND_TEST(S3DataUsageCacheTest, UpdateWithFullCacheEviction);
  FRIEND_TEST(S3DataUsageCacheTest, UpdateWithFullCacheNoEviction);
};

class DataUsageItemFactory {
 public:
  virtual ~DataUsageItemFactory() {}
  virtual std::shared_ptr<DataUsageItem> create_data_usage_item(
      std::shared_ptr<RequestObject> req, const std::string &key_in_cache,
      std::function<void(DataUsageItem *, DataUsageItemState,
                         DataUsageItemState)> subscriber) {
    return std::make_shared<DataUsageItem>(req, key_in_cache, subscriber);
  }
};
