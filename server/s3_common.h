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

#include <stdlib.h>
#include <string>

#ifdef __cplusplus
#define EXTERN_C_BLOCK_BEGIN extern "C" {
#define EXTERN_C_BLOCK_END }
#define EXTERN_C_FUNC extern "C"
#else
#define EXTERN_C_BLOCK_BEGIN
#define EXTERN_C_BLOCK_END
#define EXTERN_C_FUNC
#endif

#define MAX_COLLISION_TRY 20

#define ACCOUNT_USER_INDEX_NAME "ACCOUNTUSERINDEX"

// We reserve 255 oids after M0_CLOVIS_ID_APP for S3 servers internal use.
#define S3_OID_RESERVED_COUNT 255
// First OID after M0_CLOVIS_ID_APP is reserved for S3 root AccountUserIndex.
// see s3_user_account_index_metadata.h/cc
#define S3_ROOT_ACC_USER_IDX_OFFSET 1

enum class S3ApiType {
  service,
  bucket,
  object,
  faultinjection,  // Non S3 Apitype used internally for system tests
  unsupported      // Invalid or Unsupported API
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
  multidelete,
  multipart
};

enum class S3AsyncOpStatus {
  unknown,
  inprogress,
  success,
  failed,
  connection_failed
};

enum class S3IOOpStatus { saved, failed };

enum class S3ClovisEntityType { realm, object, index };

// returns 'true' if numeric value of string 'x' is less than that of 'y'
struct S3NumStrComparator {
  bool operator()(const std::string &x, const std::string &y) const {
    return strtoul(x.c_str(), NULL, 0) < strtoul(y.c_str(), NULL, 0);
  }
};

#endif
