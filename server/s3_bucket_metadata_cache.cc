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

S3BucketMetadataCache* S3BucketMetadataCache::p_instance;

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
  if (!can_remove()) {
    s3_log(S3_LOG_FATAL, nullptr,
           "Motr operation is in progress while bucket metadata cache instance "
           "is destructing. Such behavior isn't expected and must be fixed.");
  }
}

bool S3BucketMetadataCache::Item::can_remove() const {

  return !p_engine_load && !p_engine_modify;
}

inline const std::string& S3BucketMetadataCache::Item::get_bucket_name() const {
  assert(p_value);
  return p_value->get_bucket_name();
}

void S3BucketMetadataCache::Item::on_load(S3BucketMetadataState state) {
  s3_log(S3_LOG_DEBUG, nullptr, "%s Entry", __func__);
  assert(p_engine_load);

  if (p_engine_modify) {
    // Wait for modify operation has finished
    p_engine_load.reset();
  } else {
    p_engine_modify = std::move(p_engine_load);
    on_done(state);
  }
  s3_log(S3_LOG_DEBUG, nullptr, "%s Exit", __func__);
}

void S3BucketMetadataCache::Item::on_done(S3BucketMetadataState state) {
  s3_log(S3_LOG_DEBUG, nullptr, "%s Entry", __func__);

  assert(CurrentOp::none != this->current_op);
  assert(p_engine_modify != nullptr);

  const auto current_op = this->current_op;
  this->current_op = CurrentOp::none;

  *p_value = *p_engine_modify;
  p_engine_modify.reset();

  update_time = Clock::now();
  S3BucketMetadataCache::p_instance->updated(this);

  while (!fetch_waiters.empty()) {

    auto on_fetch = std::move(fetch_waiters.front());
    fetch_waiters.pop();

    if (on_fetch) {
      on_fetch(state, *p_value);
    }
  }
  if (CurrentOp::saving == current_op || CurrentOp::deleting == current_op) {

    assert(this->on_changed);
    auto on_changed = std::move(this->on_changed);
    on_changed(state);
  }
  if (S3BucketMetadataState::failed == state ||
      S3BucketMetadataState::failed_to_launch == state ||
      CurrentOp::deleting == current_op) {

    S3BucketMetadataCache::p_instance->remove_item(p_value->get_bucket_name());
  }
  s3_log(S3_LOG_DEBUG, nullptr, "%s Exit", __func__);
}

void S3BucketMetadataCache::Item::fetch(const S3BucketMetadata& src,
                                        FetchHandlerType on_fetch) {
  s3_log(S3_LOG_DEBUG, src.get_request_id(), "%s Entry", __func__);

  fetch_waiters.push(std::move(on_fetch));

  const auto seconds_lasted = std::chrono::duration_cast<std::chrono::seconds>(
      access_time - update_time).count();

  const bool is_expired =
      seconds_lasted >= S3BucketMetadataCache::p_instance->expire_interval_sec;

  if (is_expired) {
    s3_log(S3_LOG_DEBUG, src.get_request_id(),
           "Cache entry for \"%s\" is expired", src.get_bucket_name().c_str());
  }
  if (!is_expired && !S3BucketMetadataCache::p_instance->disabled &&
      p_value->get_state() == S3BucketMetadataState::present) {

    s3_log(S3_LOG_DEBUG, src.get_request_id(),
           "Using cached bucket metadata for \"%s\"",
           src.get_bucket_name().c_str());

    // We have to "retain" one S3 request to avoid a case when number of
    // requests to Motr exceeds the number of requests to S3server
    if (seconds_lasted <
            S3BucketMetadataCache::p_instance->refresh_interval_sec ||
        fetch_waiters.size() > 1) {

      on_fetch = std::move(fetch_waiters.front());
      fetch_waiters.pop();

      on_fetch(S3BucketMetadataState::present, *p_value);

      s3_log(S3_LOG_DEBUG, nullptr, "%s Exit", __func__);
      return;

    } else {
      s3_log(S3_LOG_DEBUG, nullptr,
             "Cache entry for \"%s\" needs to be refreshed",
             p_value->get_bucket_name().c_str());
    }
  }
  if (p_engine_modify || p_engine_load) {

    s3_log(S3_LOG_DEBUG, nullptr,
           "Another operation is in progress for \"%s\" bucket. Updated "
           "metadata will be received soon.",
           p_value->get_bucket_name().c_str());

  } else {
    p_engine_load = S3BucketMetadataCache::p_instance->create_engine(*p_value);

    p_engine_load->load(*p_value,
                        std::bind(&S3BucketMetadataCache::Item::on_load, this,
                                  std::placeholders::_1));
    current_op = CurrentOp::fetching;
  }
  s3_log(S3_LOG_DEBUG, nullptr, "%s Exit", __func__);
}

void S3BucketMetadataCache::Item::save(const S3BucketMetadata& src,
                                       StateHandlerType on_save) {
  s3_log(S3_LOG_DEBUG, src.get_stripped_request_id(), "%s Entry", __func__);

  if (p_engine_modify) {

    s3_log(S3_LOG_ERROR, src.get_request_id(),
           "Another modify operation is in progress");
    on_save(S3BucketMetadataState::failed_to_launch);

  } else {
    current_op = CurrentOp::saving;
    on_changed = std::move(on_save);

    p_engine_modify = S3BucketMetadataCache::p_instance->create_engine(src);

    p_engine_modify->save(src, std::bind(&S3BucketMetadataCache::Item::on_done,
                                         this, std::placeholders::_1));
  }
  s3_log(S3_LOG_DEBUG, nullptr, "%s Exit", __func__);
}

void S3BucketMetadataCache::Item::update(const S3BucketMetadata& src,
                                         StateHandlerType on_update) {
  s3_log(S3_LOG_DEBUG, src.get_stripped_request_id(), "%s Entry", __func__);

  if (p_engine_modify) {

    s3_log(S3_LOG_ERROR, src.get_request_id(),
           "Another modify operation is in progress");
    on_update(S3BucketMetadataState::failed_to_launch);

  } else {
    current_op = CurrentOp::saving;
    on_changed = std::move(on_update);

    p_engine_modify = S3BucketMetadataCache::p_instance->create_engine(src);

    p_engine_modify->update(
        src, std::bind(&S3BucketMetadataCache::Item::on_done, this,
                       std::placeholders::_1));
  }
  s3_log(S3_LOG_DEBUG, nullptr, "%s Exit", __func__);
}

void S3BucketMetadataCache::Item::remove(StateHandlerType on_remove) {
  s3_log(S3_LOG_DEBUG, nullptr, "%s Entry", __func__);

  if (p_engine_modify) {

    s3_log(S3_LOG_ERROR, nullptr, "Another modify operation is in progress");
    on_remove(S3BucketMetadataState::failed_to_launch);

  } else {
    current_op = CurrentOp::deleting;
    on_changed = std::move(on_remove);

    p_engine_modify =
        S3BucketMetadataCache::p_instance->create_engine(*p_value);

    p_engine_modify->remove(std::bind(&S3BucketMetadataCache::Item::on_done,
                                      this, std::placeholders::_1));
  }
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

  if (p_instance) {
    s3_log(S3_LOG_FATAL, "",
           "Only one instance of S3BucketMetadataCache is allowed");
  }
  this->s3_motr_bucket_metadata_factory =
      motr_bucket_metadata_factory
          ? std::move(motr_bucket_metadata_factory)
          : std::make_shared<S3MotrBucketMetadataFactory>();
  p_instance = this;
}

S3BucketMetadataCache::~S3BucketMetadataCache() {
  S3BucketMetadataCache::p_instance = nullptr;
}

S3BucketMetadataCache* S3BucketMetadataCache::get_instance() {
  if (!p_instance) {
    s3_log(S3_LOG_FATAL, nullptr,
           "Bucket metadata cache instance isn't created yet");
  }
  return p_instance;
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

bool S3BucketMetadataCache::shrink() {

  for (auto rit = sorted_by_update.rbegin(); rit != sorted_by_update.rend();
       ++rit) {
    auto* p_item = *rit;

    const auto seconds_lasted =
        std::chrono::duration_cast<std::chrono::seconds>(
            Clock::now() - p_item->update_time).count();

    if (seconds_lasted < expire_interval_sec) {

      s3_log(S3_LOG_DEBUG, "",
             "There are no expired entries in bucket metadata cache");
      break;
    }
    if (p_item->can_remove()) {

      s3_log(S3_LOG_DEBUG, "",
             "Bucket \"%s\" is selected to be removed from metadata cache",
             p_item->get_bucket_name().c_str());

      remove_item(p_item->get_bucket_name());
      return true;
    }
  }
  for (auto rit = sorted_by_access.rbegin(); rit != sorted_by_access.rend();
       ++rit) {
    auto* p_item = *rit;

    if (p_item->can_remove()) {

      s3_log(S3_LOG_DEBUG, "",
             "Bucket \"%s\" is selected to be removed from metadata cache",
             p_item->get_bucket_name().c_str());

      remove_item(p_item->get_bucket_name());
      return true;
    }
  }
  s3_log(S3_LOG_WARN, "", "Cannt remove any item from bucket metadata cache");

  return false;
}

S3BucketMetadataCache::Item* S3BucketMetadataCache::get_item(
    const S3BucketMetadata& src) {

  const auto& bucket_name = src.get_bucket_name();
  const bool f_new = (items.end() == items.find(bucket_name));

  // L-part of the expression below contains the predicted number of cache
  // entries after the function returns (without call of shrink())
  if (items.size() + f_new > max_cache_size) {

    s3_log(S3_LOG_DEBUG, src.get_request_id(), "Bucket metadata cache is full");

    if (!shrink()) {
      return nullptr;
    }
  }
  auto& sptr = items[bucket_name];

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
                                  FetchHandlerType on_fetch) {

  s3_log(S3_LOG_DEBUG, src.get_stripped_request_id(), "%s Entry", __func__);

  Item* p_item = get_item(src);

  if (p_item) {
    p_item->fetch(src, std::move(on_fetch));
  } else {
    on_fetch(S3BucketMetadataState::failed_to_launch, src);
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataCache::save(const S3BucketMetadata& src,
                                 StateHandlerType on_save) {

  s3_log(S3_LOG_DEBUG, src.get_stripped_request_id(), "%s Entry", __func__);

  Item* p_item = get_item(src);

  if (p_item) {
    p_item->save(src, std::move(on_save));
  } else {
    on_save(S3BucketMetadataState::failed_to_launch);
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataCache::update(const S3BucketMetadata& src,
                                   StateHandlerType on_update) {

  s3_log(S3_LOG_DEBUG, src.get_stripped_request_id(), "%s Entry", __func__);

  Item* p_item = get_item(src);

  if (p_item) {
    p_item->update(src, std::move(on_update));
  } else {
    on_update(S3BucketMetadataState::failed_to_launch);
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataCache::remove(const S3BucketMetadata& src,
                                   StateHandlerType on_remove) {

  s3_log(S3_LOG_DEBUG, src.get_stripped_request_id(), "%s Entry", __func__);

  Item* p_item = get_item(src);

  if (p_item) {
    p_item->remove(std::move(on_remove));
  } else {
    on_remove(S3BucketMetadataState::failed_to_launch);
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataCache::updated(Item* p_item) {
  sorted_by_update.erase(p_item->ptr_update);
  p_item->ptr_update =
      sorted_by_update.insert(sorted_by_update.begin(), p_item);
}

std::unique_ptr<S3BucketMetadataV1> S3BucketMetadataCache::create_engine(
    const S3BucketMetadata& src) {

  return s3_motr_bucket_metadata_factory->create_motr_bucket_metadata_obj(src);
}
