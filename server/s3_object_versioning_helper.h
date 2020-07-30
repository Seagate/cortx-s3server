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
