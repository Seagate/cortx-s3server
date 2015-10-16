
#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_ERROR_CODES_H__
#define __MERO_FE_S3_SERVER_S3_ERROR_CODES_H__

#include "s3_error_messages.h"

#define S3HttpSuccess200         EVHTP_RES_OK
#define S3HttpSuccess204         EVHTP_RES_NOCONTENT
#define S3HttpFailed400          EVHTP_RES_400
#define S3HttpFailed409          EVHTP_RES_CONFLICT
#define S3HttpFailed500          EVHTP_RES_500
#define S3HttpFailed404          EVHTP_RES_NOTFOUND


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
  S3ErrorDetails& details;

  std::string xml_message;
public:
  S3Error(std::string error_code, std::string req_id, std::string res_key);

  int get_http_status_code();

  std::string& to_xml();
};

#endif
