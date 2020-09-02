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

#pragma once

#ifndef __S3_SERVER_S3_REQUEST_OBJECT_H__
#define __S3_SERVER_S3_REQUEST_OBJECT_H__

#include <map>
#include <memory>
#include <set>
#include <string>
#include <tuple>
#include <vector>

/* libevhtp */
#include <gtest/gtest_prod.h>
#include "evhtp_wrapper.h"
#include "event_wrapper.h"

#include "request_object.h"
#include "s3_async_buffer_opt.h"
#include "s3_chunk_payload_parser.h"
#include "s3_log.h"
#include "s3_option.h"
#include "s3_perf_logger.h"
#include "s3_timer.h"
#include "s3_uuid.h"
#include "s3_audit_info.h"

class S3AsyncBufferOptContainerFactory;

class S3RequestObject : public RequestObject {
  std::string bucket_name;
  std::string object_name;
  std::string default_acl;
  std::string s3_action;

  S3AuditInfo audit_log_obj;
  size_t object_size;

  S3ApiType s3_api_type;
  S3OperationCode s3_operation_code;

 public:
  S3RequestObject(
      evhtp_request_t* req, EvhtpInterface* evhtp_obj_ptr,
      std::shared_ptr<S3AsyncBufferOptContainerFactory> async_buf_factory =
          nullptr,
      EventInterface* event_obj_ptr = nullptr);
  virtual ~S3RequestObject();
  void set_api_type(S3ApiType apitype);
  virtual S3ApiType get_api_type();
  virtual size_t get_header_size() { return header_size; }
  virtual size_t get_user_metadata_size() { return user_metadata_size; }
  void set_operation_code(S3OperationCode operation_code);
  virtual S3OperationCode get_operation_code();
  virtual void populate_and_log_audit_info();

 public:
  virtual void set_object_size(size_t obj_size);
  // Operation params.
  std::string get_object_uri();
  std::string get_action_str();
  virtual S3AuditInfo& get_audit_info();

  virtual void set_bucket_name(const std::string& name);
  virtual const std::string& get_bucket_name();
  virtual void set_object_name(const std::string& name);
  virtual const std::string& get_object_name();
  virtual void set_action_str(const std::string& action);
  virtual void set_default_acl(const std::string& name);
  virtual const std::string& get_default_acl();

 public:

  FRIEND_TEST(S3MockAuthClientCheckTest, CheckAuth);
  FRIEND_TEST(S3RequestObjectTest, ReturnsValidUriPaths);
  FRIEND_TEST(S3RequestObjectTest, ReturnsValidRawQuery);
  FRIEND_TEST(S3PutBucketActionTest, ValidateBucketNameValidNameTest1);
  FRIEND_TEST(S3PutBucketActionTest, ValidateBucketNameValidNameTest2);
  FRIEND_TEST(S3PutBucketActionTest, ValidateBucketNameValidNameTest3);
  FRIEND_TEST(S3PutBucketActionTest, ValidateBucketNameValidNameTest4);
  FRIEND_TEST(S3PutBucketActionTest, ValidateBucketNameValidNameTest5);
  FRIEND_TEST(S3PutBucketActionTest, ValidateBucketNameValidNameTest6);
  FRIEND_TEST(S3PutBucketActionTest, ValidateBucketNameInvalidNameTest1);
  FRIEND_TEST(S3PutBucketActionTest, ValidateBucketNameInvalidNameTest2);
  FRIEND_TEST(S3PutBucketActionTest, ValidateBucketNameInvalidNameTest3);
  FRIEND_TEST(S3PutBucketActionTest, ValidateBucketNameInvalidNameTest4);
  FRIEND_TEST(S3PutBucketActionTest, ValidateBucketNameInvalidNameTest5);
  FRIEND_TEST(S3PutBucketActionTest, ValidateBucketNameInvalidNameTest6);
  FRIEND_TEST(S3PutBucketActionTest, ValidateBucketNameInvalidNameTest7);
  FRIEND_TEST(S3PutBucketActionTest, ValidateBucketNameInvalidNameTest8);
  FRIEND_TEST(S3PutBucketActionTest, ValidateBucketNameInvalidNameTest9);
  FRIEND_TEST(S3PutBucketActionTest, ValidateBucketNameInvalidNameTest10);
  FRIEND_TEST(S3PutBucketActionTest, ValidateBucketNameInvalidNameTest11);
  FRIEND_TEST(S3PutBucketActionTest, ValidateBucketNameInvalidNameTest12);
  FRIEND_TEST(S3PutBucketActionTest, ValidateBucketNameInvalidNameTest13);
  FRIEND_TEST(S3PutBucketActionTest, ValidateBucketNameInvalidNameTest14);
  FRIEND_TEST(S3PutBucketActionTest, ValidateBucketNameInvalidNameTest15);
  FRIEND_TEST(S3RequestObjectTest, SetStartClientRequestReadTimeout);
  FRIEND_TEST(S3RequestObjectTest, StopClientReadTimerNull);
  FRIEND_TEST(S3RequestObjectTest, StopClientReadTimer);
  FRIEND_TEST(S3RequestObjectTest, FreeReadTimer);
  FRIEND_TEST(S3RequestObjectTest, FreeReadTimerNull);
  FRIEND_TEST(S3RequestObjectTest, TriggerClientReadTimeoutNoCallback);
  FRIEND_TEST(S3RequestObjectTest, TriggerClientReadTimeout);
  FRIEND_TEST(ActionTest, ClientReadTimeoutSendResponseToClient);
};

#endif
