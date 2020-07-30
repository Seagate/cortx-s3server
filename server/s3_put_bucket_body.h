#pragma once

#ifndef __S3_SERVER_S3_PUT_BUCKET_BODY_H__
#define __S3_SERVER_S3_PUT_BUCKET_BODY_H__

#include <string>

class S3PutBucketBody {
  std::string xml_content;
  bool is_valid;
  std::string location_constraint;
  bool parse_and_validate();

 public:
  S3PutBucketBody(std::string& xml);
  virtual bool isOK();
  virtual std::string get_location_constraint();
};

#endif
