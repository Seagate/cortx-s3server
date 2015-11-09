/*
 * COPYRIGHT 2015 SEAGATE LLC
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
 * Original author:  Rajesh Nambiar   <rajesh.nambiar@seagate.com>
 * Original creation date: 1-Oct-2015
 */

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
