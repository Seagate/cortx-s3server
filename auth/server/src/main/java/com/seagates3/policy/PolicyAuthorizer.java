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

import java.util.Map;
import java.util.List;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.seagates3.model.Requestor;
import com.seagates3.response.ServerResponse;
import com.amazonaws.auth.policy.Action;
import com.amazonaws.auth.policy.Condition;

public
abstract class PolicyAuthorizer extends PolicyAuthorizerConstants {

 protected
  final Logger LOGGER =
      LoggerFactory.getLogger(PolicyAuthorizer.class.getName());

 public
  abstract ServerResponse
      authorizePolicy(Requestor requestor, Map<String, String> requestBody);
  /**
          * Below method will identify requested operation by user
          */
 protected
  String identifyOperationToAuthorize(String s3Action) {
    String action = null;
    if (null != s3Action) {
      switch (s3Action) {
        case "HeadBucket":
          action = "ListBucket";
          break;
        case "HeadObject":
          action = "GetObject";
          break;
        case "DeleteBucketTagging":
          action = "PutBucketTagging";
          break;
        default:
          action = s3Action;
          break;
      }
      action = "s3:" + action;
      LOGGER.debug("identifyOperationToAuthorize has returned action as - " +
                   action);
    }
    return action;
  }

  /**
   * Below will validate requested resource against resource inside policy
   *
   * @param resourceList
   * @param requestedResource
   * @return
   */
 protected
  boolean isResourceMatching(List<String> resourceList,
                             String requestedResource) {
    boolean isMatching = false;
    if (requestedResource != null && !requestedResource.isEmpty()) {
      for (String resourceArn : resourceList) {
        String resource = PolicyUtil.getResourceFromResourceArn(resourceArn);
        if (PolicyUtil.isPatternMatching(requestedResource, resource)) {
          isMatching = true;
          break;
        }
      }
    }
    LOGGER.debug("isResourceMatching:: result - " + String.valueOf(isMatching));
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
 protected
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
    LOGGER.debug("isActionMatching:: result - " + String.valueOf(isMatching));
    return isMatching;
  }

  /**
   * Checks if the Conditions from policy are satisfied in the request. Returns
   * true if there are no conditions in policy.
   *
   * @param conditions
   * @param requestBody
   * @return - true if policy conditions are satisfied
   */
 protected
  boolean isConditionMatching(List<Condition> conditions,
                              Map<String, String> requestBody) {
    if (conditions.isEmpty()) return true;

    /**
     * Check if the headers from requestBody satisfy the policy conditions
     * Sample
     * Condition - "Condition": { "StringEquals": { "s3:x-amz-acl":
     * ["bucket-owner-read", "bucket-owner-full-control"] } }
     */
    boolean result = false;
    for (Condition condition : conditions) {
      PolicyCondition pc = ConditionFactory.getCondition(
          condition.getType(),
          ConditionUtil.removeKeyPrefix(condition.getConditionKey()),
          condition.getValues());
      if (pc != null)
        result = pc.isSatisfied(requestBody);
      else
        result = false;

      if (!result) break;
    }
    LOGGER.debug("isConditionMatching:: result - " + String.valueOf(result));
    return result;
  }
}
