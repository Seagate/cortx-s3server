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

#ifndef __S3_UT_MOCK_MOTR_URI_H__
#define __S3_UT_MOCK_MOTR_URI_H__

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "motr_uri.h"

class MockMotrPathStyleURI : public MotrPathStyleURI {
 public:
  MockMotrPathStyleURI(std::shared_ptr<MotrRequestObject> req)
      : MotrPathStyleURI(req) {}
  MOCK_METHOD0(get_key_name, std::string&());
  MOCK_METHOD0(get_object_oid_lo, std::string&());
  MOCK_METHOD0(get_object_oid_ho, std::string&());
  MOCK_METHOD0(get_index_id_lo, std::string&());
  MOCK_METHOD0(get_index_id_hi, std::string&());
  MOCK_METHOD0(get_operation_code, MotrOperationCode());
  MOCK_METHOD0(get_motr_api_type, MotrApiType());
};

class MockMotrUriFactory : public MotrUriFactory {
 public:
  MOCK_METHOD2(create_uri_object,
               MotrURI*(MotrUriType uri_type,
                        std::shared_ptr<MotrRequestObject> request));
};
#endif
