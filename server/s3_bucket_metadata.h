#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_BUCKET_METADATA_H__
#define __MERO_FE_S3_SERVER_S3_BUCKET_METADATA_H__

#include <functional>
#include <string>
#include <map>

#include "s3_bucket_acl.h"
#include "s3_asyncop_context_base.h"

class S3BucketMetadata;

enum class S3BucketMetadataOpType {
  r,
  w
}

class S3BucketMetadataRWContext : public S3AsyncOpContextBase {
  S3BucketMetadata* metadata;
  S3BucketMetadataOpType operation_type;
public:
  S3BucketMetadataRWContext(S3RequestObject *req, std::function<void(void)> handler, S3BucketMetadata *metadata, S3BucketMetadataOpType type) : S3AsyncOpContextBase(req, handler), metadata(metadata), operation_type(type) {}

  S3BucketMetadata* get_metadata() {
    return metadata;
  }

  S3BucketMetadataOpType operation_type() {
    return operation_type;
  }
};

enum class S3BucketMetadataState {
  empty,    // Initial state, no lookup done
  present,  // Metadata exists and was read successfully
  absent,   // Metadata not present in store.
  saved     // Metadata saved to store.
}

class S3BucketMetadata {
  // Holds mainly system-defined metadata (creation date etc)
  // Partially supported on need bases, some of these are placeholders
private:
  // The name for a Bucket
  // http://docs.aws.amazon.com/AmazonS3/latest/dev/BucketRestrictions.html
  std::string bucket_name;

  std::map<std::string, std::string> system_defined;

  S3BucketACL bucket_ACL;

  S3BucketMetadataState state;

private:
  // AWS recommends that all bucket names comply with DNS naming convention
  // See Bucket naming restrictions in above link.
  void validate_bucket_name();

  // Any other validations we want to do on metadata
  void validate();
public:
  S3BucketMetadata(S3RequestObject *req);

  void async_load(std::function<void(void)> handler);

  S3BucketMetadataState state() {
    return state;
  }

  // Streaming to json
  std::string to_json();

  void from_json(std::string content);
};

#endif
