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

#include "s3_perf_logger.h"
#include "s3_option.h"

S3PerfLogger* S3PerfLogger::instance;

S3PerfLogger::S3PerfLogger(const std::string& log_file) {

  if (!instance) {
    perf_file.open(log_file.c_str(), std::ios_base::ate);

    if (perf_file.is_open()) {
      instance = this;
    }
  }
}

void S3PerfLogger::write(const char* perf_text, const char* req_id,
                         size_t elapsed_time) {

  if (instance && elapsed_time != (size_t)(-1)) {
    instance->perf_file << (req_id && req_id[0] ? req_id : "<unknown>") << ' '
                        << perf_text << ':' << elapsed_time << '\n';
  }
}
