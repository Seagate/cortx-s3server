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

import java.util.Date;
import java.util.Map;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.seagates3.dao.AccountDAO;
import com.seagates3.dao.AccountLoginProfileDAO;
import com.seagates3.dao.DAODispatcher;
import com.seagates3.dao.DAOResource;
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Account;
import com.seagates3.model.Requestor;
import com.seagates3.parameter.validator.S3ParameterValidatorUtil;
import com.seagates3.model.User;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.AccountLoginProfileResponseGenerator;
import com.seagates3.response.generator.AccountResponseGenerator;
import com.seagates3.util.DateUtil;
/**
 *
 * TODO : Remove this comment once below code tested end to end
 *
 */
public
class AccountLoginProfileController extends AbstractController {

 private
  final AccountDAO accountDAO;
 private
  final AccountLoginProfileDAO accountLoginProfileDAO;

 private
  final AccountLoginProfileResponseGenerator
      accountLoginProfileResponseGenerator;
 private
  final AccountResponseGenerator accountResponseGenerator;
 private
  final Logger LOGGER =
      LoggerFactory.getLogger(AccountLoginProfileController.class.getName());

 public
  AccountLoginProfileController(Requestor requestor,
                                Map<String, String> requestBody) {
    super(requestor, requestBody);

    accountDAO = (AccountDAO)DAODispatcher.getResourceDAO(DAOResource.ACCOUNT);
    accountLoginProfileDAO =
        (AccountLoginProfileDAO)DAODispatcher.getResourceDAO(
            DAOResource.ACCOUNT_LOGIN_PROFILE);
    accountLoginProfileResponseGenerator =
        new AccountLoginProfileResponseGenerator();
    accountResponseGenerator = new AccountResponseGenerator();
  }

  @Override public ServerResponse create() {
    Account account;
    try {
      account = accountDAO.find(requestor.getAccount().getName());
    }
    catch (DataAccessException ex) {
      return accountResponseGenerator.internalServerError();
    }

    if (!account.exists()) {
      LOGGER.error("Account [" + account.getName() + "] does not exists");
      return accountResponseGenerator.noSuchEntity();
    } else {
      if (account.getPassword() == null) {
        try {
          // Validate new password as per password policy
          if (!S3ParameterValidatorUtil.validatePasswordPolicy(
                   requestBody.get("Password"))) {
            LOGGER.error(
                "Password does not conform to the account password policy");
            return accountResponseGenerator.passwordPolicyVoilation();
          }
          account.setPassword(requestBody.get("Password"));

          account.setProfileCreateDate(
              DateUtil.toLdapDate(new Date(DateUtil.getCurrentTime())));
          if (requestBody.get("PasswordResetRequired") == null) {
            account.setPwdResetRequired("FALSE");
          } else {
            account.setPwdResetRequired(
                requestBody.get("PasswordResetRequired").toUpperCase());
          }
          accountLoginProfileDAO.save(account);
        }
        catch (DataAccessException ex) {
          LOGGER.error("Exception occurred while saving Account - " +
                       account.getName());
          return accountLoginProfileResponseGenerator.internalServerError();
        }
      } else {
        LOGGER.error("LoginProfile already exists for Account" +
                     account.getName());
        return accountLoginProfileResponseGenerator.entityAlreadyExists();
      }
    }

    return accountLoginProfileResponseGenerator.generateCreateResponse(account);
  }

  /**
    * Below method will return login profile of the Account requested
    */
  @Override public ServerResponse list() {
    Account account = null;
    ServerResponse response = null;
    try {
      account = accountDAO.find(requestor.getAccount().getName());
      if (account.exists() && account.getPassword() != null) {
        response =
            accountLoginProfileResponseGenerator.generateGetResponse(account);
      } else {
        response = accountLoginProfileResponseGenerator.noSuchEntity();
      }
    }
    catch (DataAccessException ex) {
      response = accountLoginProfileResponseGenerator.internalServerError();
    }
    LOGGER.debug("Returned response is  - " + response);
    return response;
  }

  /**
   * Below method will update login profile of requested user.
   */
  @Override public ServerResponse update() throws DataAccessException {
    Account account = null;
    ServerResponse response = null;
    try {
      account = accountDAO.find(requestor.getAccount().getName());

      if (!account.exists()) {
        LOGGER.error("Account [" + requestor.getAccount().getName() +
                     "] does not exists");
        response = accountResponseGenerator.noSuchEntity();
      } else {
        if (account.getPassword() == null &&
            (account.getProfileCreateDate() == null ||
             account.getProfileCreateDate().isEmpty())) {

          String errorMessage = "LoginProfile not created for account - " +
                                requestor.getAccount().getName();
          LOGGER.error(errorMessage);
          response = accountResponseGenerator.noSuchEntity(errorMessage);

        } else {

          if (requestBody.get("Password") != null) {
            // Validate new password as per password policy
            if (!S3ParameterValidatorUtil.validatePasswordPolicy(
                     requestBody.get("Password"))) {
              LOGGER.error(
                  "Password does not conform to the account password policy");
              return accountResponseGenerator.passwordPolicyVoilation();
            }
            account.setPassword(requestBody.get("Password"));
            LOGGER.info("Updating old password with new password");
          }

          if (requestBody.get("PasswordResetRequired") != null) {
            account.setPwdResetRequired(
                requestBody.get("PasswordResetRequired").toUpperCase());
            LOGGER.info("Updating password reset required flag");
          }
          accountLoginProfileDAO.save(account);
          response =
              accountLoginProfileResponseGenerator.generateUpdateResponse();
        }
      }
    }
    catch (DataAccessException ex) {
      LOGGER.error("Exception occurred while doing ldap operation for user - " +
                   requestor.getAccount().getName());
      response = accountLoginProfileResponseGenerator.internalServerError();
    }
    return response;
  }
}
