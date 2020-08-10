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

package com.seagates3.response.generator;

import java.util.ArrayList;
import java.util.LinkedHashMap;

import com.seagates3.authserver.AuthServerConfig;
import com.seagates3.model.User;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.formatter.xml.XMLResponseFormatter;

public
class UserLoginProfileResponseGenerator extends AbstractResponseGenerator {

 public
  ServerResponse generateCreateResponse(User user) {
    LinkedHashMap responseElements = new LinkedHashMap();
    responseElements.put("UserName", user.getName());
    responseElements.put("PasswordResetRequired",
                         user.getPwdResetRequired().toLowerCase());
    responseElements.put("CreateDate", user.getProfileCreateDate());
    return new XMLResponseFormatter().formatCreateResponse(
        "CreateLoginProfile", "LoginProfile", responseElements,
        AuthServerConfig.getReqId());
  }

  /**
   * Below method will generate 'GetLoginProfile' response
   *
   * @param user
   * @return LoginProfile (username and password) of the requested user
   */
 public
  ServerResponse generateGetResponse(User user) {
    LinkedHashMap<String, String> responseElements = new LinkedHashMap<>();
    ArrayList<LinkedHashMap<String, String>> userMembers = new ArrayList<>();
    responseElements.put("UserName", user.getName());
    responseElements.put("CreateDate", user.getProfileCreateDate());
    responseElements.put("PasswordResetRequired",
                         user.getPwdResetRequired().toLowerCase());
    userMembers.add(responseElements);
    return new XMLResponseFormatter().formatGetResponse(
        "GetLoginProfile", "LoginProfile", userMembers,
        AuthServerConfig.getReqId());
  }

  /**
   * Below method will generate 'UpdateLoginProfile' response
   *
   * @param user
   * @return
   */
 public
  ServerResponse generateUpdateResponse() {
    return new XMLResponseFormatter().formatUpdateResponse(
        "UpdateLoginProfile");
  }

  /**
  * Below method will generate 'ChangePassword' response
  * @return
  */
 public
  ServerResponse generateChangePasswordResponse() {
    return new XMLResponseFormatter().formatChangePasswordResponse(
        "ChangePassword");
  }
}
