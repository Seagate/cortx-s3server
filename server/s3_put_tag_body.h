#pragma once

#ifndef __S3_SERVER_S3_PUT_TAG_BODY_H__
#define __S3_SERVER_S3_PUT_TAG_BODY_H__

#include <gtest/gtest_prod.h>
#include <string>
#include <map>
#include <libxml/xmlmemory.h>

#define BUCKET_MAX_TAGS 50
#define OBJECT_MAX_TAGS 10
#define TAG_KEY_MAX_LENGTH 128
#define TAG_VALUE_MAX_LENGTH 256

class S3PutTagBody {
  std::string xml_content;
  std::string request_id;
  std::map<std::string, std::string> bucket_tags;

  bool is_valid;
  bool parse_and_validate();

 public:
  S3PutTagBody(std::string& xml, std::string& request);
  virtual bool isOK();
  virtual bool read_key_value_node(xmlNodePtr& sub_child);
  virtual bool validate_bucket_xml_tags(
      std::map<std::string, std::string>& bucket_tags_as_map);
  virtual bool validate_object_xml_tags(
      std::map<std::string, std::string>& object_tags_as_map);
  virtual const std::map<std::string, std::string>& get_resource_tags_as_map();

  // For Testing purpose
  FRIEND_TEST(S3PutTagBodyTest, ValidateRequestBodyXml);
  FRIEND_TEST(S3PutTagBodyTest, ValidateRequestCompareContents);
  FRIEND_TEST(S3PutTagBodyTest, ValidateRepeatedKeysXml);
  FRIEND_TEST(S3PutTagBodyTest, ValidateEmptyTagSetXml);
  FRIEND_TEST(S3PutTagBodyTest, ValidateEmptyTagsXml);
  FRIEND_TEST(S3PutTagBodyTest, ValidateEmptyKeyXml);
  FRIEND_TEST(S3PutTagBodyTest, ValidateEmptyValueXml);
  FRIEND_TEST(S3PutTagBodyTest, ValidateValidRequestTags);
  FRIEND_TEST(S3PutTagBodyTest, ValidateRequestInvalidTagSize);
  FRIEND_TEST(S3PutTagBodyTest, ValidateRequestInvalidTagCount);
};

#endif
