
#include "s3_datetime.h"
#include <string.h>

S3DateTime::S3DateTime() : is_valid(true) {
  memset(&current_tm, 0, sizeof(struct tm));
}

bool S3DateTime::is_OK() {
  return is_valid;
}

void S3DateTime::init_current_time() {
  time_t t = time(NULL);
  struct tm *tmp = gmtime_r(&t, &current_tm);
  if (tmp == NULL) {
      printf("gmtime error");
      is_valid = false;
  }
}

std::string S3DateTime::get_GMT_string() {
  std::string gmt_time = "";
  char buffer[100] = {0};
  if (is_OK()) {
    if (strftime(buffer, sizeof(buffer), S3_GMT_DATETIME_FORMAT, &current_tm) == 0) {
        printf("strftime returned 0");
        is_valid = false;
    } else {
      is_valid = true;
      gmt_time = buffer;
    }
  }
  return gmt_time;
}
