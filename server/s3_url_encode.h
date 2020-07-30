 */

#pragma once

#ifndef __S3_SERVER_S3_URL_ENCODE_H__
#define __S3_SERVER_S3_URL_ENCODE_H__

#include <string>

void escape_char(char c, std::string& destination);

bool char_needs_url_encoding(char c);

std::string url_encode(const char* src);

#endif
