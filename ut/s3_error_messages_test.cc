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

#include <json/json.h>
#include <fstream>
#include <iostream>

#include <gtest/gtest.h>
#include "s3_error_messages.h"
/* /opt/seagate/cortx/s3/resources/s3_error_messages.json */

TEST(S3ErrorDetailsTest, DefaultConstructor) {
  S3ErrorDetails error_msg;
  EXPECT_EQ(520, error_msg.http_return_code);
  EXPECT_STREQ("Unknown Error", error_msg.description.c_str());
}

TEST(S3ErrorDetailsTest, ConstructorWithMsg) {
  S3ErrorDetails error_msg("message", 1);
  EXPECT_EQ(1, error_msg.http_return_code);
  EXPECT_STREQ("message", error_msg.description.c_str());
}

TEST(S3ErrorDetailsTest, Get) {
  S3ErrorDetails error_msg("message2", 500);
  std::string desc = error_msg.get_message();
  int returncode = error_msg.get_http_status_code();
  EXPECT_EQ(500, returncode);
  EXPECT_STREQ("message2", desc.c_str());
}

TEST(S3ErrorMessagesTest, SingletonCheck) {
  S3ErrorMessages *inst1 = S3ErrorMessages::get_instance();
  S3ErrorMessages *inst2 = S3ErrorMessages::get_instance();
  EXPECT_EQ(inst1, inst2);
}

TEST(S3ErrorMessagesTest, Constructor) {
  S3ErrorMessages *inst = S3ErrorMessages::get_instance();
  ASSERT_TRUE(inst->error_list.size() != 0);
}

TEST(S3ErrorMessagesTest, GetDetails) {
  S3ErrorMessages *inst = S3ErrorMessages::get_instance();
  std::string errdesc;
  int httpcode;
  S3ErrorDetails errdetails = inst->get_details("AccessDenied");
  errdesc = errdetails.get_message();
  httpcode = errdetails.get_http_status_code();
  EXPECT_STREQ("Access Denied", errdesc.c_str());
  EXPECT_EQ(403, httpcode);
}
