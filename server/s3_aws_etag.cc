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

#include "s3_aws_etag.h"
#include "s3_md5_hash.h"

int S3AwsEtag::hex_to_dec(char ch) {
  switch (ch) {
    case '0':
      return 0;
      break;
    case '1':
      return 1;
      break;
    case '2':
      return 2;
      break;
    case '3':
      return 3;
      break;
    case '4':
      return 4;
      break;
    case '5':
      return 5;
      break;
    case '6':
      return 6;
      break;
    case '7':
      return 7;
      break;
    case '8':
      return 8;
      break;
    case '9':
      return 9;
      break;
    case 'A':
      return 10;
      break;
    case 'B':
      return 11;
      break;
    case 'C':
      return 12;
      break;
    case 'D':
      return 13;
      break;
    case 'E':
      return 14;
      break;
    case 'F':
      return 15;
      break;
    case 'a':
      return 10;
      break;
    case 'b':
      return 11;
      break;
    case 'c':
      return 12;
      break;
    case 'd':
      return 13;
      break;
    case 'e':
      return 14;
      break;
    case 'f':
      return 15;
      break;
    default:
      s3_log(S3_LOG_ERROR, "", "Invalid hexadecimal digit %c \n", ch);
      return -1;
  }
}

std::string S3AwsEtag::convert_hex_bin(std::string hex) {
  std::string binary;
  unsigned char digest_byte = 0;
  for (size_t i = 0; i < hex.length(); i += 2) {
    digest_byte = hex_to_dec(hex[i]);

    digest_byte = digest_byte << 4;  // shift to msb

    digest_byte = digest_byte | (unsigned char)hex_to_dec(hex[i + 1]);

    binary.push_back(digest_byte);
  }
  return binary;
}

void S3AwsEtag::add_part_etag(std::string etag) {
  hex_etag += etag;
  part_count++;
}

std::string S3AwsEtag::finalize() {
  std::string binary_etag = convert_hex_bin(hex_etag);

  MD5hash hash;
  hash.Update(binary_etag.c_str(), binary_etag.length());
  hash.Finalize();

  final_etag = hash.get_md5_string() + "-" + std::to_string(part_count);

  return final_etag;
}

std::string S3AwsEtag::get_final_etag() { return final_etag; }
