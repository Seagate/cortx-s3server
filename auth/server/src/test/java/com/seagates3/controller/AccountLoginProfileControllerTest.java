/*
 * COPYRIGHT 2019 SEAGATE LLC
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
 * Original author: Abhilekh Mustapure <abhilekh.mustapure@seagate.com>
 * Original creation date: 27-June-2019
 */
package com.seagates3.controller;

import com.seagates3.dao.AccountDAO;
import com.seagates3.dao.DAODispatcher;
import com.seagates3.dao.DAOResource;
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Account;
import com.seagates3.model.Requestor;
import com.seagates3.response.ServerResponse;
import com.seagates3.util.KeyGenUtil;
import io.netty.handler.codec.http.HttpResponseStatus;

import java.util.Map;
import java.util.TreeMap;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mockito;
import org.powermock.api.mockito.PowerMockito;
import org.powermock.core.classloader.annotations.PowerMockIgnore;
import org.powermock.core.classloader.annotations.PrepareForTest;
import org.powermock.modules.junit4.PowerMockRunner;

@RunWith(PowerMockRunner.class)
    @PrepareForTest({DAODispatcher.class, KeyGenUtil.class,
                     AccountController.class})
    @PowerMockIgnore(
        {"javax.management.*"}) public class AccountLoginProfileControllerTest {

 private
  AccountLoginProfileController accountLoginProfileController;
 private
  final String ACCOUNT_NAME = "s3test";
 private
  final String ACCOUNT_ID = "12345";
 private
  final Account ACCOUNT;
 private
  AccountDAO mockAccountDao;
 private
  final String GET_RESOURCE_DAO = "getResourceDAO";
 private
  Map<String, String> requestBodyObj = null;
 private
  Requestor requestorObj = null;

 public
  AccountLoginProfileControllerTest() {

    ACCOUNT = new Account();
    ACCOUNT.setId(ACCOUNT_ID);
    ACCOUNT.setName(ACCOUNT_NAME);
  }

  @Before public void setUp() throws Exception {
    PowerMockito.mockStatic(DAODispatcher.class);
    mockAccountDao = Mockito.mock(AccountDAO.class);
    requestorObj = new Requestor();
    requestorObj.setAccount(ACCOUNT);
    requestBodyObj = new TreeMap<>(String.CASE_INSENSITIVE_ORDER);
    requestBodyObj.put("AccountName", "s3test");
  }

  @Test public void GetUserLoginProfile_Sucessful_Api_Response()
      throws Exception {

    ACCOUNT.setId(ACCOUNT_ID);
    ACCOUNT.setName(ACCOUNT_NAME);
    ACCOUNT.setPassword("password");
    ACCOUNT.setPwdResetRequired("false");
    ACCOUNT.setProfileCreateDate("2019-06-16 15:38:53+00:00");
    PowerMockito.mockStatic(DAODispatcher.class);
    PowerMockito.doReturn(mockAccountDao)
        .when(DAODispatcher.class, GET_RESOURCE_DAO, DAOResource.ACCOUNT);
    Mockito.when(mockAccountDao.find(ACCOUNT_NAME)).thenReturn(ACCOUNT);
    accountLoginProfileController = Mockito.spy(
        new AccountLoginProfileController(requestorObj, requestBodyObj));
    ServerResponse response = accountLoginProfileController.list();
    Assert.assertEquals(HttpResponseStatus.OK, response.getResponseStatus());
  }

  @Test public void GetUserLoginProfile_DataAccessException_Response()
      throws Exception {
    PowerMockito.mockStatic(DAODispatcher.class);
    PowerMockito.doReturn(mockAccountDao)
        .when(DAODispatcher.class, GET_RESOURCE_DAO, DAOResource.ACCOUNT);
    Mockito.doThrow(new DataAccessException("failed to search user.\n"))
        .when(mockAccountDao)
        .find(ACCOUNT_NAME);
    accountLoginProfileController = Mockito.spy(
        new AccountLoginProfileController(requestorObj, requestBodyObj));
    ServerResponse response = accountLoginProfileController.list();
    Assert.assertEquals(HttpResponseStatus.INTERNAL_SERVER_ERROR,
                        response.getResponseStatus());
  }

  @Test public void GetUserLoginProfile_NoSuchEntity_Response()
      throws Exception {
    Account account = new Account();
    account.setId(ACCOUNT_ID);
    account.setName(ACCOUNT_NAME);

    PowerMockito.mockStatic(DAODispatcher.class);
    PowerMockito.doReturn(mockAccountDao)
        .when(DAODispatcher.class, GET_RESOURCE_DAO, DAOResource.ACCOUNT);
    Mockito.when(mockAccountDao.find(ACCOUNT_NAME)).thenReturn(account);
    accountLoginProfileController = Mockito.spy(
        new AccountLoginProfileController(requestorObj, requestBodyObj));
    ServerResponse response = accountLoginProfileController.list();
    Assert.assertEquals(HttpResponseStatus.UNAUTHORIZED,
                        response.getResponseStatus());
  }
}
