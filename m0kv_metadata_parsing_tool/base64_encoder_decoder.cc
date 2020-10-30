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

#define __STDC_LIMIT_MACROS

struct m0_uint128 {
    uint64_t u_hi;
    uint64_t u_lo;
};

static const std::string base64_chars =
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";
             
static inline bool is_base64(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}



/**********************************************
this function decodes the base64 encoded string 
**********************************************/
std::string base64_decode(std::string const& encoded_string) {
  int in_len = encoded_string.size();
  int i = 0;
  int j = 0;
  int in_ = 0;
  unsigned char char_array_4[4], char_array_3[3];
  std::string ret;

  while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
    char_array_4[i++] = encoded_string[in_]; in_++;
    if (i ==4) {
      for (i = 0; i <4; i++)
        char_array_4[i] = base64_chars.find(char_array_4[i]);

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; (i < 3); i++)
        ret += char_array_3[i];
      i = 0;
    }
  }

  if (i) {
    for (j = i; j <4; j++)
      char_array_4[j] = 0;

    for (j = 0; j <4; j++)
      char_array_4[j] = base64_chars.find(char_array_4[j]);

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
  }
  return ret;
}

/***************************************************
this function is used to encode the string in base64
***************************************************/
std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len) {
  std::string ret;
  int i = 0;
  int j = 0;
  unsigned char char_array_3[3];
  unsigned char char_array_4[4];

  while (in_len--) {
    char_array_3[i++] = *(bytes_to_encode++);
    if (i == 3) {
      char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
      char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
      char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
      char_array_4[3] = char_array_3[2] & 0x3f;

      for(i = 0; (i <4) ; i++)
        ret += base64_chars[char_array_4[i]];
      i = 0;
    }
  }

  if (i)
  {
    for(j = i; j < 3; j++)
      char_array_3[j] = '\0';

    char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
    char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
    char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
    char_array_4[3] = char_array_3[2] & 0x3f;

    for (j = 0; (j < i + 1); j++)
      ret += base64_chars[char_array_4[j]];

    while((i++ < 3))
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

/****************************************
main function 
****************************************/
int main(int argc, char *argv[])
{
  if (std::string(argv[1]) == "-d") {
    struct m0_uint128 id;
    id = to_m0_uint128(std::string(argv[2]));
    std::cout << "0x" << std::hex << id.u_hi << ":0x" << std::hex << id.u_lo <<"\n";
  }
  else if (std::string(argv[1]) == "-e") {
    std::string encoded_string = to_string(std::string(argv[2]));
    std::cout << encoded_string<<"\n";
  }
  else {
    std::cout << "-----------------------Usage------------------------\n";
    std::cout << "(ENCODE): ./base64_encoder_decoder -e <input_string>\n";
    std::cout << "(DECODE): ./base64_encoder_decoder -d <input_string>\n";
  }
  return 0;
}



