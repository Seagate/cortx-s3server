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

#ifndef __S3_UT_MOCK_S3_PUT_TAG_BODY_H__
#define __S3_UT_MOCK_S3_PUT_TAG_BODY_H__

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "s3_put_tag_body.h"

using ::testing::_;
using ::testing::Return;

class MockS3PutTagBody : public S3PutTagBody {
 public:
  MockS3PutTagBody(std::string& xml, std::string& request_id)
      : S3PutTagBody(xml, request_id) {}
  MOCK_METHOD0(isOK, bool());
  MOCK_METHOD0(get_resource_tags_as_map, std::map<std::string, std::string>&());
  MOCK_METHOD1(validate_bucket_xml_tags,
               bool(std::map<std::string, std::string>& tags_str));
};

#endif
