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

import java.util.Iterator;
import java.util.List;
import java.util.Set;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.amazonaws.auth.policy.Action;
import com.amazonaws.auth.policy.Condition;
import com.amazonaws.auth.policy.Principal;
import com.amazonaws.auth.policy.Resource;
import com.amazonaws.auth.policy.Statement;
import com.amazonaws.auth.policy.Statement.Effect;
import com.amazonaws.auth.policy.internal.JsonDocumentFields;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import com.seagates3.authserver.AuthServerConfig;
import com.seagates3.dao.ldap.AccountImpl;
import com.seagates3.dao.ldap.UserImpl;
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Account;
import com.seagates3.model.User;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.PolicyResponseGenerator;

/**
 * Generic class to validate Policy. Sub classes of this class should be
 * instantiated in order to validate the policy.
 */
public
abstract class PolicyValidator {

 private
  final Logger LOGGER =
      LoggerFactory.getLogger(PolicyValidator.class.getName());

 protected
  PolicyResponseGenerator responseGenerator = null;

  public abstract ServerResponse validatePolicy(String inputBucket, String jsonPolicy);
  abstract boolean isArnFormatValid(String arn);

  /**
   * Validate if the Effect value is one of - Allow/Deny
   *
   * @param effectValue
   * @return {@link ServerResponse}
   */
 protected
  ServerResponse validateEffect(Effect effectValue) {
    ServerResponse response = null;
    if (effectValue != null) {
      if (Statement.Effect.valueOf(effectValue.name()) == null) {
        response = responseGenerator.malformedPolicy("Invalid effect : " +
                                                     effectValue.name());
        LOGGER.error("Effect value is invalid in policy");
      }
    } else {
      response =
          responseGenerator.malformedPolicy("Missing required field Effect");
      LOGGER.error("Missing required field Effect");
    }
    return response;
  }

  /**
   * Validate the Conditions form Statement
   *
   * @param conditionList
 * @param actionList
   * @return {@link ServerResponse}
   */
 protected
  ServerResponse validateCondition(List<Condition> conditionList,
                                   List<Action> actionList) {
    ServerResponse response = null;
    ConditionUtil util = ConditionUtil.getInstance();
    if (conditionList != null) {
      for (Condition condition : conditionList) {
        String conditionType = condition.getType();
        String conditionKey = condition.getConditionKey();

        // Validate Condition Type
        if (!util.isConditionTypeValid(conditionType)) {
          response = responseGenerator.malformedPolicy(
              "Invalid Condition type : " + conditionType);
          LOGGER.error("Condition type - " + conditionType +
                       " is invalid in bucket policy");
          break;
        }

        // Validate Condition Key
        if (!util.isConditionKeyValid(conditionKey)) {
          response = responseGenerator.malformedPolicy(
              "Policy has an invalid condition key");
          LOGGER.error("Policy has an invalid condition key: " + conditionKey);
          break;
        }

        // Validate combination of Condition key and Action
        for (Action action : actionList) {
          if (!util.isConditionCombinationValid(conditionKey,
                                                action.getActionName())) {
            String errorMsg = "Conditions do not apply to combination of " +
                              "actions and resources in statement";
            response = responseGenerator.malformedPolicy(errorMsg);
            LOGGER.error(errorMsg + " for condition key: " + conditionKey);
            break;
          }
        }
      }
    }
    return response;
  }

  /**
   * Validate the Principal
   * @param principals
   * @return {@link ServerResponse}
   */
 protected
  ServerResponse validatePrincipal(List<Principal> principals) {
    ServerResponse response = null;
    if (principals != null && !principals.isEmpty()) {
      for (Principal principal : principals) {
        if (!principal.getProvider().equals("AWS") &&
            !principal.getProvider().equals("Service") &&
            !principal.getProvider().equals("Federated") &&
            !principal.getProvider().equals("*")) {

          response =
              responseGenerator.malformedPolicy("Invalid bucket policy syntax");
          LOGGER.error("Invalid bucket policy syntax");
          break;
        }
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
          if (new PrincipalArnParser().isArnFormatValid(id)) {
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
          LOGGER.error("Exception occurred while finding principal", e);
          isValid = false;
        }
        break;
      case "Service":
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

  /**
   * Validate Action and Resource respectively first. Then validate each
   *Resource
   * against the Actions.
   *
   * @param actionList
   * @param resourceValues
   * @param inputBucket
   * @return {@link ServerResponse}
   */
  abstract ServerResponse
      validateActionAndResource(List<Action> actionList,
                                List<Resource> resourceValues,
                                String inputBucket);


  protected
  ServerResponse validateVersion(String versionValue) {
    ServerResponse response = null;
    String policyVersion = AuthServerConfig.getPolicyVersion();
    if (versionValue != null) {
        if (versionValue.isEmpty()) {
          response = responseGenerator.malformedPolicy(
              "Syntax errors in policy.");
          LOGGER.error("Version field cannot be empty");
          return response;

        } else if (!versionValue.equals(policyVersion)) {
          response = responseGenerator.malformedPolicy(
              "Policy document must have version "+policyVersion+" or greater.");
          LOGGER.error(
              "Policy document must have version "+policyVersion+" or greater.");
          return response;
        }
      }
    return response;
  }
  protected
  ServerResponse validateStatementSyntax(JSONObject jsonObject) {
    ServerResponse response = null;
    if (jsonObject.get(JsonDocumentFields.STATEMENT) instanceof JSONArray) {
        JSONArray arr = (JSONArray)jsonObject.get(JsonDocumentFields.STATEMENT);
        if (arr.length() == 0) {
          response =
              responseGenerator.malformedPolicy("Syntax errors in policy.");
          LOGGER.error("Statement array can not be empty");
          return response;
        }
        for (int count = 0; count < arr.length(); count++) {
          if (!(arr.get(count) instanceof JSONObject)) {
            response =
                responseGenerator.malformedPolicy("Syntax errors in policy.");
            LOGGER.error(
                "Statement array element must be a instance of JSONObject");
            return response;
          }
        }
      } else if (!(jsonObject.get(JsonDocumentFields.STATEMENT) instanceof JSONObject)) {
    	  response =
    	            responseGenerator.malformedPolicy("Syntax errors in policy.");
    	        LOGGER.error(
    	            "Statement can not be other than a json object or array of json "+
    	            "objects");
    	        return response;
      } 
    return response;
  }
  
  protected
  ServerResponse validateEffect(String effectValue) {
    ServerResponse response = null;
    if (effectValue != null) {
        if (effectValue.isEmpty()) {
          response = responseGenerator.malformedPolicy(
              "Syntax errors in policy.");
          LOGGER.error("Required field Effect is empty");
          return response;

        } else if (!effectValue.equals(Statement.Effect.Allow.toString()) &&
                   !effectValue.equals(Statement.Effect.Deny.toString())) {
          response = responseGenerator.malformedPolicy("Syntax errors in policy.");
          LOGGER.error("Invalid effect value - " +
                       effectValue);
          return response;
        }
      }
    return response;
  }
  
  protected
  ServerResponse validateActionSyntax(JSONObject jsonObject) {
    ServerResponse response = null;
    if (jsonObject.get(JsonDocumentFields.ACTION) instanceof JSONArray) {
        JSONArray arr = (JSONArray)jsonObject.get(JsonDocumentFields.ACTION);
        if (arr.length() == 0) {
          response = responseGenerator.malformedPolicy(
              "Policy statement must contain actions.");
          LOGGER.error(
              "Policy statement must contain actions, it can not be empty "+
              "array.");
          return response;
        }
        for (int count = 0; count < arr.length(); count++) {
          if (!(arr.get(count) instanceof String)) {
            response =
                responseGenerator.malformedPolicy("Syntax errors in policy.");
            LOGGER.error("Action array element must be a instance of String");
            return response;
          }
        }
      }else if (!(jsonObject.get(
              JsonDocumentFields.ACTION) instanceof String)) {
    	  response =
			     responseGenerator.malformedPolicy("Syntax errors in policy.");
			 LOGGER.error(
			     "Action can not be other than string or array of strings");
			 return response;

      }
    return response;
  }
  
  protected
  ServerResponse validateResourceSyntax(JSONObject jsonObject) {
    ServerResponse response = null;
    if (jsonObject.get(JsonDocumentFields.RESOURCE) instanceof JSONArray) {
        JSONArray arr =
            (JSONArray)jsonObject.get(JsonDocumentFields.RESOURCE);
        if (arr.length() == 0) {
          response = responseGenerator.malformedPolicy(
              "Policy statement must contain resources.");
          LOGGER.error(
              "Policy statement must contain resources, it can not be empty "+
              "array");
          return response;
        }
        for (int count = 0; count < arr.length(); count++) {
          if (!(arr.get(count) instanceof String)) {
            response =
                responseGenerator.malformedPolicy("Syntax errors in policy.");
            LOGGER.error(
                "Resource array element must be a instance of String");
            return response;
          }
          String resourceArn = arr.get(count).toString();
          if (!this.isArnFormatValid(resourceArn) &&
              !resourceArn.equals("*")) {
            response = responseGenerator.malformedPolicy(
                "Resource " + resourceArn +
                " must be in ARN format or \"*\".");
            LOGGER.error("Resource " + resourceArn +
                         " must be in ARN format or \"*\".");
            return response;
          }
        }
      } else if (jsonObject.get(
              JsonDocumentFields.RESOURCE) instanceof String) {
			 String resourceArn =
			     jsonObject.get(JsonDocumentFields.RESOURCE).toString();
			 if (!this.isArnFormatValid(resourceArn) && !resourceArn.equals("*")) {
			   response = responseGenerator.malformedPolicy(
			       "Resource " + resourceArn + " must be in ARN format or \"*\".");
			   LOGGER.error("Resource " + resourceArn +
			                " must be in ARN format or \"*\".");
			   return response;
			 }
		} else {
	          response =
	                  responseGenerator.malformedPolicy("Syntax errors in policy.");
	              LOGGER.error(
	                  "Resource can not be other than string or array of strings");
	              return response;
	      }
    return response;
  }
  
  protected
  ServerResponse checkUnknownElements(Iterator<String> keys, Set<String> elements) {
    ServerResponse response = null;
    while (keys.hasNext()) {
      String key = keys.next();
      if (!elements.contains(key)) {  // some unknown field found
        response = responseGenerator.malformedPolicy("Syntax errors in policy");
        LOGGER.error("Unknown field in a policy document - " + key);
        return response;
      }
    }
    return response;
  }
  
}