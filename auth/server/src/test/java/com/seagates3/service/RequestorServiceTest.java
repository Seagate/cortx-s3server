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

package com.seagates3.service;

import static org.hamcrest.core.StringContains.containsString;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertThat;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.powermock.api.mockito.PowerMockito.doReturn;
import static org.powermock.api.mockito.PowerMockito.mockStatic;
import static org.powermock.api.mockito.PowerMockito.spy;
import static org.powermock.api.mockito.PowerMockito.whenNew;

import java.util.HashMap;
import java.util.Map;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.powermock.api.mockito.mockpolicies.Slf4jMockPolicy;
import org.powermock.core.classloader.annotations.MockPolicy;
import org.powermock.core.classloader.annotations.PowerMockIgnore;
import org.powermock.core.classloader.annotations.PrepareForTest;
import org.powermock.modules.junit4.PowerMockRunner;
import org.powermock.reflect.internal.WhiteboxImpl;
import org.slf4j.LoggerFactory;

import com.seagates3.authentication.ClientRequestToken;
import com.seagates3.authserver.AuthServerConfig;
import com.seagates3.dao.AccessKeyDAO;
import com.seagates3.dao.DAODispatcher;
import com.seagates3.dao.DAOResource;
import com.seagates3.dao.RequestorDAO;
import com.seagates3.exception.DataAccessException;
import com.seagates3.exception.InternalServerException;
import com.seagates3.exception.InvalidAccessKeyException;
import com.seagates3.exception.InvalidRequestorException;
import com.seagates3.model.AccessKey;
import com.seagates3.model.GlobalData;
import com.seagates3.model.Requestor;
import com.seagates3.perf.S3Perf;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.ResponseGenerator;

@RunWith(PowerMockRunner.class)
    @PrepareForTest({RequestorService.class, DAODispatcher.class,
                     LoggerFactory.class,    S3Perf.class,
                     GlobalDataStore.class,  AuthServerConfig.class})
    @PowerMockIgnore("javax.management.*")
    @MockPolicy(Slf4jMockPolicy.class) public class RequestorServiceTest {

 private
  AccessKeyDAO accessKeyDAO;
 private
  ClientRequestToken clientRequestToken;
 private
  Requestor requestor;
 private
  RequestorDAO requestorDAO;
 private
  AccessKey accessKey;
 private
  ServerResponse serverResponse;

 private
  final String accessKeyID = "v_accessKeyId";

  @Before public void setUp() throws Exception {
    accessKeyDAO = mock(AccessKeyDAO.class);
    mockStatic(DAODispatcher.class);
    mockStatic(AuthServerConfig.class);
    when(DAODispatcher.getResourceDAO(DAOResource.ACCESS_KEY))
        .thenReturn(accessKeyDAO);
    when(AuthServerConfig.getCacheTimeout()).thenReturn(0);
    clientRequestToken = mock(ClientRequestToken.class);
    doReturn(accessKeyID).when(clientRequestToken).getAccessKeyId();

    S3Perf s3Perf = mock(S3Perf.class);
    whenNew(S3Perf.class).withNoArguments().thenReturn(s3Perf);
    accessKey = mock(AccessKey.class);
    requestorDAO = mock(RequestorDAO.class);
    requestor = mock(Requestor.class);
    mockStatic(GlobalDataStore.class);
    GlobalDataStore mockGlobalInstance = mock(GlobalDataStore.class);
    when(GlobalDataStore.getInstance()).thenReturn(mockGlobalInstance);
    when(mockGlobalInstance.getAuthenticationMap())
        .thenReturn(new HashMap<String, GlobalData>());
    ResponseGenerator mockResponseGenerator = mock(ResponseGenerator.class);
    whenNew(ResponseGenerator.class).withNoArguments().thenReturn(
        mockResponseGenerator);
    serverResponse = new ServerResponse();
    ServerResponse invalidClientTokenResponse = new ServerResponse();
    ServerResponse expiredCredResponse = new ServerResponse();
    ServerResponse inactiveAccessKeyResponse = new ServerResponse();
    ServerResponse invalidAccessKeyResponse = new ServerResponse();
    ServerResponse internalFailureResponse = new ServerResponse();
    serverResponse.setResponseBody("Success");
    invalidClientTokenResponse.setResponseBody("InvalidClientTokenId");
    expiredCredResponse.setResponseBody("ExpiredCredential");
    inactiveAccessKeyResponse.setResponseBody("InactiveAccessKey");
    invalidAccessKeyResponse.setResponseBody("InvalidAccessKeyId");
    internalFailureResponse.setResponseBody("InternalFailure");

    when(mockResponseGenerator.internalServerError())
        .thenReturn(serverResponse);
    when(mockResponseGenerator.inactiveAccessKey())
        .thenReturn(inactiveAccessKeyResponse);
    when(mockResponseGenerator.invalidClientTokenId())
        .thenReturn(invalidClientTokenResponse);
    when(mockResponseGenerator.expiredCredential())
        .thenReturn(expiredCredResponse);
    when(mockResponseGenerator.invalidAccessKey())
        .thenReturn(invalidAccessKeyResponse);
    when(mockResponseGenerator.internalServerError())
        .thenReturn(internalFailureResponse);
  }

  @Test public void getRequestorTest() throws Exception {
    when(accessKeyDAO.find(accessKeyID)).thenReturn(accessKey);
    spy(RequestorService.class);
    doReturn(Boolean.TRUE)
        .when(RequestorService.class, "validateAccessKey", accessKey);
    when(DAODispatcher.getResourceDAO(DAOResource.REQUESTOR))
        .thenReturn(requestorDAO);
    when(requestorDAO.find(accessKey)).thenReturn(requestor);
    doReturn(Boolean.TRUE).when(RequestorService.class, "validateRequestor",
                                requestor, clientRequestToken);

    Requestor result = RequestorService.getRequestor(clientRequestToken);

    assertEquals(requestor, result);
    verify(accessKeyDAO).find(accessKeyID);
    verify(requestorDAO).find(accessKey);
  }

  @Test(
      expected =
          InternalServerException
              .class) public void getRequestorTest_RequestorFindShouldThrowException()
      throws Exception {
    when(accessKeyDAO.find(accessKeyID)).thenReturn(accessKey);
    spy(RequestorService.class);
    doReturn(Boolean.TRUE)
        .when(RequestorService.class, "validateAccessKey", accessKey);
    when(DAODispatcher.getResourceDAO(DAOResource.REQUESTOR))
        .thenReturn(requestorDAO);
    when(requestorDAO.find(accessKey)).thenThrow(DataAccessException.class);
    RequestorService.getRequestor(clientRequestToken);
  }

  @Test(
      expected =
          InternalServerException
              .class) public void getRequestorTest_AccessKeyFindShouldThrowException()
      throws Exception {
    when(accessKeyDAO.find(accessKeyID)).thenThrow(DataAccessException.class);

    RequestorService.getRequestor(clientRequestToken);
  }

  @Test public void validateAccessKeyTest() throws Exception {
    when(accessKey.exists()).thenReturn(Boolean.TRUE);
    when(accessKey.isAccessKeyActive()).thenReturn(Boolean.TRUE);

    Boolean result = WhiteboxImpl.invokeMethod(RequestorService.class,
                                               "validateAccessKey", accessKey);

    assertTrue(result);
  }

  @Test public void validateAccessKeyTest_AccessKeyDoesNotExist()
      throws Exception {
    try {
      WhiteboxImpl.invokeMethod(RequestorService.class, "validateAccessKey",
                                accessKey);
      fail("Should throw InvalidAccessKeyException");
    }
    catch (InvalidAccessKeyException e) {
      assertThat(e.getMessage(), containsString("InvalidAccessKeyId"));
    }

    verify(accessKey, times(0)).isAccessKeyActive();
  }

  @Test public void validateAccessKeyTestInactiveAccessKey() throws Exception {
    when(accessKey.exists()).thenReturn(Boolean.TRUE);

    try {
      WhiteboxImpl.invokeMethod(RequestorService.class, "validateAccessKey",
                                accessKey);
      fail("Should throw InvalidAccessKeyException");
    }
    catch (InvalidAccessKeyException e) {
      assertThat(e.getMessage(), containsString("InactiveAccessKey"));
    }

    verify(accessKey, times(1)).exists();
    verify(accessKey, times(1)).isAccessKeyActive();
  }

  @Test public void validateRequestorTest() throws Exception {
    when(requestor.exists()).thenReturn(Boolean.TRUE);
    when(requestor.isFederatedUser()).thenReturn(Boolean.FALSE);
    when(requestor.getAccesskey()).thenReturn(accessKey);

    Boolean result =
        WhiteboxImpl.invokeMethod(RequestorService.class, "validateRequestor",
                                  requestor, clientRequestToken);

    assertTrue(result);
  }

  @Test public void validateRequestorTestRequestorDoesntExist()
      throws Exception {
    try {
      WhiteboxImpl.invokeMethod(RequestorService.class, "validateRequestor",
                                requestor, clientRequestToken);
      fail("Should throw exception if requestor doesn't exist.");
    }
    catch (InvalidRequestorException e) {
      assertThat(e.getMessage(), containsString("InternalFailure"));
    }

    verify(requestor, times(0)).isFederatedUser();
  }

  @Test public void validateRequestorTestRequstorIsFederatedUser()
      throws Exception {
    when(requestor.exists()).thenReturn(Boolean.TRUE);
    when(requestor.getAccesskey()).thenReturn(accessKey);
    when(requestor.isFederatedUser()).thenReturn(Boolean.TRUE);
    Map headers = mock(Map.class);
    when(clientRequestToken.getRequestHeaders()).thenReturn(headers);
    when(headers.get("X-Amz-Security-Token"))
        .thenReturn("AAAHVXNlclRrbgfOpSykBAXO7g");
    when(accessKey.getToken()).thenReturn("AAAHVXNlclRrbgfOpSykBAXO7g");
    when(accessKey.getExpiry()).thenReturn("9999-01-10T08:02:47.806-05:00");

    Boolean result =
        WhiteboxImpl.invokeMethod(RequestorService.class, "validateRequestor",
                                  requestor, clientRequestToken);

    assertTrue(result);
  }

  @Test public void validateRequestorTestRequstorIsFedUserInvalidClientTokenID()
      throws Exception {
    when(requestor.exists()).thenReturn(Boolean.TRUE);
    when(requestor.getAccesskey()).thenReturn(accessKey);
    when(requestor.isFederatedUser()).thenReturn(Boolean.TRUE);
    Map headers = mock(Map.class);
    when(clientRequestToken.getRequestHeaders()).thenReturn(headers);
    when(headers.get("X-Amz-Security-Token"))
        .thenReturn("AAAHVXNlclRrbgfOpSykBAXO7g");
    when(accessKey.getToken()).thenReturn("INVALID-TOKEN");
    try {
      WhiteboxImpl.invokeMethod(RequestorService.class, "validateRequestor",
                                requestor, clientRequestToken);
      fail("Should throw InvalidRequestorException if client token id is " +
           "invalid.");
    }
    catch (InvalidRequestorException e) {
      assertThat(e.getMessage(), containsString("InvalidClientTokenId"));
    }
  }

  @Test public void validateRequestorTestRequstorIsFedUserExpiredCredential()
      throws Exception {
    when(requestor.exists()).thenReturn(Boolean.TRUE);
    when(requestor.getAccesskey()).thenReturn(accessKey);
    when(requestor.isFederatedUser()).thenReturn(Boolean.TRUE);
    Map headers = mock(Map.class);
    when(clientRequestToken.getRequestHeaders()).thenReturn(headers);
    when(headers.get("X-Amz-Security-Token"))
        .thenReturn("AAAHVXNlclRrbgfOpSykBAXO7g");
    when(accessKey.getToken()).thenReturn("AAAHVXNlclRrbgfOpSykBAXO7g");
    when(accessKey.getExpiry()).thenReturn("1970-01-10T08:02:47.806-05:00");
    try {
      WhiteboxImpl.invokeMethod(RequestorService.class, "validateRequestor",
                                requestor, clientRequestToken);
      fail("Should throw InvalidRequestorException if federated credentials " +
           "have expired");
    }
    catch (InvalidRequestorException e) {
      assertThat(e.getMessage(), containsString("ExpiredCredential"));
    }
  }
}

