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

#include "gtest/gtest.h"

#include "libxml/parser.h"
#include "libxml/xmlmemory.h"

#include "s3_error_codes.h"

// To use a test fixture, derive a class from testing::Test.
class S3ErrorTest : public testing::Test {
 protected:  // You should make the members protected s.t. they can be
             // accessed from sub-classes.

  // A helper function for testing xml content.
  xmlDocPtr get_parsed_doc(std::string& content) {
    xmlDocPtr document = xmlParseDoc((const xmlChar*)content.c_str());
    return document;
  }

  void is_valid_xml(std::string& xml_content) {
    xmlDocPtr document = get_parsed_doc(xml_content);
    ASSERT_FALSE(document == NULL);
    xmlFreeDoc(document);
  }

  void xmls_are_equal(std::string& first, std::string& second) {
    xmlDocPtr document_first = get_parsed_doc(first);
    ASSERT_FALSE(document_first == NULL);

    xmlChar* content_first = NULL;
    int size = 0;
    xmlDocDumpMemory(document_first, &content_first, &size);

    xmlDocPtr document_second = get_parsed_doc(second);
    ASSERT_FALSE(document_second == NULL);

    xmlChar* content_second = NULL;
    size = 0;
    xmlDocDumpMemory(document_second, &content_second, &size);

    ASSERT_TRUE(xmlStrcmp(content_first, content_second) == 0);

    xmlFree(content_first);
    xmlFreeDoc(document_first);
    xmlFree(content_second);
    xmlFreeDoc(document_second);
  }

  void has_element_with_value(std::string& xml_content,
                              std::string element_name,
                              std::string element_value) {
    xmlDocPtr document = get_parsed_doc(xml_content);
    ASSERT_FALSE(document == NULL);

    xmlNodePtr root_node = xmlDocGetRootElement(document);
    ASSERT_FALSE(root_node == NULL);

    // Now search for the element_name
    xmlNodePtr child = root_node->xmlChildrenNode;
    xmlChar* value = NULL;
    while (child != NULL) {
      if ((!xmlStrcmp(child->name, (const xmlChar*)element_name.c_str()))) {
        value = xmlNodeGetContent(child);
        ASSERT_FALSE(value == NULL);
        if (value == NULL) {
          xmlFree(value);
          xmlFreeDoc(document);
          return;
        }
        EXPECT_STREQ(element_value.c_str(), (char*)value);

        xmlFree(value);
        xmlFreeDoc(document);
        return;
      } else {
        child = child->next;
      }
    }
    FAIL() << "Element [" << element_name
           << "] is missing in the xml document [" << xml_content << "]";
  }

  // Declares the variables your tests want to use.
};

TEST_F(S3ErrorTest, HasValidHttpCodes) {
  S3Error error("BucketNotEmpty", "dummy-request-id", "SomeBucketName");
  EXPECT_EQ(409, error.get_http_status_code());
}

TEST_F(S3ErrorTest, ReturnValidErrorXml) {
  S3Error error("BucketNotEmpty", "dummy-request-id", "SomeBucketName");
  std::string xml_content = error.to_xml();
  is_valid_xml(xml_content);

  std::string expected_response =
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  expected_response +=
      "<Error>\n"
      "<Code>BucketNotEmpty</Code>"
      "<Message>The bucket you tried to delete is not empty.</Message>"
      "<Resource>SomeBucketName</Resource>"
      "<RequestId>dummy-request-id</RequestId>"
      "</Error>\n";

  EXPECT_EQ(expected_response, xml_content);
  // xmls_are_equal(xml_content, expected_response);

  has_element_with_value(xml_content, "Message",
                         "The bucket you tried to delete is not empty.");
}

TEST_F(S3ErrorTest, Negative) {
  S3Error error("NegativeTestCase", "dummy-request-id", "SomeBucketName");
  std::string expected_response =
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  expected_response +=
      "<Error>\n"
      "<Code>NegativeTestCase</Code>"
      "<Message>Unknown Error</Message>"
      "<Resource>SomeBucketName</Resource>"
      "<RequestId>dummy-request-id</RequestId>"
      "</Error>\n";
  EXPECT_EQ(520, error.get_http_status_code());
  EXPECT_EQ(expected_response, error.to_xml());
}
