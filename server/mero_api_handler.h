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

#ifndef __S3_SERVER_MERO_API_HANDLER_H__
#define __S3_SERVER_MERO_API_HANDLER_H__

#include <memory>

#include "gtest/gtest_prod.h"

#include "action_base.h"
#include "s3_common.h"
#include "mero_request_object.h"

class MeroAPIHandler {
 protected:
  std::shared_ptr<MeroRequestObject> request;
  std::shared_ptr<Action> action;

  MeroOperationCode operation_code;

  // Only for Unit testing
  std::shared_ptr<MeroAPIHandler> _get_self_ref() { return self_ref; }
  std::shared_ptr<Action> _get_action() { return action; }
  std::string request_id;

 private:
  std::shared_ptr<MeroAPIHandler> self_ref;

 public:
  MeroAPIHandler(std::shared_ptr<MeroRequestObject> req,
                 MeroOperationCode op_code)
      : request(req), operation_code(op_code) {
    request_id = request->get_request_id();
  }
  virtual ~MeroAPIHandler() {}

  virtual void create_action() = 0;

  virtual void dispatch() {
    s3_log(S3_LOG_DEBUG, request_id, "Entering");

    if (action) {
      action->manage_self(action);
      action->start();
    } else {
      request->respond_unsupported_api();
    }
    i_am_done();

    s3_log(S3_LOG_DEBUG, "", "Exiting");
  }

  // Self destructing object.
  void manage_self(std::shared_ptr<MeroAPIHandler> ref) { self_ref = ref; }
  // This *MUST* be the last thing on object. Called @ end of dispatch.
  void i_am_done() { self_ref.reset(); }

  FRIEND_TEST(MeroAPIHandlerTest, ConstructorTest);
  FRIEND_TEST(MeroAPIHandlerTest, ManageSelfAndReset);
  FRIEND_TEST(MeroAPIHandlerTest, DispatchActionTest);
  FRIEND_TEST(MeroAPIHandlerTest, DispatchUnSupportedAction);
};

class MeroIndexAPIHandler : public MeroAPIHandler {
 public:
  MeroIndexAPIHandler(std::shared_ptr<MeroRequestObject> req,
                      MeroOperationCode op_code)
      : MeroAPIHandler(req, op_code) {}

  virtual void create_action();

  FRIEND_TEST(MeroIndexAPIHandlerTest, ConstructorTest);
  FRIEND_TEST(MeroIndexAPIHandlerTest, ManageSelfAndReset);
  FRIEND_TEST(MeroIndexAPIHandlerTest, ShouldCreateS3GetServiceAction);
  FRIEND_TEST(MeroIndexAPIHandlerTest, OperationNoneDefaultNoAction);
};

class MeroKeyValueAPIHandler : public MeroAPIHandler {
 public:
  MeroKeyValueAPIHandler(std::shared_ptr<MeroRequestObject> req,
                         MeroOperationCode op_code)
      : MeroAPIHandler(req, op_code) {}

  virtual void create_action();

  // FRIEND_TEST(MeroKeyValueAPIHandlerTest, ConstructorTest);
  // FRIEND_TEST(MeroKeyValueAPIHandlerTest, ManageSelfAndReset);
  // FRIEND_TEST(MeroKeyValueAPIHandlerTest,
  // ShouldCreateS3GetBucketlocationAction);
  // FRIEND_TEST(MeroKeyValueAPIHandlerTest,
  // ShouldNotCreateS3PutBucketlocationAction);
  // FRIEND_TEST(MeroKeyValueAPIHandlerTest, ShouldNotHaveAction4OtherHttpOps);
  // FRIEND_TEST(MeroKeyValueAPIHandlerTest,
  //             ShouldCreateS3DeleteMultipleObjectsAction);
  // FRIEND_TEST(MeroKeyValueAPIHandlerTest, ShouldCreateS3GetBucketACLAction);
  // FRIEND_TEST(MeroKeyValueAPIHandlerTest, ShouldCreateS3PutBucketACLAction);
  // FRIEND_TEST(MeroKeyValueAPIHandlerTest,
  // ShouldCreateS3GetMultipartBucketAction);
  // FRIEND_TEST(MeroKeyValueAPIHandlerTest,
  // ShouldCreateS3GetBucketPolicyAction);
  // FRIEND_TEST(MeroKeyValueAPIHandlerTest,
  // ShouldCreateS3PutBucketPolicyAction);
  // FRIEND_TEST(MeroKeyValueAPIHandlerTest,
  // ShouldCreateS3DeleteBucketPolicyAction);
  // FRIEND_TEST(MeroKeyValueAPIHandlerTest, ShouldCreateS3HeadBucketAction);
  // FRIEND_TEST(MeroKeyValueAPIHandlerTest, ShouldCreateS3GetBucketAction);
  // FRIEND_TEST(MeroKeyValueAPIHandlerTest, ShouldCreateS3PutBucketAction);
  // FRIEND_TEST(MeroKeyValueAPIHandlerTest, ShouldCreateS3DeleteBucketAction);
  // FRIEND_TEST(MeroKeyValueAPIHandlerTest, OperationNoneDefaultNoAction);
};

class MeroObjectAPIHandler : public MeroAPIHandler {
 public:
  MeroObjectAPIHandler(std::shared_ptr<MeroRequestObject> req,
                       MeroOperationCode op_code)
      : MeroAPIHandler(req, op_code) {}

  virtual void create_action();

  // FRIEND_TEST(MeroObjectAPIHandlerTest, ConstructorTest);
  // FRIEND_TEST(MeroObjectAPIHandlerTest, ManageSelfAndReset);
  // FRIEND_TEST(MeroObjectAPIHandlerTest, ShouldCreateS3GetObjectACLAction);
  // FRIEND_TEST(MeroObjectAPIHandlerTest, ShouldCreateS3PutObjectACLAction);
  // FRIEND_TEST(MeroObjectAPIHandlerTest, ShouldNotHaveAction4OtherHttpOps);
  // FRIEND_TEST(MeroObjectAPIHandlerTest, ShouldCreateS3PostCompleteAction);
  // FRIEND_TEST(MeroObjectAPIHandlerTest,
  // ShouldCreateS3PostMultipartObjectAction);
  // FRIEND_TEST(MeroObjectAPIHandlerTest, DoesNotSupportCopyPart);
  // FRIEND_TEST(MeroObjectAPIHandlerTest, ShouldCreateS3PutMultiObjectAction);
  // FRIEND_TEST(MeroObjectAPIHandlerTest,
  // ShouldCreateS3GetMultipartPartAction);
  // FRIEND_TEST(MeroObjectAPIHandlerTest, ShouldCreateS3AbortMultipartAction);
  // FRIEND_TEST(MeroObjectAPIHandlerTest, ShouldCreateS3HeadObjectAction);
  // FRIEND_TEST(MeroObjectAPIHandlerTest,
  // ShouldCreateS3PutChunkUploadObjectAction);
  // FRIEND_TEST(MeroObjectAPIHandlerTest, DoesNotSupportCopyObject);
  // FRIEND_TEST(MeroObjectAPIHandlerTest, ShouldCreateS3PutObjectAction);
  // FRIEND_TEST(MeroObjectAPIHandlerTest, ShouldCreateS3GetObjectAction);
  // FRIEND_TEST(MeroObjectAPIHandlerTest, ShouldCreateS3DeleteObjectAction);
  // FRIEND_TEST(MeroObjectAPIHandlerTest, NoAction);
};

class MeroFaultinjectionAPIHandler : public MeroAPIHandler {
 public:
  MeroFaultinjectionAPIHandler(std::shared_ptr<MeroRequestObject> req,
                               MeroOperationCode op_code)
      : MeroAPIHandler(req, op_code) {}

  virtual void create_action();
};

class MeroAPIHandlerFactory {
 public:
  virtual ~MeroAPIHandlerFactory() {}

  virtual std::shared_ptr<MeroAPIHandler> create_api_handler(
      MeroApiType api_type, std::shared_ptr<MeroRequestObject> request,
      MeroOperationCode op_code);

  // FRIEND_TEST(MeroAPIHandlerFactoryTest, ShouldCreateMeroIndexAPIHandler);
  // FRIEND_TEST(MeroAPIHandlerFactoryTest, ShouldCreateMeroKeyValueAPIHandler);
  // FRIEND_TEST(MeroAPIHandlerFactoryTest, ShouldCreateS3ObjectAPIHandler);
  // FRIEND_TEST(MeroAPIHandlerFactoryTest,
  // ShouldCreateS3FaultinjectionAPIHandler);
};

#endif
