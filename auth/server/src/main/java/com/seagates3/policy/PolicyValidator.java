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
import com.amazonaws.auth.policy.Statement.Effect;
import com.amazonaws.auth.policy.conditions.ArnCondition;
import com.amazonaws.auth.policy.conditions.DateCondition;
import com.amazonaws.auth.policy.conditions.IpAddressCondition;
import com.amazonaws.auth.policy.conditions.NumericCondition;
import com.amazonaws.auth.policy.conditions.StringCondition;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import com.seagates3.dao.ldap.AccountImpl;
import com.seagates3.dao.ldap.UserImpl;
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Account;
import com.seagates3.model.User;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.BucketPolicyResponseGenerator;
import com.seagates3.util.PolicyUtil;
import com.seagates3.util.BinaryUtil;

public
class PolicyValidator {

 private
  final Logger LOGGER =
      LoggerFactory.getLogger(PolicyValidator.class.getName());
 private
  BucketPolicyResponseGenerator responseGenerator = null;

 public
  PolicyValidator() {
    responseGenerator = new BucketPolicyResponseGenerator();
    try {
      ActionsInitializer.init();
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

 public
  ServerResponse validateBucketPolicy(String inputBucket, String jsonPolicy) {

    ServerResponse response = null;
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

 private
  ServerResponse validatePrincipal(List<Principal> principals) {
    ServerResponse response = null;
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

 private
  boolean isPrincipalIdValid(Principal principal) {
    boolean isValid = true;
    String provider = principal.getProvider();
    String id = principal.getId();
    AccountImpl accountImpl = new AccountImpl();
    UserImpl userImpl = new UserImpl();
    Account account = null;
    switch (provider) {
      case "AWS":
        account = null;
        User user = null;
        try {
          if (PolicyUtil.isArnFormatValid(id)) {
            user = userImpl.findByArn(id);
            if (user == null || !user.exists()) {
              isValid = false;
            }
          } else {
            account = accountImpl.findByID(id);
            if (account == null || !account.exists()) {
              user = userImpl.findByUserId(id);
              if (user == null || !user.exists()) {
                isValid = false;
              }
            }
          }
        }
        catch (DataAccessException e) {
          LOGGER.error("Exception occurred while finding account by account id",
                       e);
          isValid = false;
        }
        break;
      case "CanonicalUser":
        account = null;
        try {
          account = accountImpl.findByCanonicalID(id);
        }
        catch (DataAccessException e) {
          LOGGER.error(
              "Exception occurred while finding account by canonical id", e);
        }
        if (account == null || !account.exists()) isValid = false;
        break;
      case "Federated":
        // Not supporting as of now
        isValid = false;
        break;
    }
    return isValid;
  }

 private
  ServerResponse validateResource(List<Resource> resourceValues,
                                  String inputBucket,
                                  List<String> actionsList) {
    ServerResponse response = null;
    if (resourceValues != null && !resourceValues.isEmpty()) {
      for (Resource resource : resourceValues) {
        String resourceArn = resource.getId();
        if (PolicyUtil.isArnFormatValid(resourceArn)) {
          String resourceName =
              PolicyUtil.getBucketFromResourceArn(resourceArn);
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

 private
  ServerResponse validateEffect(Effect effectValue) {
    ServerResponse response = null;
    if (effectValue != null) {
      if (Statement.Effect.valueOf(effectValue.name()) == null) {
        response = responseGenerator.malformedPolicy("Invalid effect : " +
                                                     effectValue.name());
        LOGGER.error("Effect value is invalid in bucket policy");
      }
    } else {
      response =
          responseGenerator.malformedPolicy("Missing required field Effect");
      LOGGER.error("Missing required field Effect");
    }
    return response;
  }

 private
  ServerResponse validateCondition(List<Condition> conditionList) {
    ServerResponse response = null;
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

 private
  ServerResponse validateActionAndResource(List<Action> actionList,
                                           List<Resource> resourceValues,
                                           String inputBucket) {
    ServerResponse response = null;
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
}
