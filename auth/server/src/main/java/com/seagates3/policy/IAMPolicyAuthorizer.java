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

package com.seagates3.policy;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import org.json.JSONException;
import org.json.JSONObject;
import com.seagates3.model.User;
import com.amazonaws.auth.policy.Action;
import com.amazonaws.auth.policy.Statement;
import com.amazonaws.auth.policy.Statement.Effect;
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Requestor;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.AuthorizationResponseGenerator;
import com.seagates3.constants.APIRequestParamsConstants;
import io.netty.handler.codec.http.HttpResponseStatus;

public
class IAMPolicyAuthorizer extends PolicyAuthorizer {

  @Override public ServerResponse authorizePolicy(
      Requestor requestor, Map<String, String> requestBody) {

    ServerResponse serverResponse = null;

    // authorizePolicy will return NULL if no relevant entry found in policy
    // authorized if match is found
    // AccessDenied if Deny found
    String requestedOperation = null;
    try {
      if (requestBody.get(APIRequestParamsConstants.ACTION) != null &&
          !requestBody.get(APIRequestParamsConstants.ACTION)
               .contains(APIRequestParamsConstants.AUTHORIZE)) {
        requestedOperation = APIRequestParamsConstants.IAM_PREFIX +
                             requestBody.get(APIRequestParamsConstants.ACTION);
      } else {
        requestedOperation = identifyOperationToAuthorize(
            requestBody.get(APIRequestParamsConstants.S3_ACTION));
      }
      LOGGER.debug("operation to authorize - " + requestedOperation);
      if (requestedOperation != null && requestor != null &&
          requestor.getPolicyDocuments() != null) {
        for (String policyToAuthorize : requestor.getPolicyDocuments()) {
          // authorize policy
          serverResponse =
              authorizeOperation(requestBody, requestedOperation.toLowerCase(),
                                 policyToAuthorize, requestor.getUser());
          if (serverResponse != null &&
              serverResponse.getResponseStatus() != HttpResponseStatus.OK) {
            break;
          }
        }
      }
    }
    catch (Exception e) {
      LOGGER.error("Exception while authorizing - ", e);
    }
    LOGGER.debug("IAM policy authorization response - " + serverResponse);
    return serverResponse;
  }

  /**
   * Below will authorize requested operation based on Resource, and Action
   * present inside existing iam policy
   */
 private
  ServerResponse authorizeOperation(Map<String, String> requestBody,
                                    String requestedOperation,
                                    String policyJson,
                                    User user) throws DataAccessException,
      JSONException {

    ServerResponse response = null;
    AuthorizationResponseGenerator responseGenerator =
        new AuthorizationResponseGenerator();
    JSONObject obj = new JSONObject(policyJson);
    String policyString = obj.toString();
    com.amazonaws.auth.policy.Policy existingPolicy =
        com.amazonaws.auth.policy.Policy.fromJson(policyString);
    String requestedResource = PolicyUtil.getResourceFromUri(
        requestBody.get(APIRequestParamsConstants.CLIENT_ABSOLUTE_URI));
    List<Statement> statementList =
        new ArrayList<Statement>(existingPolicy.getStatements());
    for (Statement stmt : statementList) {
      List<String> resourceList = PolicyUtil.convertCommaSeparatedStringToList(
          stmt.getResources().get(0).getId());
      if (isResourceMatching(resourceList, requestedResource) ||
          isIamResourceMatching(
              resourceList,
              requestBody.get(APIRequestParamsConstants.POLICYARN),
              requestBody.get(APIRequestParamsConstants.USER_NAME), user)) {
        List<Action> actionsList = stmt.getActions();
        if (isIamActionMatching(actionsList, requestedOperation)) {
          if (stmt.getEffect().equals(Effect.Allow)) {
            response = responseGenerator.ok();
          } else {
            response = responseGenerator.AccessDenied();
            break;
          }
        }
      }
    }
    return response;
  }

 protected
  boolean isIamActionMatching(List<Action> actionsList,
                              String requestedOperation) {
    boolean isMatching = false;
    for (Action action : actionsList) {
      if (PolicyUtil.isPatternMatching(requestedOperation.toLowerCase(),
                                       action.getActionName().toLowerCase())) {
        isMatching = true;
        break;
      }
    }
    LOGGER.debug("isIamActionMatching:: result - " +
                 String.valueOf(isMatching));
    return isMatching;
  }

  /**
           * Below will match policyARN and userARN with resource in policy
           * will return true if either of two matches
           * @param resourceList
           * @param policyArn
           * @param userArn
           * @return
           */
 protected
  boolean isIamResourceMatching(List<String> resourceList, String policyArn,
                                String userName, User user) {
    boolean isMatching = false;
    for (String resourceArn : resourceList) {
      if (userName == null) {
        isMatching = PolicyUtil.isPatternMatching(user.getArn(), resourceArn);
      } else {
        isMatching = PolicyUtil.isPatternMatching(
            "user/" + userName,
            PolicyUtil.getResourceFromResourceArn(resourceArn));
      }

      if (!isMatching) {
        isMatching = PolicyUtil.isPatternMatching(policyArn, resourceArn);
      }
    }
    LOGGER.debug("isIamResourceMatching:: result - " +
                 String.valueOf(isMatching));
    return isMatching;
  }
}

