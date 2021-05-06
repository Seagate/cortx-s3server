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

import com.seagates3.authserver.AuthServerConfig;
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
import com.seagates3.s3service.S3AccountNotifier;
import com.seagates3.util.KeyGenUtil;
import com.seagates3.service.GlobalDataStore;

import io.netty.handler.codec.http.HttpResponseStatus;

import java.util.Map;


import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class AccountController extends AbstractController {

    private final Logger LOGGER = LoggerFactory.getLogger(AccountController.class.getName());
    private final AccountDAO accountDao;
    private final UserDAO userDAO;
    private final AccessKeyDAO accessKeyDAO;
    private final RoleDAO roleDAO;
    private final GroupDAO groupDAO;
    private final PolicyDAO policyDAO;
    private final AccountResponseGenerator accountResponseGenerator;
    private final S3AccountNotifier s3;
    private boolean internalRequest = false;
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
        s3 = new S3AccountNotifier();
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

        return accountResponseGenerator.generateListResponse(
            accounts, requestBody.get("ShowAll"));
    }

    @Override
    public ServerResponse create() {
        String name = requestBody.get("AccountName");
        String email = requestBody.get("Email");
        Account account;
        int accountCount = 0;
        LOGGER.info("Creating account: " + name);
        int maxAccountLimit = AuthServerConfig.getMaxAccountLimit();
        int internalAccountCount =
            AuthServerConfig.getS3InternalAccounts().size();
        int maxAllowedLdapResults = maxAccountLimit + internalAccountCount;

        try {
          accountCount = getTotalCountOfAccounts();

          if (accountCount >= maxAllowedLdapResults) {
            LOGGER.error("Maximum allowed Account limit has exceeded (i.e." +
                         maxAccountLimit + ")");
            return accountResponseGenerator.maxAccountLimitExceeded(
                maxAccountLimit);
          }
        }
        catch (DataAccessException ex) {
          LOGGER.error("Failed to validate account entry count limit -" + ex);
          return accountResponseGenerator.internalServerError();
        }

        try {
            account = accountDao.find(name);
        } catch (DataAccessException ex) {
          LOGGER.error("Failed to find account in ldap -" + ex);
            return accountResponseGenerator.internalServerError();
        }

        if (account.exists()) {
            return accountResponseGenerator.entityAlreadyExists();
        }

        account.setId(KeyGenUtil.createAccountId());

        try {
          // Generate unique canonical id for account
          String canonicalId = generateUniqueCanonicalId();
          if (canonicalId != null) {
            account.setCanonicalId(canonicalId);
          } else {
            // Failed to generate unique canonical id
            return accountResponseGenerator.internalServerError();
          }
        }
        catch (DataAccessException ex) {
          return accountResponseGenerator.internalServerError();
        }

        account.setEmail(email);

        try {
            accountDao.save(account);
        } catch (DataAccessException ex) {
          // Check for constraint violation exception for email address
          if (ex.getMessage().contains("some attributes not unique")) {
            try {
              account = accountDao.findByEmailAddress(email);
            }
            catch (DataAccessException e) {
              return accountResponseGenerator.internalServerError();
            }

            if (account.exists()) {
              return accountResponseGenerator.emailAlreadyExists();
            }
          }
            return accountResponseGenerator.internalServerError();
        }

        User root;
        try {
          root = createRootUser(name, account.getId());
        } catch (DataAccessException ex) {
            return accountResponseGenerator.internalServerError();
        }

        AccessKey rootAccessKey;
        try {
            rootAccessKey = createRootAccessKey(root);
        } catch (DataAccessException ex) {
            return accountResponseGenerator.internalServerError();
        }

        try {
          // Added delay so that newly created keys are replicated in ldap
          Thread.sleep(500);
        }
        catch (InterruptedException e) {
          LOGGER.error("Create account delay failing - ", e);
          Thread.currentThread().interrupt();
        }

        // Handle multi-threaded create API calls by deleting extra accounts,
        // if account limit creation exceeded due to multiple thread/multiple
        // node create() API calls.
        try {
          accountCount = getTotalCountOfAccounts();
        }
        catch (DataAccessException ex) {
          LOGGER.error("failed to get total count of accounts from ldap :" +
                       ex);
          return accountResponseGenerator.internalServerError();
        }

        try {
          if (accountCount > maxAllowedLdapResults) {
            // delete newly created account since we exceeded account
            // creation limit.
            accountDao.ldap_delete_account(account);
            return accountResponseGenerator.internalServerError();
          }
        }
        catch (DataAccessException ex) {
          LOGGER.error("Failed to create account -" + ex);
          return accountResponseGenerator.internalServerError();
        }

        return accountResponseGenerator.generateCreateResponse(account, root,
                rootAccessKey);
    }

    /**
     * Fetch total account count present in ldap
     * @return count of accounts
     * @throws DataAccessException
     */
   private
    int getTotalCountOfAccounts() throws DataAccessException {
      Account[] accounts;
      accounts = accountDao.findAll();
      return accounts.length;
    }

    /**
 * Generate canonical id and check if its unique in ldap
 * @throws DataAccessException
 */
   private
    String generateUniqueCanonicalId() throws DataAccessException {
      Account account;
      String canonicalId;
      for (int i = 0; i < 5; i++) {
        canonicalId = KeyGenUtil.createCanonicalId();
        account = accountDao.findByCanonicalID(canonicalId);

        if (!account.exists()) {
          return canonicalId;
        }
      }
      return null;
    }

    public ServerResponse resetAccountAccessKey() {
        String name = requestBody.get("AccountName");
        LOGGER.info("Resetting access key of account: " + name);

        Account account;
        try {
            account = accountDao.find(name);
        } catch (DataAccessException ex) {
            return accountResponseGenerator.internalServerError();
        }

        if (!account.exists()) {
            LOGGER.error("Account [" + name +"] doesnot exist");
            return accountResponseGenerator.noSuchEntity();
        }

        User root;
        try {
            root = userDAO.find(account.getName(), "root");
        } catch (DataAccessException e) {
            return accountResponseGenerator.internalServerError();
        }

        if (!root.exists()) {
                LOGGER.error("Root user of account [" + name +"] doesnot exist");
                return accountResponseGenerator.noSuchEntity();
        }

        // Delete Existing Root Access Keys
        LOGGER.info("Deleting existing access key of account: " + name);
        try {
            deleteAccessKeys(root);
        } catch (DataAccessException e) {
            return accountResponseGenerator.internalServerError();
        }

        LOGGER.debug("Creating new access key for account: " + name);
        AccessKey rootAccessKey;
        try {
            rootAccessKey = createRootAccessKey(root);
        } catch (DataAccessException ex) {
            return accountResponseGenerator.internalServerError();
        }
        try {
          // Added delay so that newly created keys are replicated in ldap
          Thread.sleep(500);
        }
        catch (InterruptedException e) {
          LOGGER.error("Reset key  delay failing - ", e);
          Thread.currentThread().interrupt();
        }
        return accountResponseGenerator.generateResetAccountAccessKeyResponse(
                                          account, root, rootAccessKey);
    }

    /*
     * Create a root user for the account.
     */
   private
    User createRootUser(String accountName,
                        String accountId) throws DataAccessException {
        User user = new User();
        user.setAccountName(accountName);
        user.setName("root");
        user.setPath("/");
        user.setUserType(User.UserType.IAM_USER);
        user.setId(KeyGenUtil.createUserId());
        user.setArn("arn:aws:iam::" + accountId + ":root");
        LOGGER.info("Creating root user for account: " + accountName);

        userDAO.save(user);
        return user;
    }

    /*
     * Create access keys for the root user.
     */
    private AccessKey createRootAccessKey(User root) throws DataAccessException {
        AccessKey accessKey = new AccessKey();
        accessKey.setUserId(root.getId());
        accessKey.setId(KeyGenUtil.createUserAccessKeyId(true));
        accessKey.setSecretKey(KeyGenUtil.generateSecretKey());
        accessKey.setStatus(AccessKeyStatus.ACTIVE);

        accessKeyDAO.save(accessKey);

        return accessKey;
    }

    @Override
    public ServerResponse delete() {
        String name = requestBody.get("AccountName");
        Account account;

        LOGGER.info("Deleting account: " + name);

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

        if (requestor.getId() != null &&
            !(requestor.getId().equals(root.getId()))) {
            return accountResponseGenerator.unauthorizedOperation();
        }

        boolean force = false;
        if (requestBody.containsKey("force")) {
            force = Boolean.parseBoolean(requestBody.get("force"));
        }

        //Notify S3 Server of account deletion
        if (!internalRequest) {
          ServerResponse resp = null;
          // check if access key is ldap credentials or not
          if (requestor.getAccesskey().getId().equals(
                  AuthServerConfig.getLdapLoginCN())) {
            AccessKey accountAccessKey;
            try {  // if ldap credentials are used then find access key of
                   // account.
              accountAccessKey = accessKeyDAO.findAccountAccessKey(account);
            }
            catch (DataAccessException e) {
              LOGGER.error("Failed to find Access Key for account :" +
                           account.getName() + "exception: " + e);
              return accountResponseGenerator.internalServerError();
            }
            LOGGER.debug("Sending delete account [" + account.getName() +
                         "] notification to S3 Server");
            resp = s3.notifyDeleteAccount(
                account.getId(), accountAccessKey.getId(),
                accountAccessKey.getSecretKey(), accountAccessKey.getToken());
          } else {
            LOGGER.debug("Sending delete account [" + account.getName() +
                         "] notification to S3 Server");
            resp = s3.notifyDeleteAccount(
                account.getId(), requestor.getAccesskey().getId(),
                requestor.getAccesskey().getSecretKey(),
                requestor.getAccesskey().getToken());
          }

          if (resp == null ||
              !resp.getResponseStatus().equals(HttpResponseStatus.OK)) {
                LOGGER.error("Account [" + account.getName() + "] delete "
                    + "notification failed.");
                return resp;
            }
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

        LOGGER.info("Deleting all access keys of user: " + user.getName());

        AccessKey[] accessKeys = accessKeyDAO.findAll(user);
        for (AccessKey accessKey : accessKeys) {
            accessKeyDAO.delete(accessKey);
            GlobalDataStore.getInstance().getAuthenticationMap().remove(
                accessKey.getId());
        }
    }

    private void deleteUser(User user) throws DataAccessException {

        LOGGER.info("Deleting user: " + user.getName());

        deleteAccessKeys(user);
        userDAO.delete(user);
    }

    private void deleteUsers(Account account, String path) throws DataAccessException {

        LOGGER.info("Deleting all users of account: " + account.getName());

        User[] users = userDAO.findAll(account.getName(), path);
        for (User user : users) {
            deleteUser(user);
        }
    }

    private void deleteRoles(Account account, String path) throws DataAccessException {

        LOGGER.info("Deleting all associated roles of account: "
                                            + account.getName());

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

