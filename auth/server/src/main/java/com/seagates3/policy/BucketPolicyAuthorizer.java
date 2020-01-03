/*
 * COPYRIGHT 2019 SEAGATE LLC
 *
 * THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
 * HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE TECHNOLOGY
 * LIMITED, ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
 * THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
 * BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
 * USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
 * EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.
 *
 * YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
 * THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
 * http://www.seagate.com/contact
 *
 * Original author:  Shalaka Dharap
 * Original creation date: 13-December-2019
 */

package com.seagates3.policy;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import com.amazonaws.auth.policy.Action;
import com.amazonaws.auth.policy.Policy;
import com.amazonaws.auth.policy.Principal;
import com.amazonaws.auth.policy.Statement;
import com.amazonaws.auth.policy.Statement.Effect;
import com.amazonaws.util.json.JSONObject;
import com.seagates3.acl.AccessControlList;
import com.seagates3.authorization.Authorizer;
import com.seagates3.dao.ldap.AccountImpl;
import com.seagates3.dao.ldap.UserImpl;
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Account;
import com.seagates3.model.Requestor;
import com.seagates3.model.User;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.AuthorizationResponseGenerator;

import io.netty.handler.codec.http.HttpMethod;
import io.netty.handler.codec.http.HttpResponseStatus;

public
class BucketPolicyAuthorizer extends PolicyAuthorizer {

  @Override public ServerResponse authorizePolicy(
      Requestor requestor, Map<String, String> requestBody) {
    ServerResponse serverResponse = null;
    // authorizePolicy will return NULL if no relevant entry found in policy
    // authorized if match is found
    // AccessDenied if Deny found
    try {
      String requestedOperation = identifyOperationToAuthorize(requestBody);
      if (requestedOperation != null) {
        JSONObject obj = new JSONObject(requestBody.get("Policy"));
        Policy policy = Policy.fromJson(obj.toString());
        String requestedResource =
            PolicyUtil.getResourceFromUri(requestBody.get("ClientAbsoluteUri"));
        String bucketOwner = new AccessControlList().getOwner(requestBody);

        serverResponse =
            authorizeOperation(policy, requestedOperation, requestor,
                               requestedResource, bucketOwner);
      }
    }
    catch (Exception e) {
      LOGGER.error("Exception while authorizing", e);
    }
    return serverResponse;
  }

  /**
   * Below method will identify requested operation by user
   *
   * @param requestBody
   * @return
   */
  // TODO: Handle all possible operation combinations here
 private
  String identifyOperationToAuthorize(Map<String, String> requestBody) {
    String operation = null;

    HttpMethod httpMethod = HttpMethod.valueOf(requestBody.get("Method"));
    String clientQueryParams = requestBody.get("ClientQueryParams");

    if ("policy".equals(clientQueryParams)) {
      if (HttpMethod.PUT.equals(httpMethod)) {
        operation = PUT_BUCKET_POLICY;
      } else if (HttpMethod.GET.equals(httpMethod)) {
        operation = GET_BUCKET_POLICY;
      } else if (HttpMethod.DELETE.equals(httpMethod)) {
        operation = DELETE_BUCKET_POLICY;
      }
    }

    return operation;
  }

  /**
   * Below will authorize requested operation based on Resource,Principal and
   * Action present inside existing policy
   *
   * @param existingPolicy
   * @param requestedOperation
   * @param requestor
   * @param requestedResource
   * @param resourceOwner
   * @return
   * @throws DataAccessException
   */
 private
  ServerResponse authorizeOperation(
      Policy existingPolicy, String requestedOperation, Requestor requestor,
      String requestedResource,
      String resourceOwner) throws DataAccessException {
    ServerResponse response = null;
    AuthorizationResponseGenerator responseGenerator =
        new AuthorizationResponseGenerator();
    List<Statement> statementList =
        new ArrayList<Statement>(existingPolicy.getStatements());

    for (Statement stmt : statementList) {
      List<Principal> principalList = stmt.getPrincipals();
      if (isPrincipalMatching(principalList, requestor)) {
        List<String> resourceList =
            PolicyUtil.convertCommaSeparatedStringToList(
                stmt.getResources().get(0).getId());
        if (isResourceMatching(resourceList, requestedResource)) {
          List<Action> actionsList = stmt.getActions();
          if (isActionMatching(actionsList, requestedOperation)) {
            if (stmt.getEffect().equals(Effect.Allow)) {
              response = responseGenerator.ok();
            } else {
              response = responseGenerator.AccessDenied();
            }
          }
        }
      }
    }
    // Below will handle Get/Put/Delete Bucket Policy
    if (PolicyUtil.isPolicyOperation(requestedOperation)) {
      if (response != null &&
          response.getResponseStatus() == HttpResponseStatus.OK) {
        Account ownerAccount =
            new AccountImpl().findByCanonicalID(resourceOwner);
        if (!ownerAccount.getName().equals(requestor.getAccount().getName())) {
          response = responseGenerator.methodNotAllowed(
              "The specified method is not allowed against this resource.");
        }
      } else {
        boolean isRootUser = Authorizer.isRootUser(
            new UserImpl().findByUserId(requestor.getId()));
        if (isRootUser &&
            requestor.getAccount().getCanonicalId().equals(resourceOwner)) {
          response = responseGenerator.ok();
        } else {
          response = responseGenerator.AccessDenied();
        }
      }
    }
    return response;
  }

  /**
   * Below method will validate requested account against principal inside
   *policy
   *
   * @param principalList
   * @param requestor
   * @return
   * @throws DataAccessException
   */
 private
  boolean isPrincipalMatching(List<Principal> principalList,
                              Requestor requestor) throws DataAccessException {
    boolean isMatching = false;
    for (Principal principal : principalList) {
      String provider = principal.getProvider();
      String principalId = principal.getId();
      if ("*".equals(principalId)) {
        return true;
      }
      switch (provider) {
        case "AWS":
          if (new PrincipalArnParser().isArnFormatValid(principalId)) {
            User user = new UserImpl().findByArn(principalId);
            if (user != null && (user.getAccountName().equals(
                                    requestor.getAccount().getName())) &&
                (user.getId().equals(requestor.getId()))) {
              isMatching = true;
            }
          } else {
            if (principalId.equals(requestor.getAccount().getId())) {
              isMatching = true;
            }
          }
          break;
        case "CanonicalUser":
          if (principalId.equals(requestor.getAccount().getCanonicalId())) {
            isMatching = true;
          }
          break;
      }
      if (isMatching) {
        break;
      }
    }
    return isMatching;
  }

  /**
   * Below will validate requested resource against resource inside policy
   *
   * @param resourceList
   * @param requestedResource
   * @return
   */
 private
  boolean isResourceMatching(List<String> resourceList,
                             String requestedResource) {
    boolean isMatching = false;
    for (String resourceArn : resourceList) {
      String resource = PolicyUtil.getResourceFromResourceArn(resourceArn);
      if (PolicyUtil.isPatternMatching(requestedResource, resource)) {
        isMatching = true;
        break;
      }
    }
    return isMatching;
  }

  /**
   * Below will validate requested operation against the actions inside policy
   * including wildcard-chars
   *
   * @param actionsList
   * @param requestedOperation
   * @return
   */
 private
  boolean isActionMatching(List<Action> actionsList,
                           String requestedOperation) {
    boolean isMatching = false;
    for (Action action : actionsList) {
      List<String> matchingActionsList =
          PolicyUtil.getAllMatchingActions(action.getActionName());
      for (String matchingAction : matchingActionsList) {
        if (matchingAction.equals(requestedOperation)) {
          isMatching = true;
          break;
        }
      }
      if (isMatching) {
        break;
      }
    }
    return isMatching;
  }
}
