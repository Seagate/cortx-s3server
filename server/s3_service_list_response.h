#pragma once

#ifndef __S3_SERVER_S3_SERVICE_LIST_RESPONSE_H__
#define __S3_SERVER_S3_SERVICE_LIST_RESPONSE_H__

#include <memory>
#include <string>
#include <vector>

#include "s3_bucket_metadata.h"

class S3ServiceListResponse {
  std::string owner_name;
  std::string owner_id;
  std::vector<std::shared_ptr<S3BucketMetadata>> bucket_list;

  // Generated xml response
  std::string response_xml;

 public:
  S3ServiceListResponse();
  void set_owner_name(std::string name);
  void set_owner_id(std::string id);
  void add_bucket(std::shared_ptr<S3BucketMetadata> bucket);
  std::string& get_xml();
  int get_bucket_count() const { return bucket_list.size(); }
};

#endif
