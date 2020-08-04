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

#include "s3_datetime.h"
#include <string.h>
#include "s3_log.h"

S3DateTime::S3DateTime() : is_valid(true) {
  memset(&point_in_time, 0, sizeof(struct tm));
}

bool S3DateTime::is_OK() { return is_valid; }

void S3DateTime::init_current_time() {
  time_t t = time(NULL);
  struct tm *tmp = gmtime_r(&t, &point_in_time);
  if (tmp == NULL) {
    s3_log(S3_LOG_ERROR, "", "gmtime error\n");
    is_valid = false;
  }
}

void S3DateTime::init_with_fmt(std::string time_str, std::string format) {
  memset(&point_in_time, 0, sizeof(struct tm));
  strptime(time_str.c_str(), format.c_str(), &point_in_time);
}

void S3DateTime::init_with_gmt(std::string time_str) {
  init_with_fmt(time_str, S3_GMT_DATETIME_FORMAT);
}

void S3DateTime::init_with_iso(std::string time_str) {
  init_with_fmt(time_str, S3_ISO_DATETIME_FORMAT);
}

std::string S3DateTime::get_isoformat_string() {
  return get_format_string(S3_ISO_DATETIME_FORMAT);
}

std::string S3DateTime::get_gmtformat_string() {
  return get_format_string(S3_GMT_DATETIME_FORMAT);
}

std::string S3DateTime::get_format_string(std::string format) {
  std::string formatted_time = "";
  char timebuffer[100] = {0};
  if (is_OK()) {
    if (strftime(timebuffer, sizeof(timebuffer), format.c_str(),
                 &point_in_time) == 0) {
      s3_log(S3_LOG_ERROR, "", "strftime returned 0\n");
      is_valid = false;
    } else {
      is_valid = true;
      formatted_time = timebuffer;
    }
  }
  return formatted_time;
}
