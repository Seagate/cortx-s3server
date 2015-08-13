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
import com.seagates3.dao.AccountDAO;
import com.seagates3.dao.DAODispatcher;
import com.seagates3.dao.DAOResource;
import com.seagates3.dao.UserDAO;
import com.seagates3.model.AccessKey;
import com.seagates3.model.AccessKeyStatus;
import com.seagates3.model.Account;
import com.seagates3.model.Requestor;
import com.seagates3.model.User;
import com.seagates3.response.generator.xml.AccountResponseGenerator;
import com.seagates3.response.ServerResponse;
import com.seagates3.util.KeyGenUtil;

public class AccountController extends AbstractController {
    AccountDAO accountDao;
    AccountResponseGenerator accountResponse;

    /*
     *
     */
    public AccountController(Requestor requestor,
            Map<String, String> requestBody) {
        super(requestor, requestBody);

        accountResponse = new AccountResponseGenerator();
        accountDao = (AccountDAO) DAODispatcher.getResourceDAO(DAOResource.ACCOUNT);
    }

    @Override
    public ServerResponse create() {
        String name = requestBody.get("AccountName");
        Account account = accountDao.findAccount(name);

        if(account.exists()) {
            String errorMessage = String.format("The account %s exists already.",
                    name);
            return accountResponse.entityAlreadyExists(errorMessage);
        }

        Boolean success = accountDao.save(account);
        if(!success) {
            return accountResponse.internalServerError();
        }

        User root = createRootUser(name);
        if(root == null) {
            return accountResponse.internalServerError();
        }

        AccessKey rootAccessKey = createRootAccessKey(root);
        if(rootAccessKey == null) {
            return accountResponse.internalServerError();
        }

        return accountResponse.create(rootAccessKey);
    }

    private User createRootUser(String accountName) {
        UserDAO userDAO = (UserDAO) DAODispatcher.getResourceDAO(DAOResource.USER);
        User user = new User();
        user.setAccountName(accountName);
        user.setName("root");
        user.setPath("/");
        user.setFederateduser(Boolean.FALSE);

        user.setId(KeyGenUtil.userId());

        Boolean success = userDAO.saveUser(user);
        if(!success) {
            return null;
        }

        return user;
    }

    private AccessKey createRootAccessKey(User root) {
        AccessKeyDAO accessKeyDAO =
                (AccessKeyDAO) DAODispatcher.getResourceDAO(DAOResource.ACCESS_KEY);
        AccessKey accessKey = new AccessKey();

        String strToEncode = root.getId() + System.currentTimeMillis();

        accessKey.setUserId(root.getId());
        accessKey.setAccessKeyId(KeyGenUtil.userAccessKeyId());
        accessKey.setSecretKey(KeyGenUtil.userSercretKey(strToEncode));
        accessKey.setStatus(AccessKeyStatus.ACTIVE);

        Boolean success = accessKeyDAO.save(accessKey);
        if(!success) {
            return null;
        }

        return accessKey;
    }
}
