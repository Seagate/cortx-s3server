#pragma once

#ifndef __S3_SERVER_S3_AWS_ETAG_H__
#define __S3_SERVER_S3_AWS_ETAG_H__

#include <gtest/gtest_prod.h>
#include <string>
#include "s3_log.h"

// Used to generate Etag for multipart uploads.
class S3AwsEtag {
  std::string hex_etag;
  std::string final_etag;
  int part_count;

  // Helpers
  int hex_to_dec(char ch);
  std::string convert_hex_bin(std::string hex);

 public:
  S3AwsEtag() : part_count(0) {}

  void add_part_etag(std::string etag);
  std::string finalize();
  std::string get_final_etag();
  FRIEND_TEST(S3AwsEtagTest, Constructor);
  FRIEND_TEST(S3AwsEtagTest, HexToDec);
  FRIEND_TEST(S3AwsEtagTest, HexToDecInvalid);
  FRIEND_TEST(S3AwsEtagTest, HexToBinary);
  FRIEND_TEST(S3AwsEtagTest, AddPartEtag);
  FRIEND_TEST(S3AwsEtagTest, Finalize);
  FRIEND_TEST(S3AwsEtagTest, GetFinalEtag);
};

#endif
