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

#include "s3_object_versioning_helper.h"

#include <chrono>
#include <cmath>
#include <cstdint>

#include "base62.h"
#include "base64.h"
#include "s3_common_utilities.h"
#include "s3_uuid.h"

// Version ID timestamps are fixed to this number of characters
constexpr size_t kVersionIdTsLen = 8;
// As the version ID timestamp is encoded in Base62, the maximum value
// for 8-characters is 62^8 - 1. This is the maximum time interval in ms.
constexpr uint64_t kMaxTimeStampCount = 218340105584895;

uint64_t S3ObjectVersioningHelper::generate_timestamp(
    const std::chrono::system_clock::time_point& tp) {
  using UnsignedMillis = std::chrono::duration<uint64_t, std::milli>;
  const auto ms_since_epoch = std::chrono::time_point_cast<UnsignedMillis>(tp)
                                  .time_since_epoch()
                                  .count();
  return kMaxTimeStampCount - ms_since_epoch;
}

std::string S3ObjectVersioningHelper::get_versionid_from_timestamp(
    uint64_t ts) {
  auto version_ts = base62::base62_encode(ts, kVersionIdTsLen);

  // TODO: use 18-byte random data for more random bits, and no padding
  S3Uuid uuid;
  auto uuid_str = base64_encode(uuid.ptr(), sizeof(uuid_t));
  S3CommonUtilities::find_and_replaceall(uuid_str, "=", "");
  // TODO: URL-safe, use alternative alphabet to avoid replacement
  S3CommonUtilities::find_and_replaceall(uuid_str, "+", "-");
  S3CommonUtilities::find_and_replaceall(uuid_str, "/", "_");

  return version_ts + uuid_str;
}
