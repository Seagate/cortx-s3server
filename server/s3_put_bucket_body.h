#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_PUT_BUCKET_BODY_H__
#define __MERO_FE_S3_SERVER_S3_PUT_BUCKET_BODY_H__

#include <string>

class S3PutBucketBody {
  std::string xml_content;
  bool is_valid;
  std::string location_constraint;
  bool parse_and_validate();

public:
  S3PutBucketBody(std::string& xml);
  bool isOK();
  std::string get_location_constraint();
};

#endif
