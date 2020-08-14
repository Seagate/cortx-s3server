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

import com.seagates3.dao.AccessKeyDAO;
import com.seagates3.dao.DAODispatcher;
import com.seagates3.dao.DAOResource;
import com.seagates3.dao.UserDAO;
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Requestor;
import com.seagates3.model.User;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.UserResponseGenerator;
import com.seagates3.util.KeyGenUtil;
import java.util.Map;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class UserController extends AbstractController {

    private final UserDAO userDAO;
    private final UserResponseGenerator userResponseGenerator;
    private final Logger LOGGER =
            LoggerFactory.getLogger(UserController.class.getName());
    private
     static final String USER_ID_PREFIX = "AIDA";

    public UserController(Requestor requestor,
            Map<String, String> requestBody) {
        super(requestor, requestBody);

        userDAO = (UserDAO) DAODispatcher.getResourceDAO(DAOResource.USER);
        userResponseGenerator = new UserResponseGenerator();
    }

    /**
     * Create a new IAM user.
     *
     * @return ServerResponse
     */
    @Override
    public ServerResponse create() {
        User user;
        try {
            user = userDAO.find(requestor.getAccount().getName(),
                    requestBody.get("UserName"));
        } catch (DataAccessException ex) {
            return userResponseGenerator.internalServerError();
        }

        if (user.exists()) {
            LOGGER.error("User [" + user.getName() + "] already exists");
            return userResponseGenerator.entityAlreadyExists();
        }

        LOGGER.info("Creating user : " + user.getName());

        if (requestBody.containsKey("path")) {
            user.setPath(requestBody.get("path"));
        } else {
            user.setPath("/");
        }

        user.setUserType(User.UserType.IAM_USER);
        // UserId should starts with "AIDA".
        user.setId(USER_ID_PREFIX + KeyGenUtil.createIamUserId());

        // create and set arn here
        String arn = "arn:aws:iam::" + requestor.getAccount().getId() +
                     ":user/" + user.getName();
        LOGGER.debug("Creating and setting ARN - " + arn);
        user.setArn(arn);
        try {
            userDAO.save(user);
        } catch (DataAccessException ex) {
            return userResponseGenerator.internalServerError();
        }

        return userResponseGenerator.generateCreateResponse(user);
    }

    /**
     * Delete an IAM user.
     *
     * @return ServerResponse
     */
    @Override
    public ServerResponse delete() {
        Boolean userHasAccessKeys;
        User user;
        try {
            user = userDAO.find(requestor.getAccount().getName(),
                    requestBody.get("UserName"));
        } catch (DataAccessException ex) {
            return userResponseGenerator.internalServerError();
        }

        if (!user.exists()) {
            LOGGER.error("User [" + user.getName() + "] does not exists");
            return userResponseGenerator.noSuchEntity();
        }

        LOGGER.info("Deleting user : " + user.getName());

        AccessKeyDAO accessKeyDao
                = (AccessKeyDAO) DAODispatcher.getResourceDAO(DAOResource.ACCESS_KEY);

        try {
            userHasAccessKeys = accessKeyDao.hasAccessKeys(user.getId());
        } catch (DataAccessException ex) {
            return userResponseGenerator.internalServerError();
        }

        if (userHasAccessKeys) {
            return userResponseGenerator.deleteConflict();
        }

        try {
            userDAO.delete(user);
        } catch (DataAccessException ex) {
            return userResponseGenerator.internalServerError();
        }

        return userResponseGenerator.generateDeleteResponse();
    }

    /**
     * List all the IAM users having the given path prefix.
     *
     * @return ServerResponse.
     */
    @Override
    public ServerResponse list() {
        String pathPrefix;

        if (requestBody.containsKey("PathPrefix")) {
            pathPrefix = requestBody.get("PathPrefix");
        } else {
            pathPrefix = "/";
        }

        User[] userList;
        try {
            userList = userDAO.findAll(requestor.getAccount().getName(),
                    pathPrefix);
        } catch (DataAccessException ex) {
            return userResponseGenerator.internalServerError();
        }

        LOGGER.info("Listing users of account : "
                      + requestor.getAccount().getName());
        return userResponseGenerator.generateListResponse(userList);
    }

    /**
     * Update the name or path of an existing IAM user.
     *
     * @return ServerResponse
     */
    @Override
    public ServerResponse update() {
        String newUserName = null, newPath = null;

        if (!requestBody.containsKey("NewUserName")
                && !requestBody.containsKey("NewPath")) {
            return userResponseGenerator.missingParameter();
        }

        if ("root".equals(requestBody.get("UserName")) &&
                requestBody.containsKey("NewUserName")) {
            LOGGER.error("Cannot change user name of root user.");
            return userResponseGenerator.operationNotSupported(
                    "Cannot change user name of root user."
            );
        }

        User user;
        try {
            user = userDAO.find(requestor.getAccount().getName(),
                    requestBody.get("UserName"));
        } catch (DataAccessException ex) {
            return userResponseGenerator.internalServerError();
        }

        if (!user.exists()) {
            LOGGER.error("User [" + user.getName() + "] does not exists");
            return userResponseGenerator.noSuchEntity();
        }

        LOGGER.info("Updating user : " + user.getName());

        if (requestBody.containsKey("NewUserName")) {
            newUserName = requestBody.get("NewUserName");
            User newUser;

            try {
                newUser = userDAO.find(requestor.getAccount().getName(),
                        newUserName);
            } catch (DataAccessException ex) {
                return userResponseGenerator.internalServerError();
            }

            if (newUser.exists()) {
                return userResponseGenerator.entityAlreadyExists();
            }
        }

        if (requestBody.containsKey("NewPath")) {
            newPath = requestBody.get("NewPath");
        }

        try {
            userDAO.update(user, newUserName, newPath);
        } catch (DataAccessException ex) {
            return userResponseGenerator.internalServerError();
        }

        return userResponseGenerator.generateUpdateResponse();
    }
}
