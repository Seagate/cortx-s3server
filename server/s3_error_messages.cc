
#include <json/json.h>
#include <iostream>
#include <fstream>

#include "s3_error_messages.h"

S3ErrorMessages* S3ErrorMessages::instance = NULL;

S3ErrorMessages::S3ErrorMessages(std::string config_file) {
  Json::Value jsonroot;
  Json::Reader reader;
  std::ifstream json_file(config_file.c_str(), std::ifstream::binary);
  bool parsingSuccessful = reader.parse(json_file, jsonroot);
  if (!parsingSuccessful)
  {
    printf("Json Parsing failed for file: %s.\n", config_file.c_str());
    printf("Error: %s\n", reader.getFormattedErrorMessages().c_str());
    exit(1);
    return;
  }

  Json::Value::Members members = jsonroot.getMemberNames();
  for(auto it : members) {
    error_list[it] = S3ErrorDetails(jsonroot[it]["Description"].asString(), jsonroot[it]["httpcode"].asInt());
  }
}

S3ErrorDetails& S3ErrorMessages::get_details(std::string code){
  return error_list[code];
}

void S3ErrorMessages::init_messages(std::string config_file) {
  if(!instance){
    instance = new S3ErrorMessages(config_file);
  }
}

S3ErrorMessages* S3ErrorMessages::get_instance() {
  if(!instance){
    S3ErrorMessages::init_messages();
  }
  return instance;
}
