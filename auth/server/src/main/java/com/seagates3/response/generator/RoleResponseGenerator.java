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
