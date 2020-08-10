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
import com.seagates3.dao.RoleDAO;
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Requestor;
import com.seagates3.model.Role;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.RoleResponseGenerator;
import com.seagates3.util.ARNUtil;
import com.seagates3.util.DateUtil;
import com.seagates3.util.KeyGenUtil;
import java.util.Map;
import org.joda.time.DateTime;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class RoleController extends AbstractController {

    RoleDAO roleDAO;
    RoleResponseGenerator responseGenerator;
    private final Logger LOGGER =
            LoggerFactory.getLogger(RoleController.class.getName());

    public RoleController(Requestor requestor,
            Map<String, String> requestBody) {
        super(requestor, requestBody);

        roleDAO = (RoleDAO) DAODispatcher.getResourceDAO(DAOResource.ROLE);
        responseGenerator = new RoleResponseGenerator();
    }

    /**
     * Create new role.
     *
     * @return ServerReponse
     */
    @Override
    public ServerResponse create() {
        Role role;
        try {
            role = roleDAO.find(requestor.getAccount(),
                    requestBody.get("RoleName"));
        } catch (DataAccessException ex) {
            return responseGenerator.internalServerError();
        }

        if (role.exists()) {
            return responseGenerator.entityAlreadyExists();
        }

        role.setRoleId(KeyGenUtil.createId());
        role.setRolePolicyDoc(requestBody.get("AssumeRolePolicyDocument"));

        if (requestBody.containsKey("path")) {
            role.setPath(requestBody.get("path"));
        } else {
            role.setPath("/");
        }

        role.setCreateDate(DateUtil.toServerResponseFormat(DateTime.now()));
        String arn = ARNUtil.createARN(requestor.getAccount().getId(), "role",
                role.getRoleId());
        role.setARN(arn);

        LOGGER.info("Creating role:" + role.getName());

        try {
            roleDAO.save(role);
        } catch (DataAccessException ex) {
            return responseGenerator.internalServerError();
        }

        return responseGenerator.generateCreateResponse(role);
    }

    /*
     * TODO
     * Check if the role has policy attached before deleting.
     */
    @Override
    public ServerResponse delete() {
        Role role;
        try {
            role = roleDAO.find(requestor.getAccount(),
                    requestBody.get("RoleName"));
        } catch (DataAccessException ex) {
            return responseGenerator.internalServerError();
        }

        if (!role.exists()) {
            return responseGenerator.noSuchEntity();
        }

        LOGGER.info("Deleting role:" + role.getName());

        try {
            roleDAO.delete(role);
        } catch (DataAccessException ex) {
            return responseGenerator.internalServerError();
        }

        return responseGenerator.generateDeleteResponse();
    }

    @Override
    public ServerResponse list() {
        String pathPrefix;

        if (requestBody.containsKey("PathPrefix")) {
            pathPrefix = requestBody.get("PathPrefix");
        } else {
            pathPrefix = "/";
        }

        LOGGER.info("Listing all roles of account: " + requestor.getAccount()
                                 + " pathPrefix: " + pathPrefix);

        Role[] roleList;
        try {
            roleList = roleDAO.findAll(requestor.getAccount(), pathPrefix);
        } catch (DataAccessException ex) {
            return responseGenerator.internalServerError();
        }

        return responseGenerator.generateListResponse(roleList);
    }
}
