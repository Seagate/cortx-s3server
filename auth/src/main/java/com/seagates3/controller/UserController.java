/*
 * COPYRIGHT 2015 SEAGATE LLC
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
 * Original author:  Arjun Hariharan <arjun.hariharan@seagate.com>
 * Original creation date: 17-Sep-2014
 */

package com.seagates3.controller;

import java.util.Map;

import com.seagates3.dao.AccessKeyDAO;
import com.seagates3.dao.DAODispatcher;
import com.seagates3.dao.DAOResource;
import com.seagates3.dao.UserDAO;
import com.seagates3.model.Requestor;
import com.seagates3.model.User;
import com.seagates3.response.generator.xml.UserResponseGenerator;
import com.seagates3.response.ServerResponse;
import com.seagates3.util.KeyGenUtil;

public class UserController extends AbstractController {
    UserDAO userDAO;
    UserResponseGenerator userResponse;

    public UserController(Requestor requestor,
            Map<String, String> requestBody) {
        super(requestor, requestBody);

        userDAO = (UserDAO) DAODispatcher.getResourceDAO(DAOResource.USER);
        userResponse = new UserResponseGenerator();
    }

    @Override
    public ServerResponse create() {
        User user = userDAO.findUser(requestor.getAccountName(),
                requestBody.get("UserName"));

        if(user.exists()) {
            return userResponse.entityAlreadyExists();
        }

        if(requestBody.containsKey("path")) {
            user.setPath(requestBody.get("path"));
        } else {
            user.setPath("/");
        }

        user.setFederateduser(Boolean.FALSE);

        user.setId(KeyGenUtil.userId());

        Boolean success = userDAO.saveUser(user);
        if(!success) {
            return userResponse.internalServerError();
        }

        return userResponse.create(user.getName(), user.getPath(), user.getId());
    }

    @Override
    public ServerResponse delete() {
        Boolean userHasAccessKeys, status;
        User user = userDAO.findUser(requestor.getAccountName(),
                requestBody.get("UserName"));
        if(!user.exists()) {
            return userResponse.noSuchEntity();
        }

        AccessKeyDAO accessKeyDao =
                (AccessKeyDAO) DAODispatcher.getResourceDAO(DAOResource.ACCESS_KEY);

        userHasAccessKeys = accessKeyDao.hasAccessKeys(user.getId());
        if(userHasAccessKeys) {
            return userResponse.deleteConflict();
        }

        status = userDAO.deleteUser(user);
        if(!status) {
            return userResponse.internalServerError();
        }

        return userResponse.success("DeleteUser");
    }

    @Override
    public ServerResponse list() {
        String pathPrefix;

        if(requestBody.containsKey("PathPrefix")) {
            pathPrefix = requestBody.get("PathPrefix");
        } else {
            pathPrefix = "/";
        }

        User[] userList;
        userList = userDAO.findUsers(requestor.getAccountName(), pathPrefix);

        if(userList == null) {
            return userResponse.internalServerError();
        }

        return userResponse.list(userList);
    }

    @Override
    public ServerResponse update() {
        Boolean success;
        String newUserName = null, newPath = null;

        if(!requestBody.containsKey("NewUserName") &&
                !requestBody.containsKey("NewPath")) {
            return userResponse.missingParameter();
        }

        User user = userDAO.findUser(requestor.getAccountName(),
                requestBody.get("UserName"));

        if (!user.exists()) {
            return userResponse.noSuchEntity();
        }

        if(requestBody.containsKey("NewUserName")) {
            newUserName = requestBody.get("NewUserName");
            User newUser = userDAO.findUser(requestor.getAccountName(),
                    newUserName);

            if(newUser.exists()) {
                return userResponse.entityAlreadyExists();
            }
        }

        if(requestBody.containsKey("NewPath")) {
            newPath = requestBody.get("NewPath");
        }

        success = userDAO.updateUser(user, newUserName, newPath);
        if(!success) {
            return userResponse.internalServerError();
        }

        return userResponse.update();
    }
}
