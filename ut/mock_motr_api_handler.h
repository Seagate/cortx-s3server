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

#ifndef __S3_UT_MOCK_MOTR_API_HANDLER_H__
#define __S3_UT_MOCK_MOTR_API_HANDLER_H__

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "motr_api_handler.h"

class MockMotrIndexAPIHandler : public MotrIndexAPIHandler {
 public:
  MockMotrIndexAPIHandler(std::shared_ptr<MotrRequestObject> req,
                          MotrOperationCode op_code)
      : MotrIndexAPIHandler(req, op_code) {}
  MOCK_METHOD0(dispatch, void());
};

class MockMotrKeyValueAPIHandler : public MotrKeyValueAPIHandler {
 public:
  MockMotrKeyValueAPIHandler(std::shared_ptr<MotrRequestObject> req,
                             MotrOperationCode op_code)
      : MotrKeyValueAPIHandler(req, op_code) {}
  MOCK_METHOD0(dispatch, void());
};

class MockMotrObjectAPIHandler : public MotrObjectAPIHandler {
 public:
  MockMotrObjectAPIHandler(std::shared_ptr<MotrRequestObject> req,
                           MotrOperationCode op_code)
      : MotrObjectAPIHandler(req, op_code) {}
  MOCK_METHOD0(dispatch, void());
};

class MockMotrAPIHandlerFactory : public MotrAPIHandlerFactory {
 public:
  MOCK_METHOD3(create_api_handler,
               std::shared_ptr<MotrAPIHandler>(
                   MotrApiType api_type,
                   std::shared_ptr<MotrRequestObject> request,
                   MotrOperationCode op_code));
};

#endif
