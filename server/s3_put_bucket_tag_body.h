/*
 * COPYRIGHT 2019 SEAGATE LLC
 *
 * THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
 * HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE TECHNOLOGY
 * LIMITED, ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
 * THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
 * BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
 * USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
 * EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.
 *
 * YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
 * THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
 * http://www.seagate.com/contact
 *
 * Original author:  Siddhivinayak Shanbhag <siddhivinayak.shanbhag@seagate.com>
 * Original creation date: 09-January-2019
 */

#pragma once

#ifndef __S3_SERVER_S3_PUT_BUCKET_TAG_BODY_H__
#define __S3_SERVER_S3_PUT_BUCKET_TAG_BODY_H__

#include <gtest/gtest_prod.h>
#include <string>
#include <map>
#include <libxml/xmlmemory.h>

#define BUCKET_MAX_TAGS 50
#define BUCKET_TAG_KEY_MAX_LENGTH 128
#define BUCKET_TAG_VALUE_MAX_LENGTH 256

class S3PutBucketTagBody {
  std::string xml_content;
  std::string request_id;
  std::map<std::string, std::string> bucket_tags;

  bool is_valid;
  bool parse_and_validate();

 public:
  S3PutBucketTagBody(std::string& xml, std::string& request);
  virtual bool isOK();
  virtual bool read_key_value_node(xmlNodePtr& sub_child);
  virtual bool validate_xml_tags(
      std::map<std::string, std::string>& bucket_tags_as_map);
  virtual const std::map<std::string, std::string>& get_bucket_tags_as_map();

  // For Testing purpose
  FRIEND_TEST(S3PutBucketTagBodyTest, ValidateRequestBodyXml);
  FRIEND_TEST(S3PutBucketTagBodyTest, ValidateRequestCompareContents);
  FRIEND_TEST(S3PutBucketTagBodyTest, ValidateRepeatedKeysXml);
  FRIEND_TEST(S3PutBucketTagBodyTest, ValidateEmptyTagSetXml);
  FRIEND_TEST(S3PutBucketTagBodyTest, ValidateEmptyTagsXml);
  FRIEND_TEST(S3PutBucketTagBodyTest, ValidateEmptyKeyXml);
  FRIEND_TEST(S3PutBucketTagBodyTest, ValidateEmptyValueXml);
  FRIEND_TEST(S3PutBucketTagBodyTest, ValidateValidRequestTags);
  FRIEND_TEST(S3PutBucketTagBodyTest, ValidateRequestInvalidTagSize);
  FRIEND_TEST(S3PutBucketTagBodyTest, ValidateRequestInvalidTagCount);
};

#endif
