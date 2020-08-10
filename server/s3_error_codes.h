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

#ifndef __S3_SERVER_S3_ERROR_CODES_H__
#define __S3_SERVER_S3_ERROR_CODES_H__

#include "s3_error_messages.h"

#define S3HttpSuccess200 EVHTP_RES_OK
#define S3HttpSuccess201 EVHTP_RES_CREATED
#define S3HttpSuccess204 EVHTP_RES_NOCONTENT
#define S3HttpSuccess206 EVHTP_RES_PARTIAL
#define S3HttpFailed400 EVHTP_RES_400
#define S3HttpFailed401 EVHTP_RES_UNAUTH
#define S3HttpFailed403 EVHTP_RES_FORBIDDEN
#define S3HttpFailed404 EVHTP_RES_NOTFOUND
#define S3HttpFailed405 EVHTP_RES_METHNALLOWED
#define S3HttpFailed409 EVHTP_RES_CONFLICT
#define S3HttpFailed500 EVHTP_RES_500
#define S3HttpFailed503 EVHTP_RES_SERVUNAVAIL

/* Example error message:
  <?xml version="1.0" encoding="UTF-8"?>
  <Error>
    <Code>NoSuchKey</Code>
    <Message>The resource you requested does not exist</Message>
    <Resource>/mybucket/myfoto.jpg</Resource>
    <RequestId>4442587FB7D0A2F9</RequestId>
  </Error>
 */

class S3Error {
  std::string code;  // Error codes are read from s3_error_messages.json
  std::string request_id;
  std::string resource_key;
  std::string auth_error_message;
  S3ErrorDetails& details;

  std::string xml_message;

 public:
  S3Error(std::string error_code, std::string req_id, std::string res_key,
          std::string error_message = "");

  int get_http_status_code();

  std::string& to_xml();
};

#endif
