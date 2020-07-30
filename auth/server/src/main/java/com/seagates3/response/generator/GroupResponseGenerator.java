package com.seagates3.response.generator;

import com.seagates3.authserver.AuthServerConfig;
import com.seagates3.model.Group;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.formatter.xml.XMLResponseFormatter;
import java.util.LinkedHashMap;

public class GroupResponseGenerator extends AbstractResponseGenerator {

    public ServerResponse generateCreateResponse(Group group) {
        LinkedHashMap responseElements = new LinkedHashMap();
        responseElements.put("Path", group.getPath());
        responseElements.put("Arn", group.getARN());
        responseElements.put("GroupId", group.getGroupId());
        responseElements.put("GroupName", group.getName());
        responseElements.put("CreateDate", group.getCreateDate());

        return new XMLResponseFormatter().formatCreateResponse(
            "CreateGroup", "Group", responseElements,
            AuthServerConfig.getReqId());
    }

}
