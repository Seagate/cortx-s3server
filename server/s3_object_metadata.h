
#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_OBJECT_METADATA_H__
#define __MERO_FE_S3_SERVER_S3_OBJECT_METADATA_H__

#include <map>
#include <string>

#include "s3_object_acl.h"

class S3ObjectMetadata {
  // Holds system-defined metadata (creation date etc)
  // Holds user-defined metadata (names must begin with "x-amz-meta-")
  // Partially supported on need bases, some of these are placeholders
private:
  // The name for a key is a sequence of Unicode characters whose UTF-8 encoding is at most 1024 bytes long. http://docs.aws.amazon.com/AmazonS3/latest/dev/UsingMetadata.html#object-keys
  std::string object_key_uri;

  std::map<std::string, std::string> system_defined;

  std::map<std::string, std::string> user_defined;

  S3ObjectACL object_ACL;

private:
  // Any validations we want to do on metadata
  void validate();
public:

  void load_from_clovis();

  std::string to_json();

  void from_json(std::string content);
};

#endif
