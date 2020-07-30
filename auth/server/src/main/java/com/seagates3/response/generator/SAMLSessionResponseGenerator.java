package com.seagates3.response.generator;

import com.seagates3.authserver.AuthServerConfig;
import com.seagates3.model.Requestor;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.formatter.xml.SAMLSessionResponseFormatter;
import java.util.LinkedHashMap;

public class SAMLSessionResponseGenerator extends AbstractResponseGenerator {

    public ServerResponse generateCreateResponse(Requestor requestor) {
        LinkedHashMap credElements = new LinkedHashMap();
        credElements.put("AccessKeyId", requestor.getAccesskey().getId());
        credElements.put("SessionToken", requestor.getAccesskey().getToken());
        credElements.put("SecretAccessKey", requestor.getAccesskey()
                .getSecretKey());

        LinkedHashMap userDetailsElements = new LinkedHashMap();
        userDetailsElements.put("UserId", requestor.getId());
        userDetailsElements.put("UserName", requestor.getName());
        userDetailsElements.put("AccountId", requestor.getAccount().getId());
        userDetailsElements.put("AccountName", requestor.getAccount().getName());

        return new SAMLSessionResponseFormatter().formatCreateReponse(
            credElements, userDetailsElements, AuthServerConfig.getReqId());
    }

}
