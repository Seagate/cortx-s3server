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
import java.util.List;
import java.util.Set;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.amazonaws.auth.policy.Action;
import com.amazonaws.auth.policy.Resource;
import com.amazonaws.util.json.JSONException;
import com.amazonaws.util.json.JSONObject;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.PolicyResponseGenerator;

public
class IAMPolicyValidator extends PolicyValidator {

 private
  final Logger LOGGER =
      LoggerFactory.getLogger(IAMPolicyValidator.class.getName());
  static Set<String> policyElements = new HashSet<>();
  static Set<String> statementElements = new HashSet<>();
  IAMArnParser iamArnparser;
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

      response = validatePolicyElements(obj, policyElements, statementElements);
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

  @Override ServerResponse
  validateActionAndResource(List<Action> actionList,
                            List<Resource> resourceValues, String inputBucket) {
    // TODO Auto-generated method stub
    return null;
  }

  @Override boolean isArnFormatValid(String arn) {

    return iamArnparser.isArnFormatValid(arn);
  }
}
