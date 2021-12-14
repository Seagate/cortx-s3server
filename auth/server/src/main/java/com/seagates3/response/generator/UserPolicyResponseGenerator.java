package com.seagates3.response.generator;

import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;

import com.seagates3.authserver.AuthServerConfig;
import com.seagates3.model.Policy;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.formatter.xml.XMLResponseFormatter;

import io.netty.handler.codec.http.HttpResponseStatus;

public
class UserPolicyResponseGenerator extends AbstractResponseGenerator {

 public
  ServerResponse generateAttachUserPolicyResponse() {
    return new XMLResponseFormatter().formatAttachUserPolicyResponse(
        "AttachUserPolicy");
  }

 public
  ServerResponse generateDetachUserPolicyResponse() {
    return new XMLResponseFormatter().formatDetachUserPolicyResponse(
        "DetachUserPolicy");
  }

 public
  ServerResponse userPolicyAttachQuotaViolation() {
    String errorMessage =
        "Number of policies that can be attached exceed the limit of - " +
        AuthServerConfig.USER_POLICY_ATTACH_QUOTA;
    return invalidRequest(errorMessage);
  }

 public
  ServerResponse userPolicyAlreadyAttached() {
    String errorMessage = "The request was rejected because it attempted " +
                          "to attach already attached policy to the user.";

    return formatResponse(HttpResponseStatus.CONFLICT, "EntityAlreadyExists",
                          errorMessage);
  }

 public
  ServerResponse attachNonAttachablePolicy() {
    String errorMessage = "The request was rejected because it attempted " +
                          "to attach non attachable policy to the user.";

    return formatResponse(HttpResponseStatus.BAD_REQUEST,
                          "InvalidParameterValue", errorMessage);
  }

 public
  ServerResponse detachNonAttachedPolicy() {
    String errorMessage =
        "The request was rejected because it attempted " +
        "to detach a policy that is not attached to the user.";

    return formatResponse(HttpResponseStatus.NOT_FOUND, "NoSuchEntity",
                          errorMessage);
  }

 public
  ServerResponse generateAttachedUserPolicyListResponse(
      List<Policy> policyList) {
    ArrayList<LinkedHashMap<String, String>> policyMembers = new ArrayList<>();
    LinkedHashMap responseElements;
    for (Policy policy : policyList) {
      responseElements = new LinkedHashMap();
      responseElements.put("PolicyName", policy.getName());
      responseElements.put("PolicyArn", policy.getARN());
      policyMembers.add(responseElements);
    }
    return new XMLResponseFormatter().formatListResponse(
        "ListAttachedUserPolicies", "AttachedPolicies", policyMembers, false,
        AuthServerConfig.getReqId());
  }
}