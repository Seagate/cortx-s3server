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

#include "s3_md5_hash.h"

MD5hash::MD5hash() {
  status = MD5_Init(&md5ctx);
}

int MD5hash::Update(const char *input, size_t length) {
  if (status == 0) {
    return -1;  // failure
  }
  if (length) {
    status = MD5_Update(&md5ctx, input, length);
    if (status == 0) {
      return -1;  // failure
    }
  }
  return 0;  // success
}

int MD5hash::Finalize() {
  status = MD5_Final(md5_digest,&md5ctx);
  if (status == 0) {
    return -1;  // failure
  }

  for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
    sprintf((char *)(&md5_digest_chars[i*2]), "%02x", (int)md5_digest[i]);
  }
  return 0;
}

std::string MD5hash::get_md5_string() {
  if (status == 0) {
    return std::string("");  // failure
  }
  return std::string(md5_digest_chars);
}
