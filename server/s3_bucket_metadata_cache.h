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
#pragma once

#include "s3_bucket_metadata.h"

#include <chrono>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <queue>
#include <string>

#include <gtest/gtest_prod.h>

// Forward declarations
class S3BucketMetadataV1;
class S3MotrBucketMetadataFactory;
class S3RequestObject;

class S3BucketMetadataCache {

  // The class should have single instance
  static S3BucketMetadataCache* p_instance;

 protected:
  unsigned max_cache_size, expire_interval_sec, refresh_interval_sec;
  bool disabled = true;

 public:
  S3BucketMetadataCache(unsigned max_cache_size, unsigned expire_interval_sec,
                        unsigned refresh_interval_sec,
                        std::shared_ptr<S3MotrBucketMetadataFactory> = {});

  S3BucketMetadataCache(const S3BucketMetadataCache&) = delete;
  S3BucketMetadataCache& operator=(const S3BucketMetadataCache&) = delete;

  virtual ~S3BucketMetadataCache();

  // Getter for single instance
  static S3BucketMetadataCache* get_instance();

  void disable() { disabled = true; }
  void enable() { disabled = false; }

  using FetchHandlerType =
      std::function<void(S3BucketMetadataState, const S3BucketMetadata&)>;
  using StateHandlerType = std::function<void(S3BucketMetadataState)>;

  virtual void fetch(const S3BucketMetadata& dst, FetchHandlerType on_fetch);
  virtual void save(const S3BucketMetadata& src, StateHandlerType on_save);
  virtual void update(const S3BucketMetadata& src, StateHandlerType on_update);
  virtual void remove(const S3BucketMetadata& src, StateHandlerType on_remove);

 private:
  std::shared_ptr<S3MotrBucketMetadataFactory> s3_motr_bucket_metadata_factory;

  bool shrink();
  void remove_item(std::string bucket_name);

  std::unique_ptr<S3BucketMetadataV1> create_engine(
      const S3BucketMetadata& src);

  class Item;
  Item* get_item(const S3BucketMetadata& src);

  void updated(Item* p_item);

  std::map<std::string, std::unique_ptr<Item> > items;

  using ListItems = std::list<Item*>;
  using ListIterator = ListItems::iterator;

  ListItems sorted_by_access;
  ListItems sorted_by_update;

  using Clock = std::chrono::steady_clock;
  using Duration = Clock::duration;
  using TimePoint = std::chrono::time_point<Clock>;

  friend class S3BucketMetadataCacheTest;
};

class S3BucketMetadataCache::Item {
 public:
  explicit Item(const S3BucketMetadata& src);
  ~Item();

  void fetch(const S3BucketMetadata& src, FetchHandlerType on_fetch);
  void save(const S3BucketMetadata& src, StateHandlerType on_save);
  void update(const S3BucketMetadata& src, StateHandlerType on_update);
  void remove(StateHandlerType on_remove);

  bool can_remove() const;
  const std::string& get_bucket_name() const;

  TimePoint update_time;
  TimePoint access_time;

  ListIterator ptr_access;
  ListIterator ptr_update;

  enum class CurrentOp {
    none,
    fetching,
    saving,
    deleting
  };

 private:
  void on_done(S3BucketMetadataState state);
  void on_load(S3BucketMetadataState state);

  CurrentOp current_op = CurrentOp::none;
  // For save, update and remove operations
  StateHandlerType on_changed;
  // For fetch operation
  std::queue<FetchHandlerType> fetch_waiters;

  std::unique_ptr<S3BucketMetadata> p_value;
  std::unique_ptr<S3BucketMetadataV1> p_engine_modify;
  std::unique_ptr<S3BucketMetadataV1> p_engine_load;

  friend class S3BucketMetadataCacheTest;
};
