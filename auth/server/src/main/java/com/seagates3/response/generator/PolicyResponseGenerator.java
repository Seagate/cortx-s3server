package com.seagates3.response.generator;

import com.seagates3.authserver.AuthServerConfig;
import com.seagates3.model.Policy;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.formatter.xml.XMLResponseFormatter;

import io.netty.handler.codec.http.HttpResponseStatus;

import java.util.LinkedHashMap;

public class PolicyResponseGenerator extends AbstractResponseGenerator {

    public ServerResponse generateCreateResponse(Policy policy) {
       LinkedHashMap<String, String> responseElements =
           new LinkedHashMap<String, String>();
        responseElements.put("PolicyName", policy.getName());
        responseElements.put("DefaultVersionId", policy.getDefaultVersionid());
        responseElements.put("PolicyId", policy.getPolicyId());
        responseElements.put("Path", policy.getPath());
        responseElements.put("Arn", policy.getARN());
        responseElements.put("AttachmentCount",
                String.valueOf(policy.getAttachmentCount()));
        responseElements.put("CreateDate", policy.getCreateDate());
        responseElements.put("UpdateDate", policy.getUpdateDate());

        return new XMLResponseFormatter().formatCreateResponse(
            "CreatePolicy", "Policy", responseElements,
            AuthServerConfig.getReqId());
    }

   public
    ServerResponse malformedPolicy(String errorMessage) {
      return formatResponse(HttpResponseStatus.BAD_REQUEST, "MalformedPolicy",
                            errorMessage);
    }

   public
    ServerResponse noSuchPolicy() {
      String errorMessage = "The specified policy does not exist.";
      return formatResponse(HttpResponseStatus.NOT_FOUND, "NoSuchPolicy",
                            errorMessage);
    }

   public
    ServerResponse invalidPolicyDocument() {
      String errorMessage =
          "The content of the form does not meet the conditions specified in " +
          "the " + "policy document.";
      return formatResponse(HttpResponseStatus.BAD_REQUEST,
                            "InvalidPolicyDocument", errorMessage);
    }
}
