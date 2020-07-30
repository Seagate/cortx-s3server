package com.seagates3.response.generator;

import com.seagates3.authserver.AuthServerConfig;
import com.seagates3.model.AccessKey;
import com.seagates3.model.SAMLResponseTokens;
import com.seagates3.model.User;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.formatter.xml.AssumeRoleWithSAMLResponseFormatter;
import java.util.LinkedHashMap;

public class AssumeRoleWithSAMLResponseGenerator
        extends SAMLResponseGenerator {

    public ServerResponse generateCreateResponse(User user, AccessKey accessKey,
            SAMLResponseTokens samlResponseToken) {
        LinkedHashMap<String, String> credentials = new LinkedHashMap<>();
        credentials.put("SessionToken", accessKey.getToken());
        credentials.put("SecretAccessKey", accessKey.getSecretKey());
        credentials.put("Expiration", accessKey.getExpiry());
        credentials.put("AccessKeyId", accessKey.getId());

        LinkedHashMap<String, String> federatedUser = new LinkedHashMap<>();

        String arnValue = String.format("arn:seagate:sts:::%s",
                user.getRoleName());
        federatedUser.put("Arn", arnValue);

        String assumedRoleIdVal = String.format("%s:%s", user.getId(),
                user.getRoleName());
        federatedUser.put("AssumedRoleId", assumedRoleIdVal);

        LinkedHashMap<String, String> samlAttributes = new LinkedHashMap<>();
        samlAttributes.put("Issuer", samlResponseToken.getIssuer());
        samlAttributes.put("Audience", samlResponseToken.getAudience());
        samlAttributes.put("Subject", samlResponseToken.getsubject());
        samlAttributes.put("SubjectType", samlResponseToken.getsubjectType());

        /**
         * TODO - generate Name qualifier
         */
        samlAttributes.put("NameQualifier", "S4jYAZtpWsPHGsy1j5sKtEKOyW4=");

        return new AssumeRoleWithSAMLResponseFormatter().formatCreateResponse(
            credentials, federatedUser, samlAttributes, "6",
            AuthServerConfig.getReqId());
    }
}
