
/* AUTO-GENERATED!  DO NOT EDIT, CHANGES WILL BE LOST! */

/* generated by server/addb-codegen.py */

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

#include "s3_addb_map.h"

const uint64_t g_s3_to_addb_idx_func_name_map_size = 208;

const char* g_s3_to_addb_idx_func_name_map[] = {
    "Action::check_authentication",
    "ActionTest::func_callback_one",
    "ActionTest::func_callback_two",
    "MotrAPIHandlerTest::func_callback_one",
    "MotrAction::check_authorization",
    "MotrDeleteIndexAction::delete_index",
    "MotrDeleteIndexAction::send_response_to_s3_client",
    "MotrDeleteIndexAction::validate_request",
    "MotrDeleteIndexActionTest::func_callback_one",
    "MotrDeleteKeyValueAction::delete_key_value",
    "MotrDeleteKeyValueAction::send_response_to_s3_client",
    "MotrDeleteObjectAction::delete_object",
    "MotrDeleteObjectAction::send_response_to_s3_client",
    "MotrDeleteObjectAction::validate_request",
    "MotrGetKeyValueAction::fetch_key_value",
    "MotrGetKeyValueAction::send_response_to_s3_client",
    "MotrHeadIndexAction::check_index_exist",
    "MotrHeadIndexAction::send_response_to_s3_client",
    "MotrHeadIndexAction::validate_request",
    "MotrHeadIndexActionTest::func_callback_one",
    "MotrHeadObjectAction::check_object_exist",
    "MotrHeadObjectAction::send_response_to_s3_client",
    "MotrHeadObjectAction::validate_request",
    "MotrKVSListingAction::get_next_key_value",
    "MotrKVSListingAction::send_response_to_s3_client",
    "MotrKVSListingAction::validate_request",
    "MotrPutKeyValueAction::put_key_value",
    "MotrPutKeyValueAction::read_and_validate_key_value",
    "MotrPutKeyValueAction::send_response_to_s3_client",
    "MotrPutKeyValueActionTest::func_callback",
    "S3APIHandlerTest::func_callback_one",
    "S3AbortMultipartAction::add_object_oid_to_probable_dead_oid_list",
    "S3AbortMultipartAction::delete_multipart_metadata",
    "S3AbortMultipartAction::delete_object",
    "S3AbortMultipartAction::delete_part_index_with_parts",
    "S3AbortMultipartAction::get_multipart_metadata",
    "S3AbortMultipartAction::mark_oid_for_deletion",
    "S3AbortMultipartAction::remove_probable_record",
    "S3AbortMultipartAction::send_response_to_s3_client",
    "S3AbortMultipartActionTest::func_callback_one",
    "S3AccountDeleteMetadataAction::fetch_first_bucket_metadata",
    "S3AccountDeleteMetadataAction::send_response_to_s3_client",
    "S3AccountDeleteMetadataAction::validate_request",
    "S3AccountDeleteMetadataActionTest::func_callback_one",
    "S3Action::check_authorization",
    "S3Action::load_metadata",
    "S3Action::set_authorization_meta",
    "S3BucketActionTest::func_callback_one",
    "S3CopyObjectAction::check_source_bucket_authorization",
    "S3CopyObjectAction::copy_object",
    "S3CopyObjectAction::create_object",
    "S3CopyObjectAction::save_metadata",
    "S3CopyObjectAction::send_response_to_s3_client",
    "S3CopyObjectAction::set_source_bucket_authorization_metadata",
    "S3CopyObjectAction::validate_copyobject_request",
    "S3CopyObjectActionTest::func_callback_one",
    "S3DeleteBucketAction::delete_bucket",
    "S3DeleteBucketAction::delete_multipart_objects",
    "S3DeleteBucketAction::fetch_first_object_metadata",
    "S3DeleteBucketAction::fetch_multipart_objects",
    "S3DeleteBucketAction::remove_multipart_index",
    "S3DeleteBucketAction::remove_object_list_index",
    "S3DeleteBucketAction::remove_objects_version_list_index",
    "S3DeleteBucketAction::remove_part_indexes",
    "S3DeleteBucketAction::send_response_to_s3_client",
    "S3DeleteBucketActionTest::func_callback_one",
    "S3DeleteBucketPolicyAction::delete_bucket_policy",
    "S3DeleteBucketPolicyAction::send_response_to_s3_client",
    "S3DeleteBucketTaggingAction::delete_bucket_tags",
    "S3DeleteBucketTaggingAction::send_response_to_s3_client",
    "S3DeleteBucketTaggingActionTest::func_callback_one",
    "S3DeleteMultipleObjectsAction::fetch_objects_info",
    "S3DeleteMultipleObjectsAction::send_response_to_s3_client",
    "S3DeleteMultipleObjectsAction::validate_request",
    "S3DeleteMultipleObjectsActionTest::func_callback_one",
    "S3DeleteObjectAction::add_object_oid_to_probable_dead_oid_list",
    "S3DeleteObjectAction::delete_metadata",
    "S3DeleteObjectAction::delete_object",
    "S3DeleteObjectAction::mark_oid_for_deletion",
    "S3DeleteObjectAction::remove_probable_record",
    "S3DeleteObjectAction::send_response_to_s3_client",
    "S3DeleteObjectTaggingAction::delete_object_tags",
    "S3DeleteObjectTaggingAction::send_response_to_s3_client",
    "S3GetBucketACLAction::send_response_to_s3_client",
    "S3GetBucketAction::get_next_objects",
    "S3GetBucketAction::send_response_to_s3_client",
    "S3GetBucketAction::validate_request",
    "S3GetBucketPolicyAction::check_metadata_missing_status",
    "S3GetBucketPolicyAction::send_response_to_s3_client",
    "S3GetBucketTaggingAction::check_metadata_missing_status",
    "S3GetBucketTaggingAction::send_response_to_s3_client",
    "S3GetBucketlocationAction::fetch_bucket_info",
    "S3GetBucketlocationAction::send_response_to_s3_client",
    "S3GetMultipartBucketAction::get_next_objects",
    "S3GetMultipartBucketAction::send_response_to_s3_client",
    "S3GetMultipartPartAction::get_key_object",
    "S3GetMultipartPartAction::get_multipart_metadata",
    "S3GetMultipartPartAction::get_next_objects",
    "S3GetMultipartPartAction::send_response_to_s3_client",
    "S3GetMultipartPartActionTest::func_callback_one",
    "S3GetObjectACLAction::send_response_to_s3_client",
    "S3GetObjectAction::check_full_or_range_object_read",
    "S3GetObjectAction::read_object",
    "S3GetObjectAction::send_response_to_s3_client",
    "S3GetObjectAction::validate_object_info",
    "S3GetObjectActionTest::func_callback_one",
    "S3GetObjectTaggingAction::send_response_to_s3_client",
    "S3GetObjectTaggingActionTest::func_callback_one",
    "S3GetServiceAction::get_next_buckets",
    "S3GetServiceAction::initialization",
    "S3GetServiceAction::send_response_to_s3_client",
    "S3HeadBucketAction::send_response_to_s3_client",
    "S3HeadObjectAction::send_response_to_s3_client",
    "S3HeadServiceAction::send_response_to_s3_client",
    "S3ObjectActionTest::func_callback_one",
    "S3PostCompleteAction::add_object_oid_to_probable_dead_oid_list",
    "S3PostCompleteAction::delete_multipart_metadata",
    "S3PostCompleteAction::delete_new_object",
    "S3PostCompleteAction::delete_old_object",
    "S3PostCompleteAction::delete_part_list_index",
    "S3PostCompleteAction::fetch_multipart_info",
    "S3PostCompleteAction::get_next_parts_info",
    "S3PostCompleteAction::load_and_validate_request",
    "S3PostCompleteAction::mark_new_oid_for_deletion",
    "S3PostCompleteAction::mark_old_oid_for_deletion",
    "S3PostCompleteAction::remove_new_oid_probable_record",
    "S3PostCompleteAction::save_metadata",
    "S3PostCompleteAction::send_response_to_s3_client",
    "S3PostCompleteActionTest::func_callback_one",
    "S3PostMultipartObjectAction::check_bucket_object_state",
    "S3PostMultipartObjectAction::create_part_meta_index",
    "S3PostMultipartObjectAction::save_upload_metadata",
    "S3PostMultipartObjectAction::send_response_to_s3_client",
    "S3PostMultipartObjectAction::validate_x_amz_tagging_if_present",
    "S3PostMultipartObjectTest::func_callback_one",
    "S3PutBucketACLAction::send_response_to_s3_client",
    "S3PutBucketACLAction::setacl",
    "S3PutBucketACLAction::validate_acl_with_auth",
    "S3PutBucketACLAction::validate_request",
    "S3PutBucketAclActionTest::func_callback_one",
    "S3PutBucketAction::create_bucket",
    "S3PutBucketAction::read_metadata",
    "S3PutBucketAction::send_response_to_s3_client",
    "S3PutBucketAction::validate_bucket_name",
    "S3PutBucketAction::validate_request",
    "S3PutBucketActionTest::func_callback_one",
    "S3PutBucketPolicyAction::send_response_to_s3_client",
    "S3PutBucketPolicyAction::set_policy",
    "S3PutBucketPolicyAction::validate_policy",
    "S3PutBucketPolicyAction::validate_request",
    "S3PutBucketTaggingAction::save_tags_to_bucket_metadata",
    "S3PutBucketTaggingAction::send_response_to_s3_client",
    "S3PutBucketTaggingAction::validate_request",
    "S3PutBucketTaggingAction::validate_request_xml_tags",
    "S3PutBucketTaggingActionTest::func_callback_one",
    "S3PutChunkUploadObjectAction::create_object",
    "S3PutChunkUploadObjectAction::delete_new_object",
    "S3PutChunkUploadObjectAction::delete_old_object",
    "S3PutChunkUploadObjectAction::initiate_data_streaming",
    "S3PutChunkUploadObjectAction::mark_new_oid_for_deletion",
    "S3PutChunkUploadObjectAction::mark_old_oid_for_deletion",
    "S3PutChunkUploadObjectAction::remove_new_oid_probable_record",
    "S3PutChunkUploadObjectAction::remove_old_oid_probable_record",
    "S3PutChunkUploadObjectAction::save_metadata",
    "S3PutChunkUploadObjectAction::send_response_to_s3_client",
    "S3PutChunkUploadObjectAction::validate_put_chunk_request",
    "S3PutChunkUploadObjectAction::validate_x_amz_tagging_if_present",
    "S3PutChunkUploadObjectActionTestBase::func_callback_one",
    "S3PutFiAction::send_response_to_s3_client",
    "S3PutFiAction::set_fault_injection",
    "S3PutMultiObjectAction::check_part_details",
    "S3PutMultiObjectAction::compute_part_offset",
    "S3PutMultiObjectAction::fetch_firstpart_info",
    "S3PutMultiObjectAction::fetch_multipart_metadata",
    "S3PutMultiObjectAction::initiate_data_streaming",
    "S3PutMultiObjectAction::save_metadata",
    "S3PutMultiObjectAction::save_multipart_metadata",
    "S3PutMultiObjectAction::send_response_to_s3_client",
    "S3PutMultiObjectAction::validate_multipart_request",
    "S3PutMultipartObjectActionTest::func_callback_one",
    "S3PutObjectACLAction::send_response_to_s3_client",
    "S3PutObjectACLAction::setacl",
    "S3PutObjectACLAction::validate_acl_with_auth",
    "S3PutObjectACLAction::validate_request",
    "S3PutObjectAction::create_object",
    "S3PutObjectAction::delete_new_object",
    "S3PutObjectAction::delete_old_object",
    "S3PutObjectAction::initiate_data_streaming",
    "S3PutObjectAction::mark_new_oid_for_deletion",
    "S3PutObjectAction::mark_old_oid_for_deletion",
    "S3PutObjectAction::remove_new_oid_probable_record",
    "S3PutObjectAction::remove_old_oid_probable_record",
    "S3PutObjectAction::save_metadata",
    "S3PutObjectAction::send_response_to_s3_client",
    "S3PutObjectAction::validate_put_request",
    "S3PutObjectAction::validate_x_amz_tagging_if_present",
    "S3PutObjectActionBase::delete_new_object",
    "S3PutObjectActionBase::delete_old_object",
    "S3PutObjectActionBase::mark_new_oid_for_deletion",
    "S3PutObjectActionBase::mark_old_oid_for_deletion",
    "S3PutObjectActionBase::remove_new_oid_probable_record",
    "S3PutObjectActionBase::remove_old_oid_probable_record",
    "S3PutObjectActionTest::func_callback_one",
    "S3PutObjectTaggingAction::save_tags_to_object_metadata",
    "S3PutObjectTaggingAction::send_response_to_s3_client",
    "S3PutObjectTaggingAction::validate_request",
    "S3PutObjectTaggingAction::validate_request_xml_tags",
    "S3PutObjectTaggingActionTest::func_callback_one"};
