package com.seagates3.response.generator;

import java.util.LinkedHashMap;

import com.seagates3.model.AccessKey;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.formatter.xml.AccessKeyResponseFormatter;

public
class TempAuthCredentialsResponseGenerator extends AbstractResponseGenerator {

 public
  ServerResponse generateCreateResponse(String userName, AccessKey accessKey) {
    LinkedHashMap<String, String> responseElements = new LinkedHashMap<>();
    responseElements.put("UserName", userName);
    responseElements.put("AccessKeyId", accessKey.getId());
    responseElements.put("Status", accessKey.getStatus());
    responseElements.put("SecretAccessKey", accessKey.getSecretKey());
    responseElements.put("ExpiryTime", accessKey.getExpiry());
    responseElements.put("SessionToken", accessKey.getToken());

    return new AccessKeyResponseFormatter().formatCreateResponse(
        "GetTempAuthCredentials", "AccessKey", responseElements, "0000");
  }
}
