
#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_COMMON_H__
#define __MERO_FE_S3_SERVER_S3_COMMON_H__

#ifdef __cplusplus
#define EXTERN_C_BLOCK_BEGIN    extern "C" {
#define EXTERN_C_BLOCK_END      }
#define EXTERN_C_FUNC           extern "C"
#else
#define EXTERN_C_BLOCK_BEGIN
#define EXTERN_C_BLOCK_END
#define EXTERN_C_FUNC
#endif

enum class S3OperationCode {
  none,  // Operation on current object.
  list,

  // Common Operations
  acl,

  // Bucket Operations
  location,
  policy,
  logging,
  lifecycle,
  cors,
  notification,
  replicaton,
  tagging,
  requestPayment,
  versioning,
  website,
  listuploads,

  // Object Operations
  initupload,
  partupload,
  completeupload,
  abortupload,
  multidelete
};

enum class S3AsyncOpStatus {
  unknown,
  inprogress,
  success,
  failed
};

enum class S3IOOpStatus {
  saved,
  failed
};

#endif
