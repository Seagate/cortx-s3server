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

#ifndef __S3_SERVER_S3_AWS_ETAG_H__
#define __S3_SERVER_S3_AWS_ETAG_H__

#include <gtest/gtest_prod.h>
#include <string>
#include "s3_log.h"

// Used to generate Etag for multipart uploads.
class S3AwsEtag {
  std::string hex_etag;
  std::string final_etag;
  int part_count;

  // Helpers
  int hex_to_dec(char ch);
  std::string convert_hex_bin(std::string hex);

 public:
  S3AwsEtag() : part_count(0) {}

  void add_part_etag(std::string etag);
  std::string finalize();
  std::string get_final_etag();
  FRIEND_TEST(S3AwsEtagTest, Constructor);
  FRIEND_TEST(S3AwsEtagTest, HexToDec);
  FRIEND_TEST(S3AwsEtagTest, HexToDecInvalid);
  FRIEND_TEST(S3AwsEtagTest, HexToBinary);
  FRIEND_TEST(S3AwsEtagTest, AddPartEtag);
  FRIEND_TEST(S3AwsEtagTest, Finalize);
  FRIEND_TEST(S3AwsEtagTest, GetFinalEtag);
};

#endif
