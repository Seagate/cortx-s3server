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

S3PerfLogger* S3PerfLogger::instance = NULL;

S3PerfLogger::S3PerfLogger(std::string log_file) {
  perf_file.open(log_file.c_str());
}

S3PerfLogger::~S3PerfLogger() {
  perf_file.close();
}

void S3PerfLogger::initialize(std::string log_file) {
  if(!instance){
    instance = new S3PerfLogger(log_file);
  }
}

S3PerfLogger* S3PerfLogger::get_instance() {
  if(!instance){
    S3PerfLogger::initialize();
  }
  return instance;
}

void S3PerfLogger::write(const char* perf_text, size_t elapsed_time) {
  if (instance && S3Option::get_instance()->s3_performance_enabled()) {
    if (elapsed_time != SIZE_MAX) {
      perf_file << perf_text << ":" << elapsed_time << std::endl;  // endl flushes.
    }
  }
}

void S3PerfLogger::finalize(){
  delete instance;
}
