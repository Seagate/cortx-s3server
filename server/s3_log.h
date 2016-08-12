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

#ifndef __MERO_FE_S3_SERVER_LOG_H__
#define __MERO_FE_S3_SERVER_LOG_H__

#include <glog/logging.h>
#include <memory>

#define S3_LOG_FATAL google::GLOG_FATAL
#define S3_LOG_ERROR google::GLOG_ERROR
#define S3_LOG_WARN google::GLOG_WARNING
#define S3_LOG_INFO google::GLOG_INFO
#define S3_LOG_DEBUG (S3_LOG_FATAL + 1)

extern int s3log_level;

// Note:
// 1. Google glog doesn't have a separate severity level for DEBUG logs.
//    So we map our DEBUG logs to INFO level. This level promotion happens
//    only if S3 log level is set to DEBUG.
// 2. Logging a FATAL message terminates the program (after the message is
//    logged).
#define s3_log(loglevel, fmt, ...)                                     \
  do {                                                                 \
    int glog_level = loglevel;                                         \
    if ((loglevel == S3_LOG_DEBUG) && (s3log_level == S3_LOG_DEBUG)) { \
      glog_level = S3_LOG_INFO;                                        \
    }                                                                  \
    if (glog_level != S3_LOG_DEBUG) {                                  \
      int log_buf_len = snprintf(NULL, 0, fmt "\n", ##__VA_ARGS__);    \
      log_buf_len++;                                                   \
      std::unique_ptr<char> log_buf(new char[log_buf_len]);            \
      snprintf(log_buf.get(), log_buf_len, fmt "\n", ##__VA_ARGS__);   \
      if (glog_level == S3_LOG_INFO) {                                 \
        LOG(INFO) << log_buf.get();                                    \
      } else if (glog_level == S3_LOG_WARN) {                          \
        LOG(WARNING) << log_buf.get();                                 \
      } else if (glog_level == S3_LOG_ERROR) {                         \
        LOG(ERROR) << log_buf.get();                                   \
      } else if (glog_level == S3_LOG_FATAL) {                         \
        LOG(FATAL) << log_buf.get();                                   \
      }                                                                \
    }                                                                  \
  } while (0)

int init_log(char *process_name);
void fini_log();
void flushall_log();

#endif  // __MERO_FE_S3_SERVER_LOG_H__
