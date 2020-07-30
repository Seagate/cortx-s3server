package com.seagates3.response.generator;

import com.seagates3.authserver.AuthServerConfig;
import com.seagates3.model.AccessKey;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.formatter.xml.AccessKeyResponseFormatter;
import io.netty.handler.codec.http.HttpResponseStatus;
import java.util.ArrayList;
import java.util.LinkedHashMap;

public class AccessKeyResponseGenerator extends AbstractResponseGenerator {

    public ServerResponse generateCreateResponse(String userName,
            AccessKey accessKey) {
        LinkedHashMap responseElements = new LinkedHashMap();
        responseElements.put("UserName", userName);
        responseElements.put("AccessKeyId", accessKey.getId());
        responseElements.put("Status", accessKey.getStatus());
        responseElements.put("SecretAccessKey", accessKey.getSecretKey());

        return new AccessKeyResponseFormatter().formatCreateResponse(
            "CreateAccessKey", "AccessKey", responseElements,
            AuthServerConfig.getReqId());
    }

    public ServerResponse generateDeleteResponse() {
        return new AccessKeyResponseFormatter().formatDeleteResponse("DeleteAccessKey");
    }

    public ServerResponse generateListResponse(String userName,
            AccessKey[] accessKeyList) {
        ArrayList<LinkedHashMap<String, String>> accessKeyMembers = new ArrayList<>();
        LinkedHashMap responseElements;

        for (AccessKey accessKey : accessKeyList) {
            responseElements = new LinkedHashMap();
            responseElements.put("UserName", userName);
            responseElements.put("AccessKeyId", accessKey.getId());
            responseElements.put("Status", accessKey.getStatus());
            responseElements.put("CreateDate", accessKey.getCreateDate());

            accessKeyMembers.add(responseElements);
        }

        return new AccessKeyResponseFormatter().formatListResponse(
            userName, accessKeyMembers, false, AuthServerConfig.getReqId());
    }

    public ServerResponse generateUpdateResponse() {
        return new AccessKeyResponseFormatter().formatUpdateResponse("UpdateAccessKey");
    }

    public ServerResponse noSuchEntity(String resource) {
        String errorMessage = "The request was rejected because it referenced "
                + resource + " that does not exist.";

        return formatResponse(HttpResponseStatus.NOT_FOUND, "NoSuchEntity",
                errorMessage);
    }

    public ServerResponse accessKeyQuotaExceeded() {
        String errorMessage = "The request was rejected because the number of "
                + "access keys allowed for the user has exceeded quota.";

        return formatResponse(HttpResponseStatus.BAD_REQUEST, "AccessKeyQuotaExceeded",
                errorMessage);
    }
}
