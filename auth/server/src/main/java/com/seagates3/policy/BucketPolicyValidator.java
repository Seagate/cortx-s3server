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
 * Original creation date: 23-October-2019
 */

package com.seagates3.policy;

import java.io.UnsupportedEncodingException;
import java.util.ArrayList;
import java.util.List;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.amazonaws.auth.policy.Action;
import com.amazonaws.auth.policy.Condition;
import com.amazonaws.auth.policy.Policy;
import com.amazonaws.auth.policy.Principal;
import com.amazonaws.auth.policy.Resource;
import com.amazonaws.auth.policy.Statement;
import com.amazonaws.auth.policy.conditions.ArnCondition;
import com.amazonaws.auth.policy.conditions.DateCondition;
import com.amazonaws.auth.policy.conditions.IpAddressCondition;
import com.amazonaws.auth.policy.conditions.NumericCondition;
import com.amazonaws.auth.policy.conditions.StringCondition;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.BucketPolicyResponseGenerator;
import com.seagates3.util.BinaryUtil;

public
class BucketPolicyValidator extends PolicyValidator {

 private
  final Logger LOGGER =
      LoggerFactory.getLogger(BucketPolicyValidator.class.getName());

 private
  ServerResponse response = null;

 public
  BucketPolicyValidator() {
    responseGenerator = new BucketPolicyResponseGenerator();
    try {
      ActionsInitializer.init(PolicyUtil.Services.S3);
    }
    catch (UnsupportedEncodingException e) {
      LOGGER.error("Exception during Ppolicy Validator initializer");
    }
  }

  /**
   * Below method will validate bucket policy json passed in
   * @param jsonPolicy
   * @return null if policy validated successfully. not null if policy
   *         invalidated.
   */
  @Override public ServerResponse validatePolicy(String inputBucket,
                                                 String jsonPolicy) {

    JsonParser jsonParser = new JsonParser();
    JsonObject obj =
        (JsonObject)jsonParser.parse(BinaryUtil.base64DecodeString(jsonPolicy));
    Policy policy = null;
    try {
      policy = Policy.fromJson(obj.toString());
    }
    catch (Exception e) {
      response = responseGenerator.malformedPolicy(
          "This policy contains invalid Json");
      LOGGER.error("This policy contains invalid Json");
      return response;
    }
    // Statement validation
    if (policy.getStatements() != null) {
      List<Statement> statementList =
          new ArrayList<Statement>(policy.getStatements());
      for (Statement stmt : statementList) {
        // Action validation
        response = validateActionAndResource(stmt.getActions(),
                                             stmt.getResources(), inputBucket);
        if (response != null) break;

        // Effect validation
        response = validateEffect(stmt.getEffect());
        if (response != null) break;

        // condition validation
        response = validateCondition(stmt.getConditions());
        if (response != null) break;

        // Principal Validation
        response = validatePrincipal(stmt.getPrincipals());
        if (response != null) break;
      }
    } else {  // statement null
      response =
          responseGenerator.malformedPolicy("Missing required field Statement");
      LOGGER.error("Missing required field Statement");
    }
    return response;
  }

  @Override public ServerResponse validatePrincipal(
      List<Principal> principals) {
    if (principals != null && !principals.isEmpty()) {
      for (Principal principal : principals) {
        if (!"*".equals(principal.getId()) && !isPrincipalIdValid(principal)) {
          response =
              responseGenerator.malformedPolicy("Invalid principal in policy");
          LOGGER.error("Invalid principal in policy");
          break;
        }
      }
    } else {
      response =
          responseGenerator.malformedPolicy("Missing required field Principal");
      LOGGER.error("Missing required field Principal");
    }
    return response;
  }

  @Override public ServerResponse validateActionAndResource(
      List<Action> actionList, List<Resource> resourceValues,
      String inputBucket) {
    List<String> matchingActionsList = null;
    if (actionList != null && !actionList.isEmpty()) {
      for (Action action : actionList) {
        if (action != null) {
          matchingActionsList =
              PolicyUtil.getAllMatchingActions(action.getActionName());
        }
        if (matchingActionsList == null || matchingActionsList.isEmpty()) {
          response =
              responseGenerator.malformedPolicy("Policy has invalid action");
          LOGGER.error("Policy has invalid action");
          break;
        } else {
          response = validateResource(resourceValues, inputBucket,
                                      matchingActionsList);
          if (response != null) {
            break;
          }
        }
      }

    } else {
      response =
          responseGenerator.malformedPolicy("Missing required field Action");
      LOGGER.error("Missing required field Action");
    }
    return response;
  }

 private
  ServerResponse validateResource(List<Resource> resourceValues,
                                  String inputBucket,
                                  List<String> actionsList) {
    if (resourceValues != null && !resourceValues.isEmpty()) {
      for (Resource resource : resourceValues) {
        String resourceArn = resource.getId();
        if (new S3ArnParser().isArnFormatValid(resourceArn)) {
          String resourceName =
              PolicyUtil.getResourceFromResourceArn(resourceArn);
          int slashPosition = resourceName.indexOf('/');
          if (slashPosition != -1) {
            resourceName = resourceName.substring(0, slashPosition);
          }
          // Just check for bucket name match. Input bucket and bucket inside
          // policy json must be same
          if (!resourceName.equals(inputBucket)) {
            // input bucket not valid
            response = responseGenerator.malformedPolicy(
                "Policy has invalid resource");
            LOGGER.error("Bucket name entered not valid");
            break;
          } else {
            // check for resource and action combination validity
            if (!PolicyUtil.isActionValidForResource(actionsList,
                                                     slashPosition)) {
              response = responseGenerator.malformedPolicy(
                  "Action does not apply to any resource(s) in statement");
              LOGGER.error("Action and resource combination invalid");
              break;
            }
          }
        } else {  // arn format not valid
          response =
              responseGenerator.malformedPolicy("Policy has invalid resource");
          LOGGER.error("Resource ARN format not valid");
          break;
        }
      }
    } else {
      response =
          responseGenerator.malformedPolicy("Missing required field Resource");
      LOGGER.error("Missing required field Resource");
    }
    return response;
  }

  @Override public ServerResponse validateCondition(
      List<Condition> conditionList) {
    if (conditionList != null) {
      for (Condition condition : conditionList) {
        String invalidConditionType =
            checkAndGetInvalidConditionType(condition);
        if (invalidConditionType != null) {
          response = responseGenerator.malformedPolicy(
              "Invalid Condition type : " + invalidConditionType);
          LOGGER.error("Condition type is invalid in bucket policy");
          break;
        }
      }
    }
    return response;
  }

 private
  String checkAndGetInvalidConditionType(Condition condition) {
    String conditionType = condition.getType();
    if (ArnCondition.ArnComparisonType.valueOf(conditionType) != null) {
      conditionType = null;
    } else if (StringCondition.StringComparisonType.valueOf(conditionType) !=
               null) {
      conditionType = null;
    } else if (NumericCondition.NumericComparisonType.valueOf(conditionType) !=
               null) {
      conditionType = null;
    } else if (DateCondition.DateComparisonType.valueOf(conditionType) !=
               null) {
      conditionType = null;
    } else if (IpAddressCondition.IpAddressComparisonType.valueOf(
                   conditionType) != null) {
      conditionType = null;
    }
    return conditionType;
  }
}
