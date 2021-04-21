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

#include "s3_bucket_metadata_cache.h"
#include "s3_bucket_metadata_proxy.h"
#include "s3_datetime.h"
#include "s3_log.h"
#include "s3_request_object.h"

S3BucketMetadataProxy::S3BucketMetadataProxy(
    std::shared_ptr<S3RequestObject> s3_req_obj, const std::string& bucket)
    : S3BucketMetadata(std::move(s3_req_obj), bucket) {}

S3BucketMetadataProxy::~S3BucketMetadataProxy() = default;

void S3BucketMetadataProxy::on_load(S3BucketMetadataState state,
                                    const S3BucketMetadata& src) {

  s3_log(S3_LOG_DEBUG, stripped_request_id, "%s Entry", __func__);

  if (S3BucketMetadataState::present == state) {
    auto request_backup = std::move(this->request);

    static_cast<S3BucketMetadata&>(*this) = src;

    request = std::move(request_backup);
    request_id = request->get_request_id();
    stripped_request_id = request->get_stripped_request_id();

    handler_on_success();
  } else {
    this->state = state;

    handler_on_failed();
  }
  s3_log(S3_LOG_DEBUG, "", "%s Exit", __func__);
}

void S3BucketMetadataProxy::initialize() {
  account_name = request->get_account_name();
  account_id = request->get_account_id();
  user_name = request->get_user_name();
  user_id = request->get_user_id();

  system_defined_attribute["Owner-Canonical-id"] = owner_canonical_id =
      request->get_canonical_id();

  // Set the defaults
  S3DateTime current_time;
  current_time.init_current_time();
  system_defined_attribute["Date"] = current_time.get_isoformat_string();

  auto& location_constraint = system_defined_attribute["LocationConstraint"];
  if (location_constraint.empty()) {
    location_constraint = "us-west-2";
  }
}

void S3BucketMetadataProxy::on_save(S3BucketMetadataState state) {
  s3_log(S3_LOG_DEBUG, stripped_request_id, "%s Entry\n", __func__);
  this->state = state;

  if (S3BucketMetadataState::present == state) {
    handler_on_success();
  } else {
    handler_on_failed();
  }
}

void S3BucketMetadataProxy::on_update(S3BucketMetadataState state) {
  s3_log(S3_LOG_DEBUG, stripped_request_id, "%s Entry\n", __func__);
  this->state = state;

  if (S3BucketMetadataState::present == state) {
    handler_on_success();
  } else {
    handler_on_failed();
  }
}

void S3BucketMetadataProxy::on_remove(S3BucketMetadataState state) {
  s3_log(S3_LOG_DEBUG, stripped_request_id, "%s Entry\n", __func__);
  this->state = state;

  if (S3BucketMetadataState::missing == state) {
    handler_on_success();
  } else {
    handler_on_failed();
  }
}

void S3BucketMetadataProxy::load(std::function<void(void)> on_success,
                                 std::function<void(void)> on_failed) {

  s3_log(S3_LOG_DEBUG, stripped_request_id, "%s Entry\n", __func__);

  handler_on_success = std::move(on_success);
  handler_on_failed = std::move(on_failed);

  using namespace std::placeholders;

  S3BucketMetadataCache::get_instance()->fetch(
      *this, std::bind(&S3BucketMetadataProxy::on_load, this, _1, _2));
}

void S3BucketMetadataProxy::save(std::function<void(void)> on_success,
                                 std::function<void(void)> on_failed) {

  s3_log(S3_LOG_DEBUG, stripped_request_id, "%s Entry\n", __func__);

  handler_on_success = std::move(on_success);
  handler_on_failed = std::move(on_failed);

  initialize();

  S3BucketMetadataCache::get_instance()->save(
      *this,
      std::bind(&S3BucketMetadataProxy::on_save, this, std::placeholders::_1));
}

void S3BucketMetadataProxy::update(std::function<void(void)> on_success,
                                   std::function<void(void)> on_failed) {

  s3_log(S3_LOG_DEBUG, stripped_request_id, "%s Entry\n", __func__);

  handler_on_success = std::move(on_success);
  handler_on_failed = std::move(on_failed);

  S3BucketMetadataCache::get_instance()->update(
      *this, std::bind(&S3BucketMetadataProxy::on_update, this,
                       std::placeholders::_1));
}

void S3BucketMetadataProxy::remove(std::function<void(void)> on_success,
                                   std::function<void(void)> on_failed) {

  s3_log(S3_LOG_DEBUG, stripped_request_id, "%s Entry\n", __func__);

  handler_on_success = std::move(on_success);
  handler_on_failed = std::move(on_failed);

  S3BucketMetadataCache::get_instance()->remove(
      *this, std::bind(&S3BucketMetadataProxy::on_remove, this,
                       std::placeholders::_1));
}
