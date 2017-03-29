/*
 * COPYRIGHT 2015 SEAGATE LLC
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
 * Original author:  Rajesh Nambiar   <rajesh.nambiarr@seagate.com>
 * Original author:  Kaustubh Deorukhkar   <kaustubh.deorukhkar@seagate.com>
 * Original creation date: 09-Nov-2015
 */

#pragma once

#ifndef __S3_UT_MOCK_S3_REQUEST_OBJECT_H__
#define __S3_UT_MOCK_S3_REQUEST_OBJECT_H__

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "s3_request_object.h"

class MockS3RequestObject : public S3RequestObject {
 public:
  MockS3RequestObject(evhtp_request_t *req, EvhtpInterface *evhtp_obj_ptr)
      : S3RequestObject(req, evhtp_obj_ptr) {}
  MOCK_METHOD0(c_get_full_path, const char *());
  MOCK_METHOD0(c_get_full_encoded_path, const char *());
  MOCK_METHOD0(get_host_header, std::string());
  MOCK_METHOD0(http_verb, S3HttpVerb());
  MOCK_METHOD0(c_get_uri_query, const char *());
  MOCK_CONST_METHOD0(get_object_name, const std::string &());
  MOCK_CONST_METHOD0(get_bucket_name, const std::string &());
  MOCK_METHOD0(get_request, evhtp_request_t *());
  MOCK_METHOD1(has_query_param_key, bool(std::string key));
  MOCK_METHOD1(set_bucket_name, void(const std::string &name));
  MOCK_METHOD1(set_object_name, void(const std::string &name));
  MOCK_METHOD1(set_api_type, void(S3ApiType));
  MOCK_METHOD1(get_query_string_value, std::string(std::string key));
  MOCK_METHOD0(get_api_type, S3ApiType());
  MOCK_METHOD1(respond_retry_after, void(int retry_after_in_secs));
  MOCK_METHOD2(set_out_header_value, void(std::string, std::string));
  MOCK_METHOD0(get_in_headers_copy, std::map<std::string, std::string> &());
  MOCK_METHOD2(send_response, void(int, std::string));
};

#endif
