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

import com.seagates3.authserver.AuthServerConfig;
import com.seagates3.model.User;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.formatter.xml.XMLResponseFormatter;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import com.seagates3.authorization.Authorizer;

public class UserResponseGenerator extends AbstractResponseGenerator {
    public ServerResponse generateCreateResponse(User user) {
        LinkedHashMap responseElements = new LinkedHashMap();
        responseElements.put("Path", user.getPath());
        responseElements.put("UserName", user.getName());
        responseElements.put("UserId", user.getId());
        responseElements.put("Arn", user.getArn());

        return new XMLResponseFormatter().formatCreateResponse(
            "CreateUser", "User", responseElements,
            AuthServerConfig.getReqId());
    }

    public ServerResponse generateDeleteResponse() {
        return new XMLResponseFormatter().formatDeleteResponse("DeleteUser");
    }

    public ServerResponse generateListResponse(Object[] responseObjects) {
        User[] userList = (User[]) responseObjects;

        ArrayList<LinkedHashMap<String, String>> userMemebers = new ArrayList<>();
        LinkedHashMap responseElements;
        for (User user : userList) {
          if (!Authorizer.isRootUser(user)) {
            responseElements = new LinkedHashMap();
            responseElements.put("UserId", user.getId());
            responseElements.put("Path", user.getPath());
            responseElements.put("UserName", user.getName());
            responseElements.put("Arn", user.getArn());
            responseElements.put("CreateDate", user.getCreateDate());
            // responseElements.put("PasswordLastUsed", "");
            userMemebers.add(responseElements);
          }
        }
        return new XMLResponseFormatter().formatListResponse(
            "ListUsers", "Users", userMemebers, false,
            AuthServerConfig.getReqId());
    }

    public ServerResponse generateUpdateResponse() {
        return new XMLResponseFormatter().formatUpdateResponse("UpdateUser");
    }
}
