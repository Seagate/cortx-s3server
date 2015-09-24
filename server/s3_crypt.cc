#include "s3_crypt.h"
using namespace std;

void MD5hash:: Update(const char *input, size_t length) {
  if (length) {
    status = MD5_Update(&md5ctx, input, length);
    assert(status);
  }
}

char * MD5hash:: Final() {
  status = MD5_Final(md5_digest,&md5ctx);
  assert(status);

  for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
    sprintf((char *)(&md5_digest_chars[i*2]), "%02x", (int)md5_digest[i]);
  }
  return md5_digest_chars;
}
