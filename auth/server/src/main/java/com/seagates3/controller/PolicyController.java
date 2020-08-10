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
import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.PolicyResponseGenerator;
import com.seagates3.util.ARNUtil;
import com.seagates3.util.DateUtil;
import com.seagates3.util.KeyGenUtil;
import java.util.Map;
import org.joda.time.DateTime;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class PolicyController extends AbstractController {

    PolicyDAO policyDAO;
    PolicyResponseGenerator responseGenerator;
    private final Logger LOGGER =
            LoggerFactory.getLogger(PolicyController.class.getName());

    public PolicyController(Requestor requestor,
            Map<String, String> requestBody) {
        super(requestor, requestBody);

        policyDAO = (PolicyDAO) DAODispatcher.getResourceDAO(DAOResource.POLICY);
        responseGenerator = new PolicyResponseGenerator();
    }

    /**
     * Create new role.
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
            return responseGenerator.internalServerError();
        }

        if (policy.exists()) {
            return responseGenerator.entityAlreadyExists();
        }

        policy.setPolicyId(KeyGenUtil.createId());
        policy.setPolicyDoc(requestBody.get("PolicyDocument"));

        String arn = ARNUtil.createARN(requestor.getAccount().getId(), "policy",
                policy.getPolicyId());
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
            return responseGenerator.internalServerError();
        }

        return responseGenerator.generateCreateResponse(policy);
    }

}
