/*
 * COPYRIGHT 2016 SEAGATE LLC
 *
 * THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
 * HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE TECHNOLOGY
 * LIMITED, ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
 * THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
 * BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
 * USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
 * EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.
 *
 * YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
 * THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
 * http://www.seagate.com/contact
 *
 * Original author:  Rajesh Nambiar   <rajesh.nambiar@seagate.com>
 * Original author:  Kaustubh Deorukhkar   <kaustubh.deorukhkar@seagate.com>
 * Original author:  Vinayak Kale <vinayak.kale@seagate.com>
 * Original creation date: 2-March-2016
 */

#pragma once

#ifndef __S3_SERVER_LOG_H__
#define __S3_SERVER_LOG_H__
#include <iostream>
#include <string>
#include <glog/logging.h>
#include <syslog.h>
#include <time.h>
#include <memory>
#include <inttypes.h>

#define S3_LOG_FATAL google::GLOG_FATAL
#define S3_LOG_ERROR google::GLOG_ERROR
#define S3_LOG_WARN google::GLOG_WARNING
#define S3_LOG_INFO google::GLOG_INFO
#define S3_LOG_DEBUG (S3_LOG_FATAL + 1)

#define S3_DEFAULT_REQID "-"

extern int s3log_level;

// Note:
// 1. Google glog doesn't have a separate severity level for DEBUG logs.
//    So we map our DEBUG logs to INFO level. This level promotion happens
//    only if S3 log level is set to DEBUG.
// 2. Logging a FATAL message terminates the program (after the message is
//    logged).
#define s3_log(loglevel, requestid, fmt, ...)                           \
  do {                                                                  \
    int s3_log__glog_level__ = (loglevel);                              \
    if ((s3_log__glog_level__ == S3_LOG_DEBUG) &&                       \
        (s3log_level == S3_LOG_DEBUG)) {                                \
      s3_log__glog_level__ = S3_LOG_INFO;                               \
    }                                                                   \
    std::string s3_log__req_id__ = (requestid);                         \
    s3_log__req_id__ =                                                  \
        s3_log__req_id__.empty() ? S3_DEFAULT_REQID : s3_log__req_id__; \
    if (s3_log__glog_level__ != S3_LOG_DEBUG) {                         \
      int s3_log__log_buf_len__ =                                       \
          snprintf(NULL, 0, "[%s] [ReqID: %s] " fmt "\n", __func__,     \
                   s3_log__req_id__.c_str(), ##__VA_ARGS__);            \
      s3_log__log_buf_len__++;                                          \
      std::unique_ptr<char[]> s3_log__log_buf__(                        \
          new char[s3_log__log_buf_len__]);                             \
      snprintf(s3_log__log_buf__.get(), s3_log__log_buf_len__,          \
               "[%s] [ReqID: %s] " fmt "\n", __func__,                  \
               s3_log__req_id__.c_str(), ##__VA_ARGS__);                \
      if (s3_log__glog_level__ == S3_LOG_INFO) {                        \
        LOG(INFO) << s3_log__log_buf__.get();                           \
      } else if (s3_log__glog_level__ == S3_LOG_WARN) {                 \
        LOG(WARNING) << s3_log__log_buf__.get();                        \
      } else if (s3_log__glog_level__ == S3_LOG_ERROR) {                \
        LOG(ERROR) << s3_log__log_buf__.get();                          \
      } else if (s3_log__glog_level__ == S3_LOG_FATAL) {                \
        LOG(FATAL) << s3_log__log_buf__.get();                          \
      }                                                                 \
    }                                                                   \
  } while (0)

// Note:
// 1. Use syslog defined severity levels as loglevel.
// 2. syslog messages will always be sent irrespective of s3loglevel.
#define s3_syslog(loglevel, fmt, ...)     \
  do {                                    \
    syslog(loglevel, fmt, ##__VA_ARGS__); \
  } while (0)

// returns timestamp in format: "yyyy:mm:dd hh:mm:ss.uuuuuu"
static inline std::string s3_get_timestamp() {
  struct timespec ts;
  struct tm result;
  char date_time[20];
  char timestamp[30];

  clock_gettime(CLOCK_REALTIME, &ts);
  tzset();
  if (localtime_r(&ts.tv_sec, &result) == NULL) {
    return std::string();
  }
  strftime(date_time, sizeof(date_time), "%Y:%m:%d %H:%M:%S", &result);
  snprintf(timestamp, sizeof(timestamp), "%s.%06li", date_time,
           ts.tv_nsec / 1000);
  return std::string(timestamp);
}

int init_log(char *process_name);
void fini_log();
void flushall_log();

#endif  // __S3_SERVER_LOG_H__
