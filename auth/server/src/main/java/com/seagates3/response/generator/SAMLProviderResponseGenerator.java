package com.seagates3.response.generator;

import com.seagates3.authserver.AuthServerConfig;
import com.seagates3.model.SAMLProvider;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.formatter.xml.SAMLProviderResponseFormatter;
import com.seagates3.response.formatter.xml.XMLResponseFormatter;
import java.util.ArrayList;
import java.util.LinkedHashMap;

public class SAMLProviderResponseGenerator extends AbstractResponseGenerator {

    public ServerResponse generateCreateResponse(SAMLProvider samlProvider) {
        LinkedHashMap responseElements = new LinkedHashMap();
        String arnValue = String.format("arn:seagate:iam::%s:%s",
                samlProvider.getAccount().getName(), samlProvider.getName());
        responseElements.put("Arn", arnValue);

        return new SAMLProviderResponseFormatter().formatCreateResponse(
            "CreateSAMLProvider", null, responseElements,
            AuthServerConfig.getReqId());
    }

    public ServerResponse generateDeleteResponse() {
        return new SAMLProviderResponseFormatter().formatDeleteResponse(
                "DeleteSAMLProvider");
    }

    public ServerResponse generateUpdateResponse(String name) {
       return new SAMLProviderResponseFormatter().formatUpdateResponse(
           name, AuthServerConfig.getReqId());
    }

    public ServerResponse generateListResponse(SAMLProvider[] samlProviderList) {
        ArrayList<LinkedHashMap<String, String>> providerMembers = new ArrayList<>();
        LinkedHashMap responseElements;

        for (SAMLProvider provider : samlProviderList) {
            responseElements = new LinkedHashMap();
            String arn = String.format("arn:seagate:iam:::%s",
                    provider.getName());

            responseElements.put("Arn", arn);
            responseElements.put("ValidUntil", provider.getExpiry());
            responseElements.put("CreateDate", provider.getCreateDate());

            providerMembers.add(responseElements);
        }

        return new XMLResponseFormatter().formatListResponse(
            "ListSAMLProviders", "SAMLProviderList", providerMembers, false,
            AuthServerConfig.getReqId());
    }
}
