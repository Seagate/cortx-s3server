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

#ifndef __S3_UT_MOCK_MERO_API_HANDLER_H__
#define __S3_UT_MOCK_MERO_API_HANDLER_H__

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "mero_api_handler.h"

class MockMeroIndexAPIHandler : public MeroIndexAPIHandler {
 public:
  MockMeroIndexAPIHandler(std::shared_ptr<MeroRequestObject> req,
                          MeroOperationCode op_code)
      : MeroIndexAPIHandler(req, op_code) {}
  MOCK_METHOD0(dispatch, void());
};

class MockMeroKeyValueAPIHandler : public MeroKeyValueAPIHandler {
 public:
  MockMeroKeyValueAPIHandler(std::shared_ptr<MeroRequestObject> req,
                             MeroOperationCode op_code)
      : MeroKeyValueAPIHandler(req, op_code) {}
  MOCK_METHOD0(dispatch, void());
};

class MockMeroObjectAPIHandler : public MeroObjectAPIHandler {
 public:
  MockMeroObjectAPIHandler(std::shared_ptr<MeroRequestObject> req,
                           MeroOperationCode op_code)
      : MeroObjectAPIHandler(req, op_code) {}
  MOCK_METHOD0(dispatch, void());
};

class MockMeroAPIHandlerFactory : public MeroAPIHandlerFactory {
 public:
  MOCK_METHOD3(create_api_handler,
               std::shared_ptr<MeroAPIHandler>(
                   MeroApiType api_type,
                   std::shared_ptr<MeroRequestObject> request,
                   MeroOperationCode op_code));
};

#endif
