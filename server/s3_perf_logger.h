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
 * Original author:  Kaustubh Deorukhkar   <kaustubh.deorukhkar@seagate.com>
 * Original creation date: 8-Jan-2016
 */

#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_PERF_LOGGER_H__
#define __MERO_FE_S3_SERVER_S3_PERF_LOGGER_H__

#include <fstream>
#include <iostream>

// Not thread-safe, but we are single threaded.
class S3PerfLogger {
 private:
  static S3PerfLogger* instance;
  S3PerfLogger(std::string log_file);

  std::ofstream perf_file;

 public:
  ~S3PerfLogger();

  // Opens the Performance log file
  static void initialize(std::string log_file = "/var/log/seagate/s3_perf.log");

  static S3PerfLogger* get_instance();

  void write(const char* perf_text, size_t elapsed_time);

  // Closes the Performance log file
  static void finalize();
};

#define LOG_PERF(perf_text, elapsed_time) \
  S3PerfLogger::get_instance()->write(perf_text, elapsed_time);

#endif
