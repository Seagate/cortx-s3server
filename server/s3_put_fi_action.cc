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
 * Original author:  Abrarahmed Momin  <abrar.habib@seagate.com>
 * Original creation date: 21th-June-2016
 */

#include <string>
#include <iostream>
#include <sstream>

#include "s3_option.h"
#include "s3_put_fi_action.h"
#include "s3_error_codes.h"
#include "s3_log.h"
#include "s3_fi_common.h"

S3PutFiAction::S3PutFiAction(std::shared_ptr<S3RequestObject> req)
    : S3Action(req, false) {
  s3_log(S3_LOG_DEBUG, "Constructor\n");

  clear_tasks();
  setup_steps();
}

void S3PutFiAction::setup_steps() {
  s3_log(S3_LOG_DEBUG, "Setting up the action\n");
  add_task(std::bind(&S3PutFiAction::set_fault_injection, this));
  add_task(std::bind(&S3PutFiAction::send_response_to_s3_client, this));
  // ...
}

void S3PutFiAction::parse_command() {
  std::string token, cstring;

  int count = 0;
  cstring = request->get_header_value("x-seagate-faultinjection");
  if (cstring.empty()) {
    return_error = "Malformed header";
    send_response_to_s3_client();  // Empty fault tag, report error
    return;
  }

  std::istringstream cline(cstring);

  while (std::getline(cline, token, ',')) {
    switch (count) {
      case 0:
        fi_opcode = token;
        break;
      case 1:
        fi_cmd = token;
        break;
      case 2:
        fi_tag = token;
        break;
      case 3:
        fi_param1 = token;
        break;
      case 4:
        fi_param2 = token;
        break;
      default:
        s3_log(S3_LOG_DEBUG, "Too many parameters\n");
        return_error = "Too many parameters";
        send_response_to_s3_client();
        return;
    }
    count++;
  }
  s3_log(S3_LOG_DEBUG, "parse_command:%s:%s:%s \n", fi_opcode.c_str(),
         fi_cmd.c_str(), fi_tag.c_str());
}

void S3PutFiAction::set_fault_injection() {
  s3_log(S3_LOG_DEBUG, "set_fault_injection\n");
  // get tag
  parse_command();

  if (fi_opcode.compare("enable") == 0) {
    s3_log(S3_LOG_DEBUG, " Fault enable:%s:%s\n", fi_cmd.c_str(),
           fi_tag.c_str());
    if (fi_cmd.compare("once") == 0)
      s3_fi_enable_once(fi_tag.c_str());
    else if (fi_cmd.compare("always") == 0)
      s3_fi_enable(fi_tag.c_str());
    else if (fi_cmd.compare("random") == 0)
      s3_fi_enable_random(fi_tag.c_str(), stoi(fi_param1));
    else if (fi_cmd.compare("enablen") == 0)
      s3_fi_enable_each_nth_time(fi_tag.c_str(), stoi(fi_param1));
    else if (fi_cmd.compare("offnonm") == 0)
      s3_fi_enable_off_n_on_m(fi_tag.c_str(), stoi(fi_param1), stoi(fi_param2));
    else {
      return_error = "Invalid command param";
      s3_log(S3_LOG_DEBUG, "Invalid command:%s\n", fi_cmd.c_str());
    }

  } else if (fi_opcode.compare("disable") == 0) {
    s3_log(S3_LOG_DEBUG, " Fault disable:%s:%s\n", fi_cmd.c_str(),
           fi_tag.c_str());
    s3_fi_disable(fi_tag.c_str());

  } else if (fi_opcode.compare("test") == 0) {
    if (s3_fi_is_enabled(fi_tag.c_str())) {
      s3_log(S3_LOG_DEBUG, " Fault:%s enabled \n", fi_tag.c_str());
    } else {
      s3_log(S3_LOG_DEBUG, " Fault:%s disabled \n", fi_tag.c_str());
    }

  } else {
    return_error = "Invalid opcode";
    s3_log(S3_LOG_DEBUG, "Invalid opcode:%s\n", fi_opcode.c_str());
  }
  next();
}

void S3PutFiAction::send_response_to_s3_client() {
  s3_log(S3_LOG_DEBUG, "Entering\n");

  if (return_error.empty()) {
    request->send_response(S3HttpSuccess200);
  } else {
    // Invalid request
    S3Error error("MalformedFICmd", request->get_request_id(), "");
    std::string& response_xml = error.to_xml();
    request->set_out_header_value("Content-Type", "application/xml");
    request->set_out_header_value("Content-Length",
                                  std::to_string(response_xml.length()));
    request->set_out_header_value("x-seagate-faultinjection",
                                  request->get_request_id());
    request->send_response(error.get_http_status_code(), response_xml);
  }
  request->resume();

  done();
  i_am_done();  // self delete
  s3_log(S3_LOG_DEBUG, "Exiting\n");
}
