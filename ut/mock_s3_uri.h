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

#ifndef __S3_UT_MOCK_S3_URI_H__
#define __S3_UT_MOCK_S3_URI_H__

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "s3_uri.h"

class MockS3PathStyleURI : public S3PathStyleURI {
 public:
  MockS3PathStyleURI(std::shared_ptr<S3RequestObject> req)
      : S3PathStyleURI(req) {}
  MOCK_METHOD0(get_bucket_name, std::string&());
  MOCK_METHOD0(get_object_name, std::string&());
  MOCK_METHOD0(get_operation_code, S3OperationCode());
  MOCK_METHOD0(get_s3_api_type, S3ApiType());
};

class MockS3VirtualHostStyleURI : public S3VirtualHostStyleURI {
 public:
  MockS3VirtualHostStyleURI(std::shared_ptr<S3RequestObject> req)
      : S3VirtualHostStyleURI(req) {}
  MOCK_METHOD0(get_bucket_name, std::string&());
  MOCK_METHOD0(get_object_name, std::string&());
  MOCK_METHOD0(get_operation_code, S3OperationCode());
  MOCK_METHOD0(get_s3_api_type, S3ApiType());
};

class MockS3UriFactory : public S3UriFactory {
 public:
  MOCK_METHOD2(create_uri_object,
               S3URI*(S3UriType uri_type,
                      std::shared_ptr<S3RequestObject> request));
};
#endif
