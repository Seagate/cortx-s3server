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

package com.seagates3.controller;

import com.seagates3.dao.DAODispatcher;
import com.seagates3.dao.DAOResource;
import com.seagates3.dao.PolicyDAO;
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Policy;
import com.seagates3.model.Requestor;
import com.seagates3.policy.IAMPolicyValidator;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.PolicyResponseGenerator;
import com.seagates3.util.ARNUtil;
import com.seagates3.util.DateUtil;
import com.seagates3.util.KeyGenUtil;
import java.util.Map;
import org.joda.time.DateTime;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import java.util.List;

public class PolicyController extends AbstractController {

    PolicyDAO policyDAO;
    PolicyResponseGenerator responseGenerator;
    IAMPolicyValidator iamPolicyValidator;
    private final Logger LOGGER =
            LoggerFactory.getLogger(PolicyController.class.getName());

    public PolicyController(Requestor requestor,
            Map<String, String> requestBody) {
        super(requestor, requestBody);

        policyDAO = (PolicyDAO) DAODispatcher.getResourceDAO(DAOResource.POLICY);
        iamPolicyValidator = new IAMPolicyValidator();
        responseGenerator = new PolicyResponseGenerator();
    }

    /**
     * Create new policy
     *
     * @return ServerReponse
     */
    @Override
    public ServerResponse create() {
        Policy policy;
        try {
            policy = policyDAO.find(requestor.getAccount(),
                    requestBody.get("PolicyName"));
        } catch (DataAccessException ex) {
          LOGGER.error("Failed to create policy- " +
                       requestBody.get("PolicyName"));
            return responseGenerator.internalServerError();
        }

        if (policy != null && policy.exists()) {
            return responseGenerator.entityAlreadyExists();
        }
        ServerResponse response = iamPolicyValidator.validatePolicy(null,requestBody.get("PolicyDocument"));
        if(response != null) {
        	return response;
        }
        policy = new Policy();
        policy.setName(requestBody.get("PolicyName"));
        policy.setAccount(requestor.getAccount());
        policy.setPolicyId(KeyGenUtil.createId());
        policy.setPolicyDoc(requestBody.get("PolicyDocument"));

        String arn = ARNUtil.createARN(requestor.getAccount().getId(), "policy",
                                       requestBody.get("PolicyName"));
        policy.setARN(arn);

        if (requestBody.containsKey("path")) {
            policy.setPath(requestBody.get("path"));
        } else {
            policy.setPath("/");
        }

        if (requestBody.containsKey("Description")) {
            policy.setDescription(requestBody.get("Description"));
        }
        String currentTime = DateUtil.toServerResponseFormat(DateTime.now());
        policy.setCreateDate(currentTime);
        policy.setUpdateDate(currentTime);
        policy.setDefaultVersionId("v1");
        policy.setAttachmentCount(0);

        LOGGER.info("Creating policy: " + policy.getName());

        try {
            policyDAO.save(policy);
        } catch (DataAccessException ex) {
          LOGGER.error("Exception while saving the policy- " +
                       requestBody.get("PolicyName"));
            return responseGenerator.internalServerError();
        }

        return responseGenerator.generateCreateResponse(policy);
    }
    /**
       * Delete an IAM Policy.
       *
       * @return ServerResponse
       */
    @Override public ServerResponse delete () {
      Policy policy = null;
      try {
        policy = policyDAO.findByArn(requestBody.get("PolicyARN"),
                                     requestor.getAccount());
      }
      catch (DataAccessException ex) {
        LOGGER.error("Failed to find the requested policy in ldap- " +
                     requestBody.get("PolicyARN"));
        return responseGenerator.internalServerError();
      }
      if (policy == null || !policy.exists()) {
        LOGGER.error(requestBody.get("PolicyARN") + "Policy does not exists");
        return responseGenerator.noSuchEntity();
      }
      LOGGER.info("Deleting policy : " + policy.getName());
      try {
        policy.setAccount(requestor.getAccount());
        policyDAO.delete (policy);
      }
      catch (DataAccessException ex) {
        LOGGER.error("Failed to delete requested policy- " +
                     requestBody.get("PolicyARN"));
        return responseGenerator.internalServerError();
      }
      return responseGenerator.generateDeleteResponse();
    }

    /**
     * List all the IAM policies for requesting account
     *
     * @return ServerResponse.
     */
    @Override public ServerResponse list() {
      List<Policy> policyList = null;
      try {
        policyList = policyDAO.findAll(requestor.getAccount());
      }
      catch (DataAccessException ex) {
        LOGGER.error("Failed to list policies for account - " +
                     requestor.getAccount().getName());
        return responseGenerator.internalServerError();
      }
      LOGGER.info("Listing policies of account : " +
                  requestor.getAccount().getName());
      return responseGenerator.generateListResponse(policyList);
    }

    /**
     * Get policy for requesting ARN
     *
     * @return ServerResponse.
     */
    @Override public ServerResponse get() {
      Policy policy = null;
      try {
        policy = policyDAO.findByArn(requestBody.get("PolicyARN"),
                                     requestor.getAccount());
      }
      catch (DataAccessException ex) {
        LOGGER.error("Failed to get requested policy- " +
                     requestBody.get("PolicyARN"));
        return responseGenerator.internalServerError();
      }
      if (policy == null || !policy.exists()) {
        LOGGER.error("Policy does not exists- " + requestBody.get("PolicyARN"));
        return responseGenerator.noSuchEntity();
      }
      LOGGER.info("Getting policy with ARN -  : " + policy.getARN());
      return responseGenerator.generateGetResponse(policy);
    }
}
