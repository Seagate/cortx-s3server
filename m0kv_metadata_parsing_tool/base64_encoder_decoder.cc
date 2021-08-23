/*
# Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# For any questions about this software or licensing,
# please email opensource@seagate.com or cortx-questions@seagate.com.
*/

#include<stdio.h>
#include<iostream>
#include <cstdint>
#include<cstring>  
#include<inttypes.h>
#include<iomanip>
#include<sstream>
#include "s3_motr_context.h"

static const std::string base64_chars =
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";
             
static inline bool is_base64(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

/****************************************************
this function decodes the base64 encoded string 
Logic :
  This function picks up 4 characters(24 bits) at a 
  time to decode base64 string and the we perform left 
  shift until '=' is found and then we perform right
  shift and retreive the store bits in integer value.
*****************************************************/

std::string base64_decode(std::string const& encoded_string) {
  int in_len = encoded_string.size();
  int pos_iterator = 0; // This is postional iterator to traverse through string.
  int in_ = 0;
  unsigned char char_array_4[4], char_array_3[3];
  std::string ret;

  while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
    char_array_4[pos_iterator++] = encoded_string[in_]; in_++;
    if (pos_iterator ==4) {
      for (pos_iterator = 0; pos_iterator <4; pos_iterator++)
        char_array_4[pos_iterator] = base64_chars.find(char_array_4[pos_iterator]);

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (pos_iterator = 0; (pos_iterator < 3); pos_iterator++)
        ret += char_array_3[pos_iterator];
      pos_iterator = 0;
    }
  }

  if (pos_iterator) {
    int null_chars  = 0; // No. of zeroes to append if number of characters are less than 3 in char_array_3
    for (null_chars = pos_iterator; null_chars <4; null_chars++)
      char_array_4[null_chars] = 0;

    for (null_chars = 0; null_chars <4; null_chars++)
      char_array_4[null_chars] = base64_chars.find(char_array_4[null_chars]);

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (null_chars = 0; (null_chars < pos_iterator - 1); null_chars++) ret += char_array_3[null_chars];
  }
  return ret;
}

/****************************************************
this function is used to encode the string in base64
Logic :
  This function picks up 3 characters at a time from
  stream of size 8 bits and divide in block of 6 bits
  and then perform right shift and then left shift on 
  this block and convert it base64 format.
*****************************************************/

std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) {
  std::string ret;
  
  int pos_iterator = 0; // This is postional iterator to traverse through string.
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  while (in_len--) {
    char_array_3[pos_iterator++] = *(bytes_to_encode++);
    if (pos_iterator == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for(pos_iterator = 0; (pos_iterator <4) ; pos_iterator++)
        ret += base64_chars[char_array_4[pos_iterator]];
      pos_iterator = 0;
    }
  }

  if (pos_iterator)
  {
    int null_chars  = 0; // No. null chars to append if number of characters are less than 4 in char_array_4
    for(null_chars = pos_iterator; null_chars < 3; null_chars++)
      char_array_3[null_chars] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (null_chars = 0; (null_chars < pos_iterator + 1); null_chars++)
      ret += base64_chars[char_array_4[null_chars]];

    while((pos_iterator++ < 3))
      ret += '=';

  }

  return ret;
}

/******************************************************************************
 this function breaks the encoded string in hi and lo format and decodes string
******************************************************************************/
struct m0_uint128 to_m0_uint128(const std::string &id_str) {
    m0_uint128 id = {0ULL, 0ULL};
    std::size_t delim_pos = id_str.find("-");
    if (delim_pos != std::string::npos) {
    std::string dec_id_u_hi = base64_decode(id_str.substr(0, delim_pos));
    std::string dec_id_u_lo = base64_decode(id_str.substr(delim_pos + 1));
    if ((dec_id_u_hi.size() == sizeof(id.u_hi)) &&
        (dec_id_u_lo.size() == sizeof(id.u_lo))) {
        memcpy((void *)&id.u_hi, dec_id_u_hi.c_str(), sizeof(id.u_hi));
        memcpy((void *)&id.u_lo, dec_id_u_lo.c_str(), sizeof(id.u_lo));
    }
    }
    return id;
}


/*********************************************************************************
 this function is used to divide the hex address in two parts and then encode them
*********************************************************************************/
std::string to_string(std::string input_string) {
  std::size_t delim_pos = input_string.find(":");
  std::string u_hi_hex = input_string.substr(0, delim_pos);
  std::string u_lo_hex = input_string.substr(delim_pos + 1);
  std::stringstream ss, ss1;
  uint64_t u_hi, u_lo;
  ss << u_hi_hex;
  ss >> std::hex >> u_hi;
  ss1 << u_lo_hex;
  ss1 >> std::hex >> u_lo;
  return base64_encode((unsigned char const *)&u_hi, sizeof(u_hi)) + "-" +
         base64_encode((unsigned char const *)&u_lo, sizeof(u_lo));
}

struct s3_motr_idx_layout to_idx_layout(const std::string &encoded) {

  struct s3_motr_idx_layout lo;
  memset(&lo, 0, sizeof lo);

  std::string s_decoded = base64_decode(encoded);
  memcpy(&lo, s_decoded.c_str(), s_decoded.length());

  return lo;
}

std::string to_string(const m0_uint128 &id) {
  return base64_encode((unsigned char const *)&id.u_hi, sizeof(id.u_hi)) + "-" +
         base64_encode((unsigned char const *)&id.u_lo, sizeof(id.u_lo));
}

/****************************************
main function 
****************************************/
int main(int argc, char *argv[])
{
  if (argc == 3)
  {
    if (std::string(argv[1]) == "-decode_oid") {
      struct m0_uint128 id;
      id = to_m0_uint128(std::string(argv[2]));
      std::cout << "0x" << std::hex << id.u_hi << ":0x" << std::hex << id.u_lo <<"\n";
    } else if (std::string(argv[1]) == "-encode_oid") {
      std::string encoded_string = to_string(std::string(argv[2]));
      std::cout << encoded_string <<"\n";
    } else if (std::string(argv[1]) == "-decode_layout") {
      struct s3_motr_idx_layout idx = to_idx_layout(std::string(argv[2]));
      std::cout << to_string(idx.oid) << std::endl;
    } else {
      std::cout << "-----------------------Usage------------------------\n";
      std::cout << "(ENCODE OID): ./base64_encoder_decoder -encode_oid "
                   "<input_string>\n";
      std::cout << "(DECODE OID): ./base64_encoder_decoder -decode_oid "
                   "<input_string>\n";
      std::cout << "(DECODE LAYOUT): ./base64_encoder_decoder -decode_layout "
                   "<input_string>\n";
    }
  } 
  else {
    std::cout << "-----------------------Usage------------------------\n";
    std::cout << "(ENCODE OID): ./base64_encoder_decoder -encode_oid "
                 "<input_string>\n";
    std::cout << "(DECODE OID): ./base64_encoder_decoder -decode_oid "
                 "<input_string>\n";
    std::cout << "(DECODE LAYOUT): ./base64_encoder_decoder -decode_layout "
                 "<input_string>\n";
  }
  return 0;
}

