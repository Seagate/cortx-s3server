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

import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Set;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.amazonaws.auth.policy.Action;
import com.amazonaws.auth.policy.Resource;
import com.amazonaws.auth.policy.internal.JsonDocumentFields;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.PolicyResponseGenerator;

public
class IAMPolicyValidator extends PolicyValidator {

 private
  final Logger LOGGER =
      LoggerFactory.getLogger(IAMPolicyValidator.class.getName());
  static Set<String> policyElements = new HashSet<>();
  static Set<String> statementElements = new HashSet<>();
  Set<String> sids = new HashSet<>();
  ArnParser iamArnparser = null;
 private
  static final int MAX_IAM_POLICY_SIZE = 6144;

 public
  IAMPolicyValidator() {
    responseGenerator = new PolicyResponseGenerator();
    iamArnparser = new IAMArnParser();
    initializePolicyElements();
    initializeStatementElements();
  }

 private
  static void initializePolicyElements() {
    policyElements.add("Version");
    policyElements.add("Statement");
  }

 private
  static void initializeStatementElements() {
    statementElements.add("Sid");
    statementElements.add("Effect");
    statementElements.add("Action");
    statementElements.add("Resource");
  }

  /**
   * This method validate the policy json passed in
   *
   * @param jsonPolicy
   * @return null if policy is valid successfully. Not null response
   *  if policy is not valid.
   *
   */
  @Override public ServerResponse validatePolicy(String inputResource,
                                                 String jsonPolicy) {
    ServerResponse response = null;
    try {

      JSONObject obj = new JSONObject(jsonPolicy);
      if (obj.toString().length() > MAX_IAM_POLICY_SIZE) {
        LOGGER.error("Cannot exceed quota for PolicySize: " +
                     MAX_IAM_POLICY_SIZE);
        return responseGenerator.limitExceeded(
            "Cannot exceed quota for PolicySize: " + MAX_IAM_POLICY_SIZE);
      }
      response = validatePolicyElements(obj);
    }
    catch (JSONException e) {
      response = responseGenerator.malformedPolicy(
          "This policy contains invalid Json - " + e.getMessage());
      LOGGER.error("This policy contains invalid Json - ", e.getMessage());
    }
    catch (Exception e) {
      response = responseGenerator.malformedPolicy(e.getMessage());
      LOGGER.error("Exception in ValidatePolicy -  ", e);
    }
    return response;
  }

  /**
   * This method validate the policy elements
 * @param jsonObject
 * @return null if elements are valid. Not null Response if elements
 * are not valid
 * @throws JSONException
 */
  ServerResponse validatePolicyElements(JSONObject jsonObject)
      throws JSONException {
    ServerResponse response = null;
    if (!jsonObject.has(JsonDocumentFields.VERSION) ||
        !jsonObject.has(JsonDocumentFields.STATEMENT)) {
      response = responseGenerator.malformedPolicy("Syntax errors in policy.");
      LOGGER.error("Missing required field Version or Statement");
      return response;
    }
    LOGGER.debug("Checking for unknown fields in policy doc");
    response = checkUnknownElements(jsonObject, policyElements);
    if (response != null) return response;
    Iterator<String> keys = jsonObject.keys();
    while (keys.hasNext()) {
      String key = keys.next();
      if (JsonDocumentFields.VERSION.equals(key)) {
        LOGGER.debug("Validating IAM policy version field");
        response = validateVersion(
            jsonObject.get(JsonDocumentFields.VERSION).toString());
        if (response != null) return response;
      } else if (JsonDocumentFields.STATEMENT.equals(key)) {
        LOGGER.debug("Validating IAM policy statement field syntax");
        response = validateStatementSyntax(jsonObject, sids);
        if (response != null) return response;

        if (jsonObject.get(JsonDocumentFields.STATEMENT) instanceof JSONArray) {
          LOGGER.debug("Validating each policy statement object.");
          JSONArray arr =
              (JSONArray)jsonObject.get(JsonDocumentFields.STATEMENT);
          for (int count = 0; count < arr.length(); count++) {
            JSONObject obj = (JSONObject)arr.get(count);
            response = validateStatementElements(obj);
            if (response != null) {
              return response;
            }
          }
        } else if (jsonObject.get(
                       JsonDocumentFields.STATEMENT) instanceof JSONObject) {
          LOGGER.debug("Validating policy statement object.");
          JSONObject obj =
              (JSONObject)jsonObject.get(JsonDocumentFields.STATEMENT);
          response = validateStatementElements(obj);
          if (response != null) {
            return response;
          }
        }
      }
    }
    return response;
  }

  /** This method validate the policy statement object
 * @param jsonObject
 * @return null if statement object is valid. Not null Response if not valid.
 * @throws JSONException
 */
  ServerResponse validateStatementElements(JSONObject jsonObject)
      throws JSONException {
    ServerResponse response = null;
    if (!jsonObject.has(JsonDocumentFields.STATEMENT_EFFECT) ||
        !jsonObject.has(JsonDocumentFields.ACTION) ||
        !jsonObject.has(JsonDocumentFields.RESOURCE)) {
      response = responseGenerator.malformedPolicy("Syntax errors in policy.");
      LOGGER.error("Missing required field Effect or Action or Resource in a " +
                   "statement object.");
      return response;
    }

    response = checkUnknownElements(jsonObject, statementElements);
    if (response != null) return response;
    Iterator<String> keys = jsonObject.keys();
    while (keys.hasNext()) {
      String key = keys.next();
      if (JsonDocumentFields.STATEMENT_EFFECT.equals(key)) {
        response = validateEffect(
            jsonObject.get(JsonDocumentFields.STATEMENT_EFFECT).toString());
        if (response != null) break;
      } else if (JsonDocumentFields.ACTION.equals(key)) {
        response = validateActionSyntax(jsonObject);
        if (response != null) break;

      } else if (JsonDocumentFields.RESOURCE.equals(key)) {
        response = validateResourceSyntax(jsonObject);
        if (response != null) break;
      }
    }
    return response;
  }

  @Override boolean isArnFormatValid(String arn) {

    return iamArnparser.isArnFormatValid(arn);
  }

  @Override ServerResponse
  validateActionAndResource(List<Action> actionList,
                            List<Resource> resourceValues, String inputBucket) {
    // TODO Auto-generated method stub
    return null;
  }
}
