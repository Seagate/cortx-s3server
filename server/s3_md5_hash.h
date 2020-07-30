#pragma once

#ifndef __S3_SERVER_S3_MD5_HASH_H__
#define __S3_SERVER_S3_MD5_HASH_H__

#include <string>
#include <gtest/gtest_prod.h>
#include <openssl/md5.h>

class MD5hash {
  MD5_CTX md5ctx;
  unsigned char md5_digest[MD5_DIGEST_LENGTH];
  int status;
  bool is_finalized = false;

 public:
  MD5hash();
  int Update(const char *input, size_t length);
  int Finalize();

  std::string get_md5_string();
  std::string get_md5_base64enc_string();

  FRIEND_TEST(MD5HashTest, Constructor);
  FRIEND_TEST(MD5HashTest, FinalBasic);
  FRIEND_TEST(MD5HashTest, FinalEmptyStr);
  FRIEND_TEST(MD5HashTest, FinalNumeral);
};
#endif
