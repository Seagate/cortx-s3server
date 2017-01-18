/*
 * COPYRIGHT 2016 SEAGATE LLC
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
 * Original author:  Rajesh Nambiar   <rajesh.nambiar@seagate.com>
 * Author:  Kaustubh Deorukhkar   <kaustubh.deorukhkar@seagate.com>
 * Original creation date: 10-Mar-2016
 */

#pragma once

#ifndef __MERO_FE_S3_SERVER_S3_AUTH_RESPONSE_SUCCESS_H__
#define __MERO_FE_S3_SERVER_S3_AUTH_RESPONSE_SUCCESS_H__

#include <string>

class S3AuthResponseSuccess {
  std::string xml_content;
  bool is_valid;

  std::string user_name;
  std::string user_id;
  std::string account_name;
  std::string account_id;
  std::string signature_SHA256;
  std::string request_id;

  bool parse_and_validate();

 public:
  S3AuthResponseSuccess(std::string& xml);
  bool isOK();

  const std::string& get_user_name();
  const std::string& get_user_id();
  const std::string& get_account_name();
  const std::string& get_account_id();
  const std::string& get_signature_sha256();
  const std::string& get_request_id();
};

#endif
