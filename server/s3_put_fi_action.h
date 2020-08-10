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

#ifndef __S3_SERVER_S3_PUT_FI_ACTION_H__
#define __S3_SERVER_S3_PUT_FI_ACTION_H__

#include "s3_action_base.h"

/* Command syntax to work with S3server Fault Injection library
 * Command: HTTP PUT against base URL  eg: PUT http://s3.seagate.com
 * Header : Header should consist of "name: multiple value" pair indicating
 *action to be invoked.
 * name   : Always "x-seagate-faultinjection" to be interpreted as fault
 *injection command
 * Value  : Is a comma separated list of values defined by their position as
 *below
 *        : "fi_opcode,fi_cmd,fi_tag,fi_param1,fi_param2"
 * fi_opcode: Fault Operation code (enable/disable/test)
 *            enable: enable the specified FP
 *            disable: disable the specified FP
 *            test: dump in s3sesrver DEBUG log current state of FP
 * fi_cmd   : Fault command to execute against FP
 *            for fi_opcode:enable possible command list below. (Refer
 *s3_fi_common.h for triggers)
 *            once: trigger s3_fi_enable_once for given FP
 *            always: trigger s3_fi_enable for given FP
 *            random: trigger s3_fi_enable_random for given FP
 *            enablen: tigger s3_fi_enable_each_nth_time
 *            offnonm: trigger s3_fi_enable_off_n_on_m
 *            For fi_opcode:disable fi_cmd is ignored and can be specified
 *empty.
 * fi_tag   : Unique fault injection TAG string as encoded at point of failure.
 * fi_param1: First parameter value to pass with fi_cmd enable where applicable.
 *(Refer s3_fi_common.h)
 * fi_param2: Second parameter value to pass with fi_cmd enable where
 *applicable. (Refer s3_fi_common.h)
 *
 * curl examples:
 * curl  --header "x-seagate-faultinjection: enable,always,put_obj_mdsave_fail"
 *-X PUT http://s3.seagate.com
 * curl  --header "x-seagate-faultinjection: disbale,noop,put_obj_mdsave_fail"
 *-X PUT http://s3.seagate.com
 * curl  --header "x-seagate-faultinjection:
 *enable,enablen,put_obj_mdsave_fail,5" -X PUT http://s3.seagate.com
 */

class S3PutFiAction : public S3Action {
  // Unique fault tag
  std::string fi_opcode;
  std::string fi_cmd;
  std::string fi_tag;
  std::string fi_param1;
  std::string fi_param2;
  std::string invalid_string;
  std::string return_error;
  // Helpers
  void parse_command();

 public:
  S3PutFiAction(std::shared_ptr<S3RequestObject> req);

  void setup_steps();
  void set_fault_injection();
  void send_response_to_s3_client();

  // For Testing purpose
  FRIEND_TEST(S3PutFiActionTest, Constructor);
  FRIEND_TEST(S3PutFiActionTest, SetFaultInjectionCmdEmpty);
  FRIEND_TEST(S3PutFiActionTest, SetFaultInjectionTooManyParam);
  FRIEND_TEST(S3PutFiActionTest, SetFaultInjectionEnableAlways);
  FRIEND_TEST(S3PutFiActionTest, SetFaultInjectionEnableOnce);
  FRIEND_TEST(S3PutFiActionTest, SetFaultInjectionEnableRandom);
  FRIEND_TEST(S3PutFiActionTest, SetFaultInjectionEnableNTime);
  FRIEND_TEST(S3PutFiActionTest, SetFaultInjectionEnableOffN);
  FRIEND_TEST(S3PutFiActionTest, SetFaultInjectionDisable);
  FRIEND_TEST(S3PutFiActionTest, SetFaultInjectionTest);
  FRIEND_TEST(S3PutFiActionTest, SendResponseToClientSuccess);
  FRIEND_TEST(S3PutFiActionTest, SendResponseToClientMalformedFICmd);
};

#endif
