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

#include <cassert>

#define S3_BUCKET_METADATA_CACHE_DEFINITION
#define S3_BUCKET_METADATA_V1_DEFINITION

#include "s3_bucket_metadata_cache.h"
#include "s3_bucket_metadata_v1.h"
#include "s3_factory.h"
#include "s3_log.h"
#include "s3_request_object.h"

S3BucketMetadataCache* p_bucket_metadata_cache;

std::unique_ptr<S3BucketMetadataV1>
S3MotrBucketMetadataFactory::create_motr_bucket_metadata_obj(
    const S3BucketMetadata& src) {

  return std::unique_ptr<S3BucketMetadataV1>(new S3BucketMetadataV1(src));
}

S3BucketMetadataCache::Item::Item(const S3BucketMetadata& src)
    : p_value(new S3BucketMetadata(src)) {

  s3_log(S3_LOG_DEBUG, "", "%s Ctor", __func__);
}

S3BucketMetadataCache::Item::~Item() {
  assert(can_remove());
  assert(fetch_waiters.empty());
  assert(!on_changed);
}

bool S3BucketMetadataCache::Item::can_remove() const {
  return CurrentOp::none == current_op;
}

inline const std::string& S3BucketMetadataCache::Item::get_bucket_name() const {
  assert(p_value);
  return p_value->get_bucket_name();
}

void S3BucketMetadataCache::Item::create_engine(const S3BucketMetadata& src) {
  assert(!p_engine);

  p_engine = p_bucket_metadata_cache->s3_motr_bucket_metadata_factory
                 ->create_motr_bucket_metadata_obj(src);
}

void S3BucketMetadataCache::Item::on_done(S3BucketMetadataState state) {
  s3_log(S3_LOG_DEBUG, nullptr, "%s Entry", __func__);

  assert(CurrentOp::none != this->current_op);
  assert(p_engine != nullptr);

  const auto current_op = this->current_op;
  this->current_op = CurrentOp::none;

  *p_value = *p_engine;
  p_engine.reset();

  update_time = Clock::now();
  p_bucket_metadata_cache->updated(this);

  if (CurrentOp::fetching == current_op) {
    assert(!fetch_waiters.empty());

    while (!fetch_waiters.empty()) {

      auto on_fetch = std::move(fetch_waiters.front());
      fetch_waiters.pop();

      on_fetch(state, *p_value);
    }
  } else {
    assert(this->on_changed);
    auto on_changed = std::move(this->on_changed);
    on_changed(state);
  }
  if (S3BucketMetadataState::failed == state ||
      S3BucketMetadataState::failed_to_launch == state ||
      CurrentOp::deleting == current_op) {

    p_bucket_metadata_cache->remove_item(p_value->get_bucket_name());
  }
  s3_log(S3_LOG_DEBUG, nullptr, "%s Exit", __func__);
}

bool S3BucketMetadataCache::Item::check_expiration() {

  const auto time_lasted_sec = std::chrono::duration_cast<std::chrono::seconds>(
      access_time - update_time);
  const auto ret =
      time_lasted_sec.count() >= p_bucket_metadata_cache->expire_interval_sec;

  if (ret) {
    s3_log(S3_LOG_DEBUG, p_value->get_request_id(),
           "Cache entry for \"%s\" is expired",
           p_value->get_bucket_name().c_str());
  }
  return ret;
}

void S3BucketMetadataCache::Item::fetch(const S3BucketMetadata& src,
                                        FetchHanderType callback, bool force) {
  s3_log(S3_LOG_DEBUG, src.get_request_id(), "%s Entry", __func__);

  const bool is_expired = check_expiration();

  if (!is_expired && !force &&
      p_value->get_state() == S3BucketMetadataState::present) {

    callback(S3BucketMetadataState::present, *p_value);
    return;
  }
  if (CurrentOp::deleting == current_op || CurrentOp::saving == current_op) {

    s3_log(S3_LOG_INFO, src.get_request_id(),
           "Bucket metadata cache: modify operation is in progress");

    callback(S3BucketMetadataState::failed, *p_value);
    return;
  }
  fetch_waiters.push(std::move(callback));

  if (CurrentOp::fetching != current_op) {
    current_op = CurrentOp::fetching;
    create_engine(src);

    p_engine->load(src, std::bind(&S3BucketMetadataCache::Item::on_done, this,
                                  std::placeholders::_1));
  }
  s3_log(S3_LOG_DEBUG, nullptr, "%s Exit", __func__);
}

void S3BucketMetadataCache::Item::save(const S3BucketMetadata& src,
                                       StateHandlerType callback) {
  s3_log(S3_LOG_DEBUG, src.get_request_id(), "%s Entry", __func__);

  if (current_op != CurrentOp::none) {
    s3_log(S3_LOG_ERROR, src.get_request_id(),
           "Another operation is beeing made");
    callback(S3BucketMetadataState::failed);
    return;
  }
  current_op = CurrentOp::saving;
  on_changed = std::move(callback);

  create_engine(src);
  p_engine->save(src, std::bind(&S3BucketMetadataCache::Item::on_done, this,
                                std::placeholders::_1));

  s3_log(S3_LOG_DEBUG, nullptr, "%s Exit", __func__);
}

void S3BucketMetadataCache::Item::update(const S3BucketMetadata& src,
                                         StateHandlerType callback) {
  s3_log(S3_LOG_DEBUG, nullptr, "%s Entry", __func__);

  if (current_op != CurrentOp::none) {
    s3_log(S3_LOG_ERROR, "", "Another operation is beeing made");
    callback(S3BucketMetadataState::failed);
    return;
  }
  current_op = CurrentOp::saving;
  on_changed = std::move(callback);

  create_engine(src);
  p_engine->update(src, std::bind(&S3BucketMetadataCache::Item::on_done, this,
                                  std::placeholders::_1));

  s3_log(S3_LOG_DEBUG, nullptr, "%s Exit", __func__);
}

void S3BucketMetadataCache::Item::remove(StateHandlerType callback) {
  s3_log(S3_LOG_DEBUG, nullptr, "%s Entry", __func__);

  if (current_op != CurrentOp::none) {
    s3_log(S3_LOG_ERROR, "", "Another operation is beeing made");
    callback(S3BucketMetadataState::failed);
    return;
  }
  current_op = CurrentOp::deleting;
  on_changed = std::move(callback);

  create_engine(*p_value);
  p_engine->remove(std::bind(&S3BucketMetadataCache::Item::on_done, this,
                             std::placeholders::_1));

  s3_log(S3_LOG_DEBUG, nullptr, "%s Exit", __func__);
}

// ************************************************************************* //
// class S3BucketMetadataCache
// ************************************************************************* //

S3BucketMetadataCache::S3BucketMetadataCache(
    unsigned max_cache_size, unsigned expire_interval_sec,
    unsigned refresh_interval_sec,
    std::shared_ptr<S3MotrBucketMetadataFactory> motr_bucket_metadata_factory)
    : max_cache_size(max_cache_size),
      expire_interval_sec(expire_interval_sec),
      refresh_interval_sec(refresh_interval_sec) {

  if (p_bucket_metadata_cache) {
    s3_log(S3_LOG_FATAL, "",
           "Only one instance of S3BucketMetadataCache is allowed");
  }
  this->s3_motr_bucket_metadata_factory =
      motr_bucket_metadata_factory
          ? std::move(motr_bucket_metadata_factory)
          : std::make_shared<S3MotrBucketMetadataFactory>();
  p_bucket_metadata_cache = this;
}

S3BucketMetadataCache::~S3BucketMetadataCache() {
  p_bucket_metadata_cache = nullptr;
}

void S3BucketMetadataCache::remove_item(std::string bucket_name) {

  assert(sorted_by_access.size() == sorted_by_update.size());
  assert(sorted_by_access.size() == items.size());

  auto map_it = items.find(bucket_name);
  assert(items.end() != map_it);

  auto* p_item = (*map_it).second.get();

  sorted_by_access.erase(p_item->ptr_access);
  sorted_by_update.erase(p_item->ptr_update);
  items.erase(map_it);

  s3_log(S3_LOG_DEBUG, "",
         "Metadata for \"%s\" has been removed from the cache",
         bucket_name.c_str());
}

void S3BucketMetadataCache::shrink() {

  for (auto rit = sorted_by_access.rbegin(); rit != sorted_by_access.rend();
       ++rit) {
    auto* p_item = *rit;

    if (p_item->can_remove()) {

      s3_log(S3_LOG_DEBUG, "",
             "Bucket \"%s\" is selected to be removed from metadata cache",
             p_item->get_bucket_name().c_str());

      remove_item(p_item->get_bucket_name());
      return;
    }
  }
  s3_log(S3_LOG_WARN, "", "Cannt remove any item from bucket metadata cache");
}

S3BucketMetadataCache::Item* S3BucketMetadataCache::get_item(
    const S3BucketMetadata& src) {

  const auto& bucket_name = src.get_bucket_name();

  if (items.end() == items.find(bucket_name) &&
      items.size() >= max_cache_size) {

    s3_log(S3_LOG_DEBUG, src.get_request_id(), "Bucket metadata cache is full");
    shrink();
  }
  auto& sptr = items[bucket_name];
  const bool f_new = !sptr;

  if (f_new) {
    s3_log(S3_LOG_DEBUG, src.get_request_id(),
           "Metadata for \"%s\" bucket is absent in cache",
           bucket_name.c_str());

    sptr.reset(new Item(src));
  } else {
    s3_log(S3_LOG_DEBUG, src.get_request_id(),
           "Metadata for \"%s\" bucket is cached", bucket_name.c_str());
  }
  Item* p_item = sptr.get();
  p_item->access_time = Clock::now();

  if (f_new) {
    p_item->ptr_update =
        sorted_by_update.insert(sorted_by_update.end(), p_item);
  } else {
    sorted_by_access.erase(p_item->ptr_access);
  }
  p_item->ptr_access =
      sorted_by_access.insert(sorted_by_access.begin(), p_item);

  return p_item;
}

void S3BucketMetadataCache::fetch(const S3BucketMetadata& src,
                                  FetchHanderType callback) {

  s3_log(S3_LOG_DEBUG, src.get_stripped_request_id(), "%s Entry", __func__);

  Item* p_item = get_item(src);
  p_item->fetch(src, std::move(callback), this->disabled);

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataCache::save(const S3BucketMetadata& src,
                                 StateHandlerType callback) {

  s3_log(S3_LOG_DEBUG, src.get_stripped_request_id(), "%s Entry", __func__);

  Item* p_item = get_item(src);
  p_item->save(src, std::move(callback));

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataCache::update(const S3BucketMetadata& src,
                                   StateHandlerType callback) {

  s3_log(S3_LOG_DEBUG, src.get_stripped_request_id(), "%s Entry", __func__);

  Item* p_item = get_item(src);
  p_item->update(src, std::move(callback));

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataCache::remove(const S3BucketMetadata& src,
                                   StateHandlerType callback) {

  s3_log(S3_LOG_DEBUG, src.get_stripped_request_id(), "%s Entry", __func__);

  Item* p_item = get_item(src);
  p_item->remove(std::move(callback));

  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataCache::updated(Item* p_item) {
  sorted_by_update.erase(p_item->ptr_update);
  p_item->ptr_update =
      sorted_by_update.insert(sorted_by_update.begin(), p_item);
}
