/*
 * Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For any questions about this software or licensing,
 * please email opensource@seagate.com or cortx-questions@seagate.com.
 *
 */


 * http://www.seagate.com/contact
 *
 * Original author:  Ivan Tishchenko   <ivan.tishchenko@seagate.com>
 * Original creation date: 28-Oct-2019
 */

#include <typeinfo>
#include <typeindex>
#include <unordered_map>

#include "s3_addb.h"

// Main goal of the functions below is to support run-time identification of
// S3AddbActionTypeId for a given Action class.  This is done using C++ RTTI.
// We're going to store a map of type_index(typeid(class)) to an ID.  Init
// function initializes that map, lookup function searches through it.

// Include all action classes' headers:
#include "mero_delete_index_action.h"
#include "mero_delete_key_value_action.h"
#include "mero_delete_object_action.h"
#include "mero_get_key_value_action.h"
#include "mero_head_index_action.h"
#include "mero_head_object_action.h"
#include "mero_kvs_listing_action.h"
#include "mero_put_key_value_action.h"
#include "s3_abort_multipart_action.h"
#include "s3_account_delete_metadata_action.h"
#include "s3_delete_bucket_action.h"
#include "s3_delete_bucket_policy_action.h"
#include "s3_delete_bucket_tagging_action.h"
#include "s3_delete_multiple_objects_action.h"
#include "s3_delete_object_action.h"
#include "s3_delete_object_tagging_action.h"
#include "s3_get_bucket_acl_action.h"
#include "s3_get_bucket_action.h"
#include "s3_get_bucket_location_action.h"
#include "s3_get_bucket_policy_action.h"
#include "s3_get_bucket_tagging_action.h"
#include "s3_get_multipart_bucket_action.h"
#include "s3_get_multipart_part_action.h"
#include "s3_get_object_acl_action.h"
#include "s3_get_object_action.h"
#include "s3_get_object_tagging_action.h"
#include "s3_get_service_action.h"
#include "s3_head_bucket_action.h"
#include "s3_head_object_action.h"
#include "s3_head_service_action.h"
#include "s3_post_complete_action.h"
#include "s3_post_multipartobject_action.h"
#include "s3_put_bucket_acl_action.h"
#include "s3_put_bucket_action.h"
#include "s3_put_bucket_policy_action.h"
#include "s3_put_bucket_tagging_action.h"
#include "s3_put_chunk_upload_object_action.h"
#include "s3_put_fi_action.h"
#include "s3_put_multiobject_action.h"
#include "s3_put_object_acl_action.h"
#include "s3_put_object_action.h"
#include "s3_put_object_tagging_action.h"

static std::unordered_map<std::type_index, enum S3AddbActionTypeId> gs_addb_map;

int s3_addb_init() {
  s3_log(S3_LOG_DEBUG, "", "Entering\n");
  // Sorry for the format, this had to be done this way to pass
  // git-clang-format check.
  gs_addb_map[std::type_index(typeid(MeroDeleteIndexAction))] =
      S3_ADDB_MERO_DELETE_INDEX_ACTION_ID;
  gs_addb_map[std::type_index(typeid(MeroDeleteKeyValueAction))] =
      S3_ADDB_MERO_DELETE_KEY_VALUE_ACTION_ID;
  gs_addb_map[std::type_index(typeid(MeroDeleteObjectAction))] =
      S3_ADDB_MERO_DELETE_OBJECT_ACTION_ID;
  gs_addb_map[std::type_index(typeid(MeroGetKeyValueAction))] =
      S3_ADDB_MERO_GET_KEY_VALUE_ACTION_ID;
  gs_addb_map[std::type_index(typeid(MeroHeadIndexAction))] =
      S3_ADDB_MERO_HEAD_INDEX_ACTION_ID;
  gs_addb_map[std::type_index(typeid(MeroHeadObjectAction))] =
      S3_ADDB_MERO_HEAD_OBJECT_ACTION_ID;
  gs_addb_map[std::type_index(typeid(MeroKVSListingAction))] =
      S3_ADDB_MERO_KVS_LISTING_ACTION_ID;
  gs_addb_map[std::type_index(typeid(MeroPutKeyValueAction))] =
      S3_ADDB_MERO_PUT_KEY_VALUE_ACTION_ID;
  gs_addb_map[std::type_index(typeid(S3AbortMultipartAction))] =
      S3_ADDB_S3_ABORT_MULTIPART_ACTION_ID;
  gs_addb_map[std::type_index(typeid(S3AccountDeleteMetadataAction))] =
      S3_ADDB_S3_ACCOUNT_DELETE_METADATA_ACTION_ID;
  gs_addb_map[std::type_index(typeid(S3DeleteBucketAction))] =
      S3_ADDB_S3_DELETE_BUCKET_ACTION_ID;
  gs_addb_map[std::type_index(typeid(S3DeleteBucketPolicyAction))] =
      S3_ADDB_S3_DELETE_BUCKET_POLICY_ACTION_ID;
  gs_addb_map[std::type_index(typeid(S3DeleteBucketTaggingAction))] =
      S3_ADDB_S3_DELETE_BUCKET_TAGGING_ACTION_ID;
  gs_addb_map[std::type_index(typeid(S3DeleteMultipleObjectsAction))] =
      S3_ADDB_S3_DELETE_MULTIPLE_OBJECTS_ACTION_ID;
  gs_addb_map[std::type_index(typeid(S3DeleteObjectAction))] =
      S3_ADDB_S3_DELETE_OBJECT_ACTION_ID;
  gs_addb_map[std::type_index(typeid(S3DeleteObjectTaggingAction))] =
      S3_ADDB_S3_DELETE_OBJECT_TAGGING_ACTION_ID;
  gs_addb_map[std::type_index(typeid(S3GetBucketACLAction))] =
      S3_ADDB_S3_GET_BUCKET_ACL_ACTION_ID;
  gs_addb_map[std::type_index(typeid(S3GetBucketAction))] =
      S3_ADDB_S3_GET_BUCKET_ACTION_ID;
  gs_addb_map[std::type_index(typeid(S3GetBucketPolicyAction))] =
      S3_ADDB_S3_GET_BUCKET_POLICY_ACTION_ID;
  gs_addb_map[std::type_index(typeid(S3GetBucketTaggingAction))] =
      S3_ADDB_S3_GET_BUCKET_TAGGING_ACTION_ID;
  gs_addb_map[std::type_index(typeid(S3GetBucketlocationAction))] =
      S3_ADDB_S3_GET_BUCKETLOCATION_ACTION_ID;
  gs_addb_map[std::type_index(typeid(S3GetMultipartBucketAction))] =
      S3_ADDB_S3_GET_MULTIPART_BUCKET_ACTION_ID;
  gs_addb_map[std::type_index(typeid(S3GetMultipartPartAction))] =
      S3_ADDB_S3_GET_MULTIPART_PART_ACTION_ID;
  gs_addb_map[std::type_index(typeid(S3GetObjectACLAction))] =
      S3_ADDB_S3_GET_OBJECT_ACL_ACTION_ID;
  gs_addb_map[std::type_index(typeid(S3GetObjectAction))] =
      S3_ADDB_S3_GET_OBJECT_ACTION_ID;
  gs_addb_map[std::type_index(typeid(S3GetObjectTaggingAction))] =
      S3_ADDB_S3_GET_OBJECT_TAGGING_ACTION_ID;
  gs_addb_map[std::type_index(typeid(S3GetServiceAction))] =
      S3_ADDB_S3_GET_SERVICE_ACTION_ID;
  gs_addb_map[std::type_index(typeid(S3HeadBucketAction))] =
      S3_ADDB_S3_HEAD_BUCKET_ACTION_ID;
  gs_addb_map[std::type_index(typeid(S3HeadObjectAction))] =
      S3_ADDB_S3_HEAD_OBJECT_ACTION_ID;
  gs_addb_map[std::type_index(typeid(S3HeadServiceAction))] =
      S3_ADDB_S3_HEAD_SERVICE_ACTION_ID;
  gs_addb_map[std::type_index(typeid(S3PostCompleteAction))] =
      S3_ADDB_S3_POST_COMPLETE_ACTION_ID;
  gs_addb_map[std::type_index(typeid(S3PostMultipartObjectAction))] =
      S3_ADDB_S3_POST_MULTIPART_OBJECT_ACTION_ID;
  gs_addb_map[std::type_index(typeid(S3PutBucketACLAction))] =
      S3_ADDB_S3_PUT_BUCKET_ACL_ACTION_ID;
  gs_addb_map[std::type_index(typeid(S3PutBucketAction))] =
      S3_ADDB_S3_PUT_BUCKET_ACTION_ID;
  gs_addb_map[std::type_index(typeid(S3PutBucketPolicyAction))] =
      S3_ADDB_S3_PUT_BUCKET_POLICY_ACTION_ID;
  gs_addb_map[std::type_index(typeid(S3PutBucketTaggingAction))] =
      S3_ADDB_S3_PUT_BUCKET_TAGGING_ACTION_ID;
  gs_addb_map[std::type_index(typeid(S3PutChunkUploadObjectAction))] =
      S3_ADDB_S3_PUT_CHUNK_UPLOAD_OBJECT_ACTION_ID;
  gs_addb_map[std::type_index(typeid(S3PutFiAction))] =
      S3_ADDB_S3_PUT_FI_ACTION_ID;
  gs_addb_map[std::type_index(typeid(S3PutMultiObjectAction))] =
      S3_ADDB_S3_PUT_MULTI_OBJECT_ACTION_ID;
  gs_addb_map[std::type_index(typeid(S3PutObjectACLAction))] =
      S3_ADDB_S3_PUT_OBJECT_ACL_ACTION_ID;
  gs_addb_map[std::type_index(typeid(S3PutObjectAction))] =
      S3_ADDB_S3_PUT_OBJECT_ACTION_ID;
  gs_addb_map[std::type_index(typeid(S3PutObjectTaggingAction))] =
      S3_ADDB_S3_PUT_OBJECT_TAGGING_ACTION_ID;

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class MeroDeleteIndexAction\n",
         (uint64_t)S3_ADDB_MERO_DELETE_INDEX_ACTION_ID,
         (int64_t)S3_ADDB_MERO_DELETE_INDEX_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class MeroDeleteKeyValueAction\n",
         (uint64_t)S3_ADDB_MERO_DELETE_KEY_VALUE_ACTION_ID,
         (int64_t)S3_ADDB_MERO_DELETE_KEY_VALUE_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class MeroDeleteObjectAction\n",
         (uint64_t)S3_ADDB_MERO_DELETE_OBJECT_ACTION_ID,
         (int64_t)S3_ADDB_MERO_DELETE_OBJECT_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class MeroGetKeyValueAction\n",
         (uint64_t)S3_ADDB_MERO_GET_KEY_VALUE_ACTION_ID,
         (int64_t)S3_ADDB_MERO_GET_KEY_VALUE_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class MeroHeadIndexAction\n",
         (uint64_t)S3_ADDB_MERO_HEAD_INDEX_ACTION_ID,
         (int64_t)S3_ADDB_MERO_HEAD_INDEX_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class MeroHeadObjectAction\n",
         (uint64_t)S3_ADDB_MERO_HEAD_OBJECT_ACTION_ID,
         (int64_t)S3_ADDB_MERO_HEAD_OBJECT_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class MeroKVSListingAction\n",
         (uint64_t)S3_ADDB_MERO_KVS_LISTING_ACTION_ID,
         (int64_t)S3_ADDB_MERO_KVS_LISTING_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class MeroPutKeyValueAction\n",
         (uint64_t)S3_ADDB_MERO_PUT_KEY_VALUE_ACTION_ID,
         (int64_t)S3_ADDB_MERO_PUT_KEY_VALUE_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class S3AbortMultipartAction\n",
         (uint64_t)S3_ADDB_S3_ABORT_MULTIPART_ACTION_ID,
         (int64_t)S3_ADDB_S3_ABORT_MULTIPART_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class S3AccountDeleteMetadataAction\n",
         (uint64_t)S3_ADDB_S3_ACCOUNT_DELETE_METADATA_ACTION_ID,
         (int64_t)S3_ADDB_S3_ACCOUNT_DELETE_METADATA_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class S3DeleteBucketAction\n",
         (uint64_t)S3_ADDB_S3_DELETE_BUCKET_ACTION_ID,
         (int64_t)S3_ADDB_S3_DELETE_BUCKET_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class S3DeleteBucketPolicyAction\n",
         (uint64_t)S3_ADDB_S3_DELETE_BUCKET_POLICY_ACTION_ID,
         (int64_t)S3_ADDB_S3_DELETE_BUCKET_POLICY_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class S3DeleteBucketTaggingAction\n",
         (uint64_t)S3_ADDB_S3_DELETE_BUCKET_TAGGING_ACTION_ID,
         (int64_t)S3_ADDB_S3_DELETE_BUCKET_TAGGING_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class S3DeleteMultipleObjectsAction\n",
         (uint64_t)S3_ADDB_S3_DELETE_MULTIPLE_OBJECTS_ACTION_ID,
         (int64_t)S3_ADDB_S3_DELETE_MULTIPLE_OBJECTS_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class S3DeleteObjectAction\n",
         (uint64_t)S3_ADDB_S3_DELETE_OBJECT_ACTION_ID,
         (int64_t)S3_ADDB_S3_DELETE_OBJECT_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class S3DeleteObjectTaggingAction\n",
         (uint64_t)S3_ADDB_S3_DELETE_OBJECT_TAGGING_ACTION_ID,
         (int64_t)S3_ADDB_S3_DELETE_OBJECT_TAGGING_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class S3GetBucketACLAction\n",
         (uint64_t)S3_ADDB_S3_GET_BUCKET_ACL_ACTION_ID,
         (int64_t)S3_ADDB_S3_GET_BUCKET_ACL_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class S3GetBucketAction\n",
         (uint64_t)S3_ADDB_S3_GET_BUCKET_ACTION_ID,
         (int64_t)S3_ADDB_S3_GET_BUCKET_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class S3GetBucketPolicyAction\n",
         (uint64_t)S3_ADDB_S3_GET_BUCKET_POLICY_ACTION_ID,
         (int64_t)S3_ADDB_S3_GET_BUCKET_POLICY_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class S3GetBucketTaggingAction\n",
         (uint64_t)S3_ADDB_S3_GET_BUCKET_TAGGING_ACTION_ID,
         (int64_t)S3_ADDB_S3_GET_BUCKET_TAGGING_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class S3GetBucketlocationAction\n",
         (uint64_t)S3_ADDB_S3_GET_BUCKETLOCATION_ACTION_ID,
         (int64_t)S3_ADDB_S3_GET_BUCKETLOCATION_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class S3GetMultipartBucketAction\n",
         (uint64_t)S3_ADDB_S3_GET_MULTIPART_BUCKET_ACTION_ID,
         (int64_t)S3_ADDB_S3_GET_MULTIPART_BUCKET_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class S3GetMultipartPartAction\n",
         (uint64_t)S3_ADDB_S3_GET_MULTIPART_PART_ACTION_ID,
         (int64_t)S3_ADDB_S3_GET_MULTIPART_PART_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class S3GetObjectACLAction\n",
         (uint64_t)S3_ADDB_S3_GET_OBJECT_ACL_ACTION_ID,
         (int64_t)S3_ADDB_S3_GET_OBJECT_ACL_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class S3GetObjectAction\n",
         (uint64_t)S3_ADDB_S3_GET_OBJECT_ACTION_ID,
         (int64_t)S3_ADDB_S3_GET_OBJECT_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class S3GetObjectTaggingAction\n",
         (uint64_t)S3_ADDB_S3_GET_OBJECT_TAGGING_ACTION_ID,
         (int64_t)S3_ADDB_S3_GET_OBJECT_TAGGING_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class S3GetServiceAction\n",
         (uint64_t)S3_ADDB_S3_GET_SERVICE_ACTION_ID,
         (int64_t)S3_ADDB_S3_GET_SERVICE_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class S3HeadBucketAction\n",
         (uint64_t)S3_ADDB_S3_HEAD_BUCKET_ACTION_ID,
         (int64_t)S3_ADDB_S3_HEAD_BUCKET_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class S3HeadObjectAction\n",
         (uint64_t)S3_ADDB_S3_HEAD_OBJECT_ACTION_ID,
         (int64_t)S3_ADDB_S3_HEAD_OBJECT_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class S3HeadServiceAction\n",
         (uint64_t)S3_ADDB_S3_HEAD_SERVICE_ACTION_ID,
         (int64_t)S3_ADDB_S3_HEAD_SERVICE_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class S3PostCompleteAction\n",
         (uint64_t)S3_ADDB_S3_POST_COMPLETE_ACTION_ID,
         (int64_t)S3_ADDB_S3_POST_COMPLETE_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class S3PostMultipartObjectAction\n",
         (uint64_t)S3_ADDB_S3_POST_MULTIPART_OBJECT_ACTION_ID,
         (int64_t)S3_ADDB_S3_POST_MULTIPART_OBJECT_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class S3PutBucketACLAction\n",
         (uint64_t)S3_ADDB_S3_PUT_BUCKET_ACL_ACTION_ID,
         (int64_t)S3_ADDB_S3_PUT_BUCKET_ACL_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class S3PutBucketAction\n",
         (uint64_t)S3_ADDB_S3_PUT_BUCKET_ACTION_ID,
         (int64_t)S3_ADDB_S3_PUT_BUCKET_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class S3PutBucketPolicyAction\n",
         (uint64_t)S3_ADDB_S3_PUT_BUCKET_POLICY_ACTION_ID,
         (int64_t)S3_ADDB_S3_PUT_BUCKET_POLICY_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class S3PutBucketTaggingAction\n",
         (uint64_t)S3_ADDB_S3_PUT_BUCKET_TAGGING_ACTION_ID,
         (int64_t)S3_ADDB_S3_PUT_BUCKET_TAGGING_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class S3PutChunkUploadObjectAction\n",
         (uint64_t)S3_ADDB_S3_PUT_CHUNK_UPLOAD_OBJECT_ACTION_ID,
         (int64_t)S3_ADDB_S3_PUT_CHUNK_UPLOAD_OBJECT_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class S3PutFiAction\n",
         (uint64_t)S3_ADDB_S3_PUT_FI_ACTION_ID,
         (int64_t)S3_ADDB_S3_PUT_FI_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class S3PutMultiObjectAction\n",
         (uint64_t)S3_ADDB_S3_PUT_MULTI_OBJECT_ACTION_ID,
         (int64_t)S3_ADDB_S3_PUT_MULTI_OBJECT_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class S3PutObjectACLAction\n",
         (uint64_t)S3_ADDB_S3_PUT_OBJECT_ACL_ACTION_ID,
         (int64_t)S3_ADDB_S3_PUT_OBJECT_ACL_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class S3PutObjectAction\n",
         (uint64_t)S3_ADDB_S3_PUT_OBJECT_ACTION_ID,
         (int64_t)S3_ADDB_S3_PUT_OBJECT_ACTION_ID);

  s3_log(S3_LOG_DEBUG, "",
         "  * id 0x%" PRIx64 "/%" PRId64  // suppress clang warning
         ": class S3PutObjectTaggingAction\n",
         (uint64_t)S3_ADDB_S3_PUT_OBJECT_TAGGING_ACTION_ID,
         (int64_t)S3_ADDB_S3_PUT_OBJECT_TAGGING_ACTION_ID);
  return 0;
}

enum S3AddbActionTypeId Action::lookup_addb_action_type_id(
    const Action& instance) {
  enum S3AddbActionTypeId action_type_id =
      gs_addb_map[std::type_index(typeid(instance))];
  return (action_type_id != 0 ? action_type_id : S3_ADDB_ACTION_BASE_ID);
}
