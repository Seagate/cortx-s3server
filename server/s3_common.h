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

#ifndef __S3_SERVER_S3_COMMON_H__
#define __S3_SERVER_S3_COMMON_H__

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

#define MAX_COLLISION_RETRY_COUNT 20

#define ACCOUNT_USER_INDEX_NAME "ACCOUNTUSERINDEX"

// We reserve 255 oids after M0_CLOVIS_ID_APP for S3 servers internal use.
#define S3_OID_RESERVED_COUNT 255
// First OID after M0_CLOVIS_ID_APP is reserved for S3 root AccountUserIndex.
// see s3_user_account_index_metadata.h/cc
#define S3_ROOT_ACC_USER_IDX_OFFSET 1

// Part sizes for multipart operation
#define MINIMUM_ALLOWED_PART_SIZE 5242880

enum class S3ApiType {
  service,
  bucket,
  object,
  management,      // Currently only used by Auth Server
  faultinjection,  // Non S3 Apitype used internally for system tests
  unsupported      // Invalid or Unsupported API
};

enum class S3OperationCode {
  none,  // Operation on current object.

  // Common Operations
  acl,

  // Bucket Operations
  encryption,
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
  analytics,
  inventory,
  metrics,
  replication,
  accelerate,
  versions,

  // Object Operations
  initupload,
  partupload,
  completeupload,
  abortupload,
  multidelete,
  multipart,
  torrent,
  selectcontent,
  restore
};

inline std::string operation_code_to_str(S3OperationCode code) {
  switch (code) {
    case S3OperationCode::none:
      return "S3OperationCode::none";
    case S3OperationCode::acl:
      return "S3OperationCode::acl";
    case S3OperationCode::encryption:
      return "S3OperationCode::encryption";
    case S3OperationCode::policy:
      return "S3OperationCode::policy";
    case S3OperationCode::location:
      return "S3OperationCode::location";
    case S3OperationCode::multipart:
      return "S3OperationCode::multipart";
    case S3OperationCode::multidelete:
      return "S3OperationCode::multidelete";
    case S3OperationCode::requestPayment:
      return "S3OperationCode::requestPayment";
    case S3OperationCode::lifecycle:
      return "S3OperationCode::lifecycle";
    case S3OperationCode::cors:
      return "S3OperationCode::cors";
    case S3OperationCode::analytics:
      return "S3OperationCode::analytics";
    case S3OperationCode::inventory:
      return "S3OperationCode::inventory";
    case S3OperationCode::metrics:
      return "S3OperationCode::metrics";
    case S3OperationCode::tagging:
      return "S3OperationCode::tagging";
    case S3OperationCode::website:
      return "S3OperationCode::website";
    case S3OperationCode::replication:
      return "S3OperationCode::replication";
    case S3OperationCode::accelerate:
      return "S3OperationCode::accelerate";
    case S3OperationCode::logging:
      return "S3OperationCode::logging";
    case S3OperationCode::notification:
      return "S3OperationCode::notification";
    case S3OperationCode::torrent:
      return "S3OperationCode::torrent";
    case S3OperationCode::versioning:
      return "S3OperationCode::versioning";
    case S3OperationCode::versions:
      return "S3OperationCode::versions";
    case S3OperationCode::selectcontent:
      return "S3OperationCode::selectcontent";
    case S3OperationCode::restore:
      return "S3OperationCode::restore";
    default:
      return "S3OperationCode::Unknown";
  }
}

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
