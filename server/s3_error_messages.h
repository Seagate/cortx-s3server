
#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_ERROR_MESSAGES_H__
#define __MERO_FE_S3_SERVER_S3_ERROR_MESSAGES_H__

#include <string>
#include <map>

class S3ErrorDetails {
  std::string description;
  int http_return_code;
public:
  S3ErrorDetails() : description(""), http_return_code(-1) {}

  S3ErrorDetails(std::string message, int http_code) {
    description = message;
    http_return_code = http_code;
  }

  std::string& get_message() {
    return description;
  }

  int get_http_status_code() {
    return http_return_code;
  }
};

class S3ErrorMessages {
private:
  static S3ErrorMessages* instance;
  S3ErrorMessages(std::string config_file);

  std::map<std::string, S3ErrorDetails> error_list;
public:
  // Loads messages and creates singleton
  static void init_messages(std::string config_file = "/opt/seagate/s3/resources/s3_error_messages.json");

  static S3ErrorMessages* get_instance();

  S3ErrorDetails& get_details(std::string code);
};

#endif
