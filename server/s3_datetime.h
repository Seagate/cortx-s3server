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

#ifndef __S3_SERVER_S3_DATETIME_H__
#define __S3_SERVER_S3_DATETIME_H__

#include <stdlib.h>
#include <time.h>
#include <string>

// UTC == GMT we always save/give out UTC/GMT time with different format
#define S3_GMT_DATETIME_FORMAT "%a, %d %b %Y %H:%M:%S GMT"
#define S3_ISO_DATETIME_FORMAT "%Y-%m-%dT%T.000Z"

// Helper to store DateTime in KV store in Json
class S3DateTime {
  struct tm point_in_time;
  bool is_valid;

  void init_with_fmt(std::string time_str, std::string format);
  std::string get_format_string(std::string format);

 public:
  S3DateTime();
  void init_current_time();
  void init_with_gmt(std::string time_str);
  void init_with_iso(std::string time_str);

  // Returns if the Object state is valid.
  bool is_OK();

  std::string get_isoformat_string();
  std::string get_gmtformat_string();
  friend class S3DateTimeTest;
};

#endif
