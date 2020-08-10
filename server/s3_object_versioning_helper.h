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

#ifndef __S3_SERVER_S3_OBJECT_VERSIONING_HELPER_H__
#define __S3_SERVER_S3_OBJECT_VERSIONING_HELPER_H__

#include <iostream>

class S3ObjectVersioingHelper {
 public:
  S3ObjectVersioingHelper() = delete;
  S3ObjectVersioingHelper(const S3ObjectVersioingHelper&) = delete;
  S3ObjectVersioingHelper& operator=(const S3ObjectVersioingHelper&) = delete;

 private:
  // method returns epoch time value in ms
  static unsigned long long get_epoch_time_in_ms();

 public:
  static std::string generate_new_epoch_time();

  // get version id from the epoch time value
  static std::string get_versionid_from_epoch_time(
      const std::string& milliseconds_since_epoch);

  // get_epoch_time_from_versionid
  static std::string generate_keyid_from_versionid(std::string versionid);
};
#endif
