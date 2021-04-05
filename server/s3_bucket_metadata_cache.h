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
 protected:
  unsigned max_cache_size, expire_interval_sec, refresh_interval_sec;
  bool disabled = false;

 public:
  S3BucketMetadataCache(unsigned max_cache_size, unsigned expire_interval_sec,
                        unsigned refresh_interval_sec,
                        std::shared_ptr<S3MotrBucketMetadataFactory> = {});

  S3BucketMetadataCache(const S3BucketMetadataCache&) = delete;
  S3BucketMetadataCache& operator=(const S3BucketMetadataCache&) = delete;

  virtual ~S3BucketMetadataCache();

  void disable() { disabled = true; }
  void enable() { disabled = false; }

  using FetchHanderType =
      std::function<void(S3BucketMetadataState, const S3BucketMetadata&)>;
  using StateHandlerType = std::function<void(S3BucketMetadataState)>;

  virtual void fetch(const S3BucketMetadata& dst, FetchHanderType callback);
  virtual void save(const S3BucketMetadata& src, StateHandlerType callback);
  virtual void update(const S3BucketMetadata& src, StateHandlerType callback);
  virtual void remove(const S3BucketMetadata& src, StateHandlerType callback);

 private:
  std::shared_ptr<S3MotrBucketMetadataFactory> s3_motr_bucket_metadata_factory;

  void shrink();
  void remove_item(std::string bucket_name);

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

  void fetch(const S3BucketMetadata& src, FetchHanderType callback, bool force);
  void save(const S3BucketMetadata& src, StateHandlerType callback);
  void update(const S3BucketMetadata& src, StateHandlerType callback);
  void remove(StateHandlerType callback);

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
  void create_engine(const S3BucketMetadata& src);
  void on_done(S3BucketMetadataState state);
  bool check_expiration();

  CurrentOp current_op = CurrentOp::none;
  // For save, update and remove operations
  StateHandlerType on_changed;
  // For fetch operation
  std::queue<FetchHanderType> fetch_waiters;

  std::unique_ptr<S3BucketMetadata> p_value;
  std::unique_ptr<S3BucketMetadataV1> p_engine;

  friend class S3BucketMetadataCacheTest;
};

// Single instance
extern S3BucketMetadataCache* p_bucket_metadata_cache;
