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

#include "s3_object_versioning_helper.h"
#include <chrono>
#include "base64.h"
#include "s3_common_utilities.h"

unsigned long long S3ObjectVersioingHelper::get_epoch_time_in_ms() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()).count();
}

std::string S3ObjectVersioingHelper::generate_new_epoch_time() {
  std::string milliseconds_since_epoch_str;
  // get current epoch time in ms
  unsigned long long milliseconds_since_epoch = get_epoch_time_in_ms();
  // flip bits
  milliseconds_since_epoch = ~milliseconds_since_epoch;
  // convert retrived current epoch time into string
  milliseconds_since_epoch_str = std::to_string(milliseconds_since_epoch);
  return milliseconds_since_epoch_str;
}

std::string S3ObjectVersioingHelper::get_versionid_from_epoch_time(
    const std::string& milliseconds_since_epoch) {
  // encode the current epoch time
  std::string versionid = base64_encode(
      reinterpret_cast<const unsigned char*>(milliseconds_since_epoch.c_str()),
      milliseconds_since_epoch.length());
  // remove padding characters that is '='
  // to make version id as url encoding format
  S3CommonUtilities::find_and_replaceall(versionid, "=", "");
  return versionid;
}

std::string S3ObjectVersioingHelper::generate_keyid_from_versionid(
    std::string versionid) {
  // add padding if requried;
  versionid = versionid.append((3 - versionid.size() % 3) % 3, '=');
  // base64 decoding of versionid
  // decoded versionid is the epoch time in milli seconds
  std::string milliseconds_since_epoch = base64_decode(versionid);
  return milliseconds_since_epoch;
}