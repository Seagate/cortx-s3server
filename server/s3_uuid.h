#pragma once

#ifndef __S3_SERVER_S3_UUID_H__
#define __S3_SERVER_S3_UUID_H__
#define MAX_UUID_STR_SIZE 37

#include <uuid/uuid.h>

class S3Uuid {
  uuid_t uuid;
  std::string str_uuid;

 public:
  S3Uuid() {
    char cstr_uuid[MAX_UUID_STR_SIZE];
    uuid_generate(uuid);
    uuid_unparse(uuid, cstr_uuid);
    str_uuid = cstr_uuid;
  }

  bool is_null() { return uuid_is_null(uuid); }

  int parse() { return uuid_parse(str_uuid.c_str(), uuid); }

  std::string get_string_uuid() { return str_uuid; }

  bool operator==(const S3Uuid& seconduuid) {
    return uuid_compare(this->uuid, seconduuid.uuid) == 0;
  }

  bool operator!=(const S3Uuid& second_uuid) {
    return uuid_compare(this->uuid, second_uuid.uuid) != 0;
  }

  const unsigned char* ptr() const { return uuid; }
};
#endif
