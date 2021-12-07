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
  // The list of items that are not running any Motr command now.
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
