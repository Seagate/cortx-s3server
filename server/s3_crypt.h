#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_CRYPT_H__
#define __MERO_FE_S3_SERVER_S3_CRYPT_H__

#include <openssl/md5.h>
#include <assert.h>
#include <stdio.h>
using namespace std;
class MD5hash {
private:
  unsigned char md5_digest[MD5_DIGEST_LENGTH + 1] = {'\0'};
  char md5_digest_chars[(MD5_DIGEST_LENGTH * 2) + 1] = {'\0'};
  MD5_CTX md5ctx;
  int status;

public:
  MD5hash() {
    status = MD5_Init(&md5ctx);
    assert(status);
  }
 
  void Update(const char *input, size_t length);
  char * Final(); 
};
#endif
