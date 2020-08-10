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

#pragma once

#ifndef __S3_SERVER_S3_DELETE_MULTIPLE_OBJECTS_RESPONSE_BODY_H__
#define __S3_SERVER_S3_DELETE_MULTIPLE_OBJECTS_RESPONSE_BODY_H__

class SuccessDeleteKey {
  std::string object_key;
  std::string version;

 public:
  SuccessDeleteKey(std::string key, std::string id = "") : object_key(key) {}

  std::string to_xml() {
    std::string xml = "";
    xml +=
        "<Deleted>\n"
        "  <Key>" +
        object_key +
        "</Key>\n"
        //  "  <VersionId>" + version + "</VersionId>\n"
        //  "  <DeleteMarker>" + true + "</DeleteMarker>\n"
        //  "  <DeleteMarkerVersionId>" + true + "</DeleteMarkerVersionId>\n"
        "</Deleted>\n";
    return xml;
  }
};

class ErrorDeleteKey {
  std::string object_key;
  std::string error_code;
  std::string error_message;

 public:
  ErrorDeleteKey(std::string key, std::string code, std::string msg)
      : object_key(key), error_code(code), error_message(msg) {}

  std::string to_xml() {
    std::string xml = "";
    xml +=
        "<Error>\n"
        "  <Key>" +
        object_key +
        "</Key>\n"
        //  "  <VersionId>" + owner_name + "</VersionId>\n"
        "  <Code>" +
        error_code +
        "</Code>\n"
        "  <Message>" +
        error_message +
        "</Message>\n"
        "</Error>\n";
    return xml;
  }
};

class S3DeleteMultipleObjectsResponseBody {
  std::vector<SuccessDeleteKey> success;
  // key, errorcode
  std::vector<ErrorDeleteKey> error;

  std::string response_xml;

 public:
  void add_success(std::string key, std::string version = "") {
    success.push_back(SuccessDeleteKey(key, version));
  }

  size_t get_success_count() { return success.size(); }

  void add_failure(std::string key, std::string code,
                   std::string message = "") {
    error.push_back(ErrorDeleteKey(key, code, message));
  }

  size_t get_failure_count() { return error.size(); }

  std::string& to_xml(bool quiet_mode = false) {

    response_xml = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    response_xml +=
        "<DeleteResult xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">";

    // By default response includes the result of deletion for each key in the
    // request.
    // In quiet mode response includes only keys where the delete operation
    // encountered an error.

    if (!quiet_mode) {
      for (auto sitem : success) {
        response_xml += sitem.to_xml();
      }
    }
    for (auto eitem : error) {
      response_xml += eitem.to_xml();
    }
    response_xml += "</DeleteResult>";
    return response_xml;
  }

  FRIEND_TEST(S3DeleteMultipleObjectsResponseBodyTest, ConstructorTest);
};

#endif
