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
import com.seagates3.model.AccessKey;
import com.seagates3.model.AccessKeyStatus;
import com.seagates3.model.Requestor;
import com.seagates3.model.User;
import com.seagates3.response.generator.xml.AccessKeyResponseGenerator;
import com.seagates3.response.ServerResponse;
import com.seagates3.util.KeyGenUtil;

public class AccessKeyController extends AbstractController {
    AccessKeyDAO accessKeyDAO;
    UserDAO userDAO;
    AccessKeyResponseGenerator accessKeyResponse;

    public AccessKeyController(Requestor requestor,
            Map<String, String> requestBody) {
        super(requestor, requestBody);

        accessKeyDAO = (AccessKeyDAO) DAODispatcher.getResourceDAO(DAOResource.ACCESS_KEY);
        userDAO = (UserDAO) DAODispatcher.getResourceDAO(DAOResource.USER);
        accessKeyResponse = new AccessKeyResponseGenerator();
    }

    /*
     * Default maximum access keys allowed per user are 2.
     */
    @Override
    public ServerResponse create() {
        String userName;

        if(requestBody.containsKey("UserName")) {
            userName = requestBody.get("UserName");
        } else {
            userName = requestor.getName();
        }

        User user = userDAO.findUser(requestor.getAccountName(), userName);

        if(!user.exists()) {
            String errorMessage = String.format("the user with name %s cannot be "
                + "found", userName);
            return accessKeyResponse.noSuchEntity(errorMessage);
        }

        if(accessKeyDAO.getCount(userName) == 2) {
            return accessKeyResponse.accessKeyQuotaExceeded();
        }

        AccessKey accessKey = new AccessKey();
        String strToEncode = user.getId() + System.currentTimeMillis();

        accessKey.setUserId(user.getId());
        accessKey.setAccessKeyId(KeyGenUtil.userAccessKeyId());
        accessKey.setSecretKey(KeyGenUtil.userSercretKey(strToEncode));
        accessKey.setStatus(AccessKeyStatus.ACTIVE);

        Boolean success = accessKeyDAO.save(accessKey);
        if(!success) {
            return accessKeyResponse.internalServerError();
        }

        return accessKeyResponse.create(userName, accessKey);
    }

    @Override
    public ServerResponse delete() {
        AccessKey accessKey = accessKeyDAO.findAccessKey(requestBody.get("AccessKeyId"));
        String errorMessage;

        if(!accessKey.exists()) {
            errorMessage = String.format("The access key with id %s does "
                    + "not exist", accessKey.getAccessKeyId());
            return accessKeyResponse.noSuchEntity(errorMessage);
        }

        /*
         * When both access key id and username are given, ensure that the access
         * key id belongs to the user.
         */
        if(requestBody.containsKey("UserName")) {
            User user = userDAO.findUser(requestor.getAccountName(),
                    requestBody.get("UserName"));

            if(!user.exists()) {
                errorMessage = String.format("the user with name %s cannot be "
                + "found", user.getName());
                return accessKeyResponse.noSuchEntity(errorMessage);
            }

            if(accessKey.getUserId().compareTo(user.getId()) != 0) {
                errorMessage = String.format("User has no access key id %s",
                        accessKey.getAccessKeyId());
                return accessKeyResponse.noSuchEntity(errorMessage);
            }
        }

        Boolean success = accessKeyDAO.delete(accessKey);

        if(!success) {
            return accessKeyResponse.internalServerError();
        }

        return accessKeyResponse.delete();
    }

    @Override
    public ServerResponse list() {
        String userName;

        if(requestBody.containsKey("UserName")) {
            userName = requestBody.get("UserName");
        } else {
            userName = requestor.getName();
        }

        User user = userDAO.findUser(requestor.getAccountName(), userName);

        if(!user.exists()) {
            String errorMessage = String.format("the user with name %s cannot be "
                + "found", userName);
            return accessKeyResponse.noSuchEntity(errorMessage);
        }

        AccessKey[] accessKeyList;
        accessKeyList = accessKeyDAO.findUserAccessKeys(user);

        return accessKeyResponse.list(userName, accessKeyList);
    }

    @Override
    public ServerResponse update() {
        AccessKey accessKey = accessKeyDAO.findAccessKey(requestBody.get("AccessKeyId"));
        String errorMessage;

        if(!accessKey.exists()) {
            errorMessage = String.format("The access key with id %s does "
                    + "not exist", accessKey.getAccessKeyId());
            return accessKeyResponse.noSuchEntity(errorMessage);
        }

        /*
         * When both access key id and username are given, ensure that the access
         * key id belongs to the user.
         */
        if(requestBody.containsKey("UserName")) {
            User user = userDAO.findUser(requestor.getAccountName(),
                    requestBody.get("UserName"));

            if(!user.exists()) {
                errorMessage = String.format("the user with name %s cannot be "
                + "found", user.getName());
                return accessKeyResponse.noSuchEntity(errorMessage);
            }

            if(accessKey.getUserId().compareTo(user.getId()) != 0) {
                errorMessage = String.format("User has no access key id %s",
                        accessKey.getAccessKeyId());
                return accessKeyResponse.noSuchEntity(errorMessage);
            }
        }

        String newStatus = requestBody.get("Status");
        if(accessKey.getStatus().compareTo(newStatus) != 0) {
            Boolean success = accessKeyDAO.updateAccessKey(accessKey, newStatus);

            if(!success) {
                return accessKeyResponse.internalServerError();
            }
        }

        return accessKeyResponse.update();
    }
}
