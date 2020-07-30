#pragma once

#ifndef __S3_SERVER_S3_ERROR_MESSAGES_H__
#define __S3_SERVER_S3_ERROR_MESSAGES_H__

#include <gtest/gtest_prod.h>
#include <map>
#include <string>
#include "s3_log.h"

class S3ErrorDetails {
  std::string description;
  int http_return_code;

 public:
  S3ErrorDetails() : description("Unknown Error"), http_return_code(520) {}

  S3ErrorDetails(std::string message, int http_code) {
    description = message;
    http_return_code = http_code;
  }

  std::string& get_message() { return description; }

  int get_http_status_code() { return http_return_code; }

  FRIEND_TEST(S3ErrorDetailsTest, DefaultConstructor);
  FRIEND_TEST(S3ErrorDetailsTest, ConstructorWithMsg);
};

class S3ErrorMessages {
 private:
  static S3ErrorMessages* instance;
  S3ErrorMessages(std::string config_file);
  ~S3ErrorMessages();

  std::map<std::string, S3ErrorDetails> error_list;

 public:
  // Loads messages and creates singleton
  static void init_messages(
      std::string config_file =
          "/opt/seagate/cortx/s3/resources/s3_error_messages.json");

  // Cleans up the singleton instance
  static void finalize();

  static S3ErrorMessages* get_instance();

  S3ErrorDetails& get_details(std::string code);

  FRIEND_TEST(S3ErrorMessagesTest, Constructor);
};

#endif
