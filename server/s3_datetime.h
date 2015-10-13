#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_DATETIME_H__
#define __MERO_FE_S3_SERVER_S3_DATETIME_H__


#include <time.h>
#include <stdlib.h>
#include <string>

#define S3_GMT_DATETIME_FORMAT "%a, %d %b %Y %H:%M:%S %Z"

// Helper to store DateTime in KV store in Json
class S3DateTime {
  struct tm current_tm;
  bool is_valid;

public:
  S3DateTime();
  void init_current_time();

  // Returns if the Object state is valid.
  bool is_OK();

  std::string get_GMT_string();
};

#endif
