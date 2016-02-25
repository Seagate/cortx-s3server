
#pragma once

#ifndef __MERO_FE_S3_SERVER_BASE64_H__
#define __MERO_FE_S3_SERVER_BASE64_H__

#include <string>

std::string base64_encode(unsigned char const* , unsigned int len);
std::string base64_decode(std::string const& s);

#endif
