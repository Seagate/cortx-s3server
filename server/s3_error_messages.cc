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

#include "s3_error_messages.h"

S3ErrorMessages* S3ErrorMessages::instance = NULL;

S3ErrorMessages::S3ErrorMessages(std::string config_file) {
  Json::Value jsonroot;
  Json::Reader reader;
  std::ifstream json_file(config_file.c_str(), std::ifstream::binary);
  bool parsingSuccessful = reader.parse(json_file, jsonroot);
  if (!parsingSuccessful) {
    s3_log(S3_LOG_FATAL, "", "Json Parsing failed for file: %s.\n",
           config_file.c_str());
    s3_log(S3_LOG_FATAL, "", "Error: %s\n",
           reader.getFormattedErrorMessages().c_str());
    exit(1);
    return;
  }

  Json::Value::Members members = jsonroot.getMemberNames();
  for (auto it : members) {
    error_list[it] = S3ErrorDetails(jsonroot[it]["Description"].asString(),
                                    jsonroot[it]["httpcode"].asInt());
  }
}

S3ErrorMessages::~S3ErrorMessages() {
  // To keep jsoncpp happy in clean up.
  error_list.clear();
}

S3ErrorDetails& S3ErrorMessages::get_details(std::string code) {
  return error_list[code];
}

void S3ErrorMessages::init_messages(std::string config_file) {
  if (!instance) {
    instance = new S3ErrorMessages(config_file);
  }
}

void S3ErrorMessages::finalize() {
  if (!instance) {
    delete instance;
  }
}

S3ErrorMessages* S3ErrorMessages::get_instance() {
  if (!instance) {
    S3ErrorMessages::init_messages();
  }
  return instance;
}
