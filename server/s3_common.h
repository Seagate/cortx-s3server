/*
 * COPYRIGHT 2015 SEAGATE LLC
 *
 * THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
 * HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE TECHNOLOGY
 * LIMITED, ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
 * THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
 * BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
 * USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
 * EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.
 *
 * YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
 * THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
 * http://www.seagate.com/contact
 *
 * Original author:  Kaustubh Deorukhkar   <kaustubh.deorukhkar@seagate.com>
 * Original creation date: 1-Oct-2015
 */

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

enum class S3ApiType {
  service,
  bucket,
  object,
  unsupported  // Invalid or Unsupported API
};

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
