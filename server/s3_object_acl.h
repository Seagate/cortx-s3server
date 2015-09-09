#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_OBJECT_ACL_H__
#define __MERO_FE_S3_SERVER_S3_OBJECT_ACL_H__

#include <string>
// #include <json.h>

class S3ObjectACL {
  // TODO

public:
  std::string to_json();

  void from_json(std::string content);
};

#endif
