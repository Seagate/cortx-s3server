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
import com.seagates3.model.Role;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.formatter.xml.XMLResponseFormatter;
import java.util.ArrayList;
import java.util.LinkedHashMap;

public class RoleResponseGenerator extends AbstractResponseGenerator {

    public ServerResponse generateCreateResponse(Role role) {
        LinkedHashMap responseElements = new LinkedHashMap();
        responseElements.put("Path", role.getPath());
        responseElements.put("Arn", role.getARN());
        responseElements.put("RoleName", role.getName());
        responseElements.put("AssumeRolePolicyDocument", role.getRolePolicyDoc());
        responseElements.put("CreateDate", role.getCreateDate());
        responseElements.put("RoleId", role.getRoleId());

        return new XMLResponseFormatter().formatCreateResponse(
            "CreateRole", "Role", responseElements,
            AuthServerConfig.getReqId());
    }

    public ServerResponse generateDeleteResponse() {
        return new XMLResponseFormatter().formatDeleteResponse("DeleteRole");
    }

    public ServerResponse generateListResponse(Role[] roleList) {
        ArrayList<LinkedHashMap<String, String>> roleMemebers = new ArrayList<>();
        LinkedHashMap responseElements;

        for (Role role : roleList) {
            responseElements = new LinkedHashMap();
            responseElements.put("Path", role.getPath());

            String arn = String.format("arn:seagate:iam::%s:%s",
                    role.getAccount().getName(), role.getName());
            responseElements.put("Arn", arn);

            responseElements.put("RoleName", role.getName());
            responseElements.put("AssumeRolePolicyDocument", role.getRolePolicyDoc());
            responseElements.put("CreateDate", role.getCreateDate());
            responseElements.put("RoleId", role.getName());

            roleMemebers.add(responseElements);
        }

        return (ServerResponse) new XMLResponseFormatter().formatListResponse(
            "ListRoles", "Roles", roleMemebers, false,
            AuthServerConfig.getReqId());
    }
}
