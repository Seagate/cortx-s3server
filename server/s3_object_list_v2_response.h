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

#ifndef __S3_SERVER_S3_OBJECT_LIST_V2_RESPONSE_H__
#define __S3_SERVER_S3_OBJECT_LIST_V2_RESPONSE_H__

#include <gtest/gtest_prod.h>
#include <memory>
#include <string>
#include "s3_object_list_response.h"

class S3ObjectListResponseV2 : public S3ObjectListResponse {
  std::string continuation_token;
  bool fetch_owner;
  std::string start_after;
  std::string response_v2_xml;
  std::string next_continuation_token;

 public:
  S3ObjectListResponseV2(const std::string &encoding_type = "");

  void set_continuation_token(const std::string &token);
  void set_fetch_owner(bool &need_owner_info);
  void set_start_after(const std::string &in_start_after);
  std::string &get_start_after() { return start_after; }
  std::string &get_continuation_token() { return continuation_token; }

  // Overridden for ListObjects V2
  std::string &get_xml(const std::string &requestor_canonical_id,
                       const std::string &bucket_owner_user_id,
                       const std::string &requestor_user_id);

  // Google tests.
  FRIEND_TEST(S3ObjectListResponseV2Test, ConstructorTest);
  FRIEND_TEST(S3ObjectListResponseV2Test, VerifyObjectListV2Setters);
  FRIEND_TEST(S3ObjectListResponseV2Test, getXML_V2);
};

#endif
