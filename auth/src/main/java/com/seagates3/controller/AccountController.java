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
 * Original creation date: 17-Sep-2015
 */
package com.seagates3.controller;

import com.seagates3.dao.AccessKeyDAO;
import com.seagates3.dao.AccountDAO;
import com.seagates3.dao.DAODispatcher;
import com.seagates3.dao.DAOResource;
import com.seagates3.dao.GroupDAO;
import com.seagates3.dao.PolicyDAO;
import com.seagates3.dao.RoleDAO;
import com.seagates3.dao.UserDAO;
import com.seagates3.dao.ldap.LDAPUtils;
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.AccessKey;
import com.seagates3.model.AccessKey.AccessKeyStatus;
import com.seagates3.model.Account;
import com.seagates3.model.Group;
import com.seagates3.model.Policy;
import com.seagates3.model.Requestor;
import com.seagates3.model.Role;
import com.seagates3.model.User;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.AccountResponseGenerator;
import com.seagates3.util.KeyGenUtil;
import java.util.Map;
import java.util.logging.Level;
import java.util.logging.Logger;

public class AccountController extends AbstractController {

    private final AccountDAO accountDao;
    private final UserDAO userDAO;
    private final AccessKeyDAO accessKeyDAO;
    private final RoleDAO roleDAO;
    private final GroupDAO groupDAO;
    private final PolicyDAO policyDAO;
    private final AccountResponseGenerator accountResponseGenerator;

    public AccountController(Requestor requestor,
            Map<String, String> requestBody) {
        super(requestor, requestBody);

        accountResponseGenerator = new AccountResponseGenerator();
        accountDao = (AccountDAO) DAODispatcher.getResourceDAO(DAOResource.ACCOUNT);
        accessKeyDAO = (AccessKeyDAO) DAODispatcher.getResourceDAO(DAOResource.ACCESS_KEY);
        userDAO = (UserDAO) DAODispatcher.getResourceDAO(DAOResource.USER);
        roleDAO = (RoleDAO) DAODispatcher.getResourceDAO(DAOResource.ROLE);
        groupDAO = (GroupDAO) DAODispatcher.getResourceDAO(DAOResource.GROUP);
        policyDAO = (PolicyDAO) DAODispatcher.getResourceDAO(DAOResource.POLICY);
    }

    /*
     * Fetch all accounts from database
     */
    public ServerResponse list() {
        Account[] accounts;

        try {
            accounts = accountDao.findAll();
        } catch (DataAccessException ex) {
            return accountResponseGenerator.internalServerError();
        }

        return accountResponseGenerator.generateListResponse(accounts);
    }

    @Override
    public ServerResponse create() {
        String name = requestBody.get("AccountName");
        Account account;
        try {
            account = accountDao.find(name);
        } catch (DataAccessException ex) {
            return accountResponseGenerator.internalServerError();
        }

        if (account.exists()) {
            return accountResponseGenerator.entityAlreadyExists();
        }

        account.setId(KeyGenUtil.createUserId());
        account.setCanonicalId(KeyGenUtil.createId());
        account.setEmail(requestBody.get("Email"));

        try {
            accountDao.save(account);
        } catch (DataAccessException ex) {
            return accountResponseGenerator.internalServerError();
        }

        User root;
        try {
            root = createRootUser(name);
        } catch (DataAccessException ex) {
            return accountResponseGenerator.internalServerError();
        }

        AccessKey rootAccessKey;
        try {
            rootAccessKey = createRootAccessKey(root);
        } catch (DataAccessException ex) {
            return accountResponseGenerator.internalServerError();
        }

        return accountResponseGenerator.generateCreateResponse(account, root,
                rootAccessKey);
    }

    /*
     * Create a root user for the account.
     */
    private User createRootUser(String accountName) throws DataAccessException {
        User user = new User();
        user.setAccountName(accountName);
        user.setName("root");
        user.setPath("/");
        user.setUserType(User.UserType.IAM_USER);

        user.setId(KeyGenUtil.createUserId());

        userDAO.save(user);
        return user;
    }

    /*
     * Create access keys for the root user.
     */
    private AccessKey createRootAccessKey(User root) throws DataAccessException {
        AccessKey accessKey = new AccessKey();
        accessKey.setUserId(root.getId());
        accessKey.setId(KeyGenUtil.createUserAccessKeyId());
        accessKey.setSecretKey(KeyGenUtil.generateSecretKey());
        accessKey.setStatus(AccessKeyStatus.ACTIVE);

        accessKeyDAO.save(accessKey);

        return accessKey;
    }

    @Override
    public ServerResponse delete() {
        String name = requestBody.get("AccountName");
        Account account;

        try {
            account = accountDao.find(name);
        } catch (DataAccessException ex) {
            return accountResponseGenerator.internalServerError();
        }

        if (!account.exists()) {
            return accountResponseGenerator.noSuchEntity();
        }

        User root;
        try {
            root = userDAO.find(account.getName(), "root");
        } catch (DataAccessException e) {
            return accountResponseGenerator.internalServerError();
        }

        if (!requestor.getId().equals(root.getId())) {
            return accountResponseGenerator.unauthorizedOperation();
        }

        boolean force = false;
        if (requestBody.containsKey("force")) {
            force = Boolean.parseBoolean(requestBody.get("force"));
        }

        try {
            if (force) {
                deleteUsers(account, "/");
                deleteRoles(account, "/");
                deleteGroups(account, "/");
                deletePolicies(account, "/");
            } else {
                User[] users = userDAO.findAll(account.getName(), "/");
                if (users.length == 1 && "root".equals(users[0].getName())) {
                    deleteUser(users[0]);
                }
            }

            accountDao.deleteOu(account, LDAPUtils.USER_OU);
            accountDao.deleteOu(account, LDAPUtils.ROLE_OU);
            accountDao.deleteOu(account, LDAPUtils.GROUP_OU);
            accountDao.deleteOu(account, LDAPUtils.POLICY_OU);
            accountDao.delete(account);
        } catch (DataAccessException e) {
            if (e.getLocalizedMessage().contains("subordinate objects must be deleted first")) {
                return accountResponseGenerator.deleteConflict();
            }

            return accountResponseGenerator.internalServerError();
        }

        return accountResponseGenerator.generateDeleteResponse();
    }

    private void deleteAccessKeys(User user) throws DataAccessException {
        AccessKey[] accessKeys = accessKeyDAO.findAll(user);
        for (AccessKey accessKey : accessKeys) {
            accessKeyDAO.delete(accessKey);
        }
    }

    private void deleteUser(User user) throws DataAccessException {
        deleteAccessKeys(user);
        userDAO.delete(user);
    }

    private void deleteUsers(Account account, String path) throws DataAccessException {
        User[] users = userDAO.findAll(account.getName(), path);
        for (User user : users) {
            deleteUser(user);
        }
    }

    private void deleteRoles(Account account, String path) throws DataAccessException {
        Role[] roles = roleDAO.findAll(account, path);
        for (Role role : roles) {
            roleDAO.delete(role);
        }
    }

    private void deleteGroups(Account account, String path) {
        // TODO
    }

    private void deletePolicies(Account account, String path) {
        // TODO
    }
}
