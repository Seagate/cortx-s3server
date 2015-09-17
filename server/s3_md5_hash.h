#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_MD5_HASH_H__
#define __MERO_FE_S3_SERVER_S3_MD5_HASH_H__

#include <openssl/md5.h>
#include <assert.h>
#include <stdio.h>
#include <string>

class MD5hash {
private:
  unsigned char md5_digest[MD5_DIGEST_LENGTH + 1] = {'\0'};
  char md5_digest_chars[(MD5_DIGEST_LENGTH * 2) + 1] = {'\0'};
  MD5_CTX md5ctx;
  int status;

public:
  MD5hash();
  int Update(const char *input, size_t length);
  int Finalize();

  std::string get_md5_string();
};
#endif
