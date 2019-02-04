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
 * Original creation date: 07-January-2019
 */

#include <libxml/parser.h>
#include <libxml/xmlmemory.h>

#include <regex>
#include "s3_log.h"
#include "s3_put_tag_body.h"

S3PutTagBody::S3PutTagBody(std::string &xml, std::string &request)
    : xml_content(xml), request_id(request), is_valid(false) {

  s3_log(S3_LOG_DEBUG, request_id, "Constructor\n");
  if (!xml.empty()) {
    parse_and_validate();
  }
}

bool S3PutTagBody::isOK() { return is_valid; }

bool S3PutTagBody::parse_and_validate() {
  /* Sample body:
  <Tagging>
   <TagSet>
     <Tag>
       <Key>Project</Key>
       <Value>Project One</Value>
     </Tag>
     <Tag>
       <Key>User</Key>
       <Value>jsmith</Value>
     </Tag>
   </TagSet>
  </Tagging>

  */
  s3_log(S3_LOG_DEBUG, request_id, "Parsing put bucket tag body\n");

  bucket_tags.clear();
  is_valid = false;

  if (xml_content.empty()) {
    s3_log(S3_LOG_WARN, request_id, "XML request body Empty.\n");
    return is_valid;
  }
  s3_log(S3_LOG_DEBUG, request_id, "Parsing xml request = %s\n",
         xml_content.c_str());
  xmlDocPtr document = xmlParseDoc((const xmlChar *)xml_content.c_str());
  if (document == NULL) {
    s3_log(S3_LOG_WARN, request_id, "XML request body Invalid.\n");
    return is_valid;
  }

  xmlNodePtr tagging_node = xmlDocGetRootElement(document);

  s3_log(S3_LOG_DEBUG, request_id, "Root Node = %s", tagging_node->name);
  // Validate the root node
  if (tagging_node == NULL ||
      xmlStrcmp(tagging_node->name, (const xmlChar *)"Tagging")) {
    s3_log(S3_LOG_WARN, request_id, "XML request body Invalid.\n");
    xmlFreeDoc(document);
    return is_valid;
  }

  // Validate child node
  xmlNodePtr tagset_node = tagging_node->xmlChildrenNode;
  if (tagset_node != NULL) {
    s3_log(S3_LOG_DEBUG, request_id, "Child Node = %s", tagset_node->name);
    if ((!xmlStrcmp(tagset_node->name, (const xmlChar *)"TagSet"))) {
      // Empty TagSet is also an valid node.
      is_valid = true;
    }
  } else {
    s3_log(S3_LOG_WARN, request_id, "XML request body Invalid.\n");
    return is_valid;
  }

  // Validate sub-child node
  xmlNodePtr tag_node = tagset_node->xmlChildrenNode;
  while (tag_node != NULL) {
    s3_log(S3_LOG_DEBUG, request_id, "Sub Child Node = %s", tag_node->name);
    if ((!xmlStrcmp(tag_node->name, (const xmlChar *)"Tag"))) {
      unsigned long key_count = xmlChildElementCount(tag_node);
      s3_log(S3_LOG_DEBUG, request_id, "Tag node children count=%ld",
             key_count);
      if (key_count != 2) {
        s3_log(S3_LOG_WARN, request_id, "XML request body Invalid.\n");
        xmlFreeDoc(document);
        is_valid = false;
        return is_valid;
      }

      is_valid = read_key_value_node(tag_node);
      if (!is_valid) {
        s3_log(S3_LOG_WARN, request_id, "XML request body Invalid.\n");
        return is_valid;
      }
      tag_node = tag_node->next;
    }
  }

  xmlFreeDoc(document);
  return is_valid;
}

bool S3PutTagBody::read_key_value_node(xmlNodePtr &tag_node) {
  // Validate key values node
  xmlNodePtr key_value_node = tag_node->xmlChildrenNode;
  xmlChar *key = NULL;
  xmlChar *value = NULL;
  std::string tag_key, tag_value;
  while (key_value_node != NULL) {
    s3_log(S3_LOG_DEBUG, request_id, "Key_Value_node = %s",
           key_value_node->name);

    if (!(xmlStrcmp(key_value_node->name, (const xmlChar *)"Key"))) {
      key = xmlNodeGetContent(key_value_node);
      tag_key = reinterpret_cast<char *>(key);
      s3_log(S3_LOG_DEBUG, request_id, "Key Node = %s ", tag_key.c_str());
      // Key should not be null
      // Use each key only once for each resource as per AWS standards,
      // duplicate tags are not allowed
      if (tag_key.empty() || bucket_tags.count(tag_key) >= 1) {
        s3_log(S3_LOG_WARN, request_id, "XML request body Invalid.\n");
        xmlFree(key);
        xmlFree(value);
        return false;
      }
    } else if (!(xmlStrcmp(key_value_node->name, (const xmlChar *)"Value"))) {
      value = xmlNodeGetContent(key_value_node);
      tag_value = reinterpret_cast<char *>(value);
      s3_log(S3_LOG_DEBUG, request_id, "Value Node = %s ", tag_value.c_str());
      if (tag_value.empty()) {
        s3_log(S3_LOG_WARN, request_id, "XML request body Invalid.\n");
        xmlFree(key);
        xmlFree(value);
        return false;
      } else {
        bucket_tags[tag_key] = tag_value;
      }
    } else {
      s3_log(S3_LOG_WARN, request_id, "XML request body Invalid.\n");
      xmlFree(key);
      xmlFree(value);
      return false;
    }

    // Only a single pair of Key-Value exists within Tag Node.
    key_value_node = key_value_node->next;
  }
  xmlFree(key);
  xmlFree(value);
  return true;
}

bool S3PutTagBody::validate_bucket_xml_tags(
    std::map<std::string, std::string> &bucket_tags_as_map) {
  // Apply all validation here
  // https://docs.aws.amazon.com/awsaccountbilling/latest/aboutv2//allocation-tag-restrictions.html;
  std::string key, value;
  // Maximum number of tags per resource: 50
  if (bucket_tags_as_map.size() > BUCKET_MAX_TAGS) {
    s3_log(S3_LOG_WARN, request_id, "XML key-value tags Invalid.\n");
    return false;
  }
  for (const auto &tag : bucket_tags_as_map) {
    key = tag.first;
    value = tag.second;
    // Key-value pairs for bucket tagging should not be empty
    if (key.empty() || value.empty()) {
      s3_log(S3_LOG_WARN, request_id, "XML key-value tag Invalid.\n");
      return false;
    }
    /* To encode 256 unicode chars, last char can use upto 2 bytes.
     Core reason is std::string stores utf-8 bytes and hence .length()
     returns bytes and not number of chars.
     For refrence : https://stackoverflow.com/a/31652705
     */
    // Maximum key length: 128 Unicode characters &
    // Maximum value length: 256 Unicode characters
    if (key.length() > TAG_KEY_MAX_LENGTH ||
        value.length() > (2 * TAG_VALUE_MAX_LENGTH)) {
      s3_log(S3_LOG_WARN, request_id, "XML key-value tag Invalid.\n");
      return false;
    }
    // Allowed characters are Unicode letters, whitespace, and numbers, plus the
    // following special characters: + - = . _ : /
    // Insignificant check, as it matches all entries.
    /*
    std::regex matcher ("((\\w|\\W|\\b|\\B|-|=|.|_|:|\/)+)");
    if ( !regex_match(key,matcher) || !regex_match (value,matcher) ) {
      s3_log(S3_LOG_WARN, request_id, "XML key-value tag Invalid.\n");
      return false;
    }
    */
  }
  return true;
}

bool S3PutTagBody::validate_object_xml_tags(
    std::map<std::string, std::string> &object_tags_as_map) {
  // Apply all validation here
  // https://docs.aws.amazon.com/awsaccountbilling/latest/aboutv2//allocation-tag-restrictions.html
  // &;
  // https://docs.aws.amazon.com/AmazonS3/latest/API/RESTObjectPUTtagging.html
  std::string key, value;
  // Maximum number of tags per resource: 10
  if (object_tags_as_map.size() > OBJECT_MAX_TAGS) {
    s3_log(S3_LOG_WARN, request_id, "XML key-value tags Invalid.\n");
    return false;
  }
  for (const auto &tag : object_tags_as_map) {
    key = tag.first;
    value = tag.second;
    // Key-value pairs for bucket tagging should not be empty
    if (key.empty() || value.empty()) {
      s3_log(S3_LOG_WARN, request_id, "XML key-value tag Invalid.\n");
      return false;
    }
    /* To encode 256 unicode chars, last char can use upto 2 bytes.
     Core reason is std::string stores utf-8 bytes and hence .length()
     returns bytes and not number of chars.
     For refrence : https://stackoverflow.com/a/31652705
     */
    // Maximum key length: 128 Unicode characters &
    // Maximum value length: 256 Unicode characters
    if (key.length() > TAG_KEY_MAX_LENGTH ||
        value.length() > (2 * TAG_VALUE_MAX_LENGTH)) {
      s3_log(S3_LOG_WARN, request_id, "XML key-value tag Invalid.\n");
      return false;
    }
    // Allowed characters are Unicode letters, whitespace, and numbers, plus the
    // following special characters: + - = . _ : /
    // Insignificant check, as it matches all entries.
    /*
    std::regex matcher ("((\\w|\\W|\\b|\\B|-|=|.|_|:|\/)+)");
    if ( !regex_match(key,matcher) || !regex_match (value,matcher) ) {
      s3_log(S3_LOG_WARN, request_id, "XML key-value tag Invalid.\n");
      return false;
    }
    */
  }
  return true;
}

const std::map<std::string, std::string> &
S3PutTagBody::get_resource_tags_as_map() {
  return bucket_tags;
}
