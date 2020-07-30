#pragma once

#ifndef __S3_SERVER_S3_SHA256_H__
#define __S3_SERVER_S3_SHA256_H__

#include <assert.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <string>

class S3sha256 {
 private:
  unsigned char hash[SHA256_DIGEST_LENGTH] = {'\0'};
  char hex_hash[SHA256_DIGEST_LENGTH * 2] = {'\0'};
  SHA256_CTX context;
  int status;

 public:
  S3sha256();
  void reset();
  bool Update(const char *input, size_t length);
  bool Finalize();

  std::string get_hex_hash();
};
#endif
