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

package com.seagates3.authentication;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;
import static org.powermock.api.mockito.PowerMockito.doReturn;

import java.util.Date;
import java.util.HashMap;
import java.util.Map;

import org.joda.time.DateTime;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.powermock.api.mockito.PowerMockito;
import org.powermock.api.mockito.mockpolicies.Slf4jMockPolicy;
import org.powermock.core.classloader.annotations.MockPolicy;
import org.powermock.core.classloader.annotations.PowerMockIgnore;
import org.powermock.core.classloader.annotations.PrepareForTest;
import org.powermock.modules.junit4.PowerMockRunner;
import org.powermock.reflect.internal.WhiteboxImpl;

import com.seagates3.model.Requestor;
import com.seagates3.response.ServerResponse;
import com.seagates3.util.DateUtil;

import io.netty.handler.codec.http.HttpResponseStatus;

@PowerMockIgnore({"javax.management.*"}) @RunWith(PowerMockRunner.class)
    @PrepareForTest({SignatureValidator.class, DateTime.class, DateUtil.class})
    @MockPolicy(Slf4jMockPolicy.class) public class SignatureValidatorTest {

    private ClientRequestToken clientRequestToken;
    private SignatureValidator signatureValidator;

    @Before
    public void setUp() {
        clientRequestToken = mock(ClientRequestToken.class);
        signatureValidator = new SignatureValidator();
    }

    @Test
    public void validateTest_ShouldReturnOk() throws Exception {
        // Arrange
        Requestor requestor = mock(Requestor.class);

        AWSSign awsSign = mock(AWSSign.class);
        SignatureValidator signatureValidatorSpy = PowerMockito.spy(signatureValidator);
        doReturn(awsSign).when(signatureValidatorSpy, "getSigner", clientRequestToken);
        when(awsSign.authenticate(clientRequestToken, requestor)).thenReturn(Boolean.TRUE);

        // when(awsSign.validateDate(Mockito.any(ClientRequestToken.class),
        //                          Mockito.any(Date.class)))
        //    .thenReturn(Boolean.TRUE);
        ServerResponse serverResponse = mock(ServerResponse.class);
        when(serverResponse.getResponseStatus())
            .thenReturn(HttpResponseStatus.OK);
        doReturn(serverResponse).when(signatureValidatorSpy,
                                      "validateSignatureDate",
                                      clientRequestToken, awsSign);

        // Act
        ServerResponse response = signatureValidatorSpy.validate(clientRequestToken, requestor);

        // Verify
        assertEquals(HttpResponseStatus.OK, response.getResponseStatus());
    }

    @Test
    public void validateTest_ShouldReturnSignatureDoesNotMatch() throws Exception {
        // Arrange
        Requestor requestor = mock(Requestor.class);

        AWSSign awsSign = mock(AWSSign.class);
        SignatureValidator signatureValidatorSpy = PowerMockito.spy(signatureValidator);
        doReturn(awsSign).when(signatureValidatorSpy, "getSigner", clientRequestToken);
        when(awsSign.authenticate(clientRequestToken, requestor)).thenReturn(Boolean.FALSE);

        ServerResponse serverResponse = mock(ServerResponse.class);
        when(serverResponse.getResponseStatus())
            .thenReturn(HttpResponseStatus.OK);
        doReturn(serverResponse).when(signatureValidatorSpy,
                                      "validateSignatureDate",
                                      clientRequestToken, awsSign);

        // Act
        ServerResponse response = signatureValidatorSpy.validate(clientRequestToken, requestor);

        // Verify
        assertEquals(HttpResponseStatus.UNAUTHORIZED, response.getResponseStatus());
    }

    @Test public void
    validateSignatureDateTest_ShouldReturnInvalidSignatureDate_WhenEmptyDate()
        throws Exception {
      // Arrange
      Requestor requestor = mock(Requestor.class);

      AWSSign awsSign = mock(AWSSign.class);
      SignatureValidator signatureValidatorSpy =
          PowerMockito.spy(signatureValidator);
      doReturn(awsSign)
          .when(signatureValidatorSpy, "getSigner", clientRequestToken);
      when(awsSign.authenticate(clientRequestToken, requestor))
          .thenReturn(Boolean.FALSE);

      ServerResponse serverResponse = mock(ServerResponse.class);
      when(serverResponse.getResponseStatus())
          .thenReturn(HttpResponseStatus.OK);

      Map<String, String> requestHeaders = new HashMap<>();
      requestHeaders.put("x-amz-date", "");
      when(clientRequestToken.getRequestHeaders()).thenReturn(requestHeaders);

      // Act
      ServerResponse response = WhiteboxImpl.invokeMethod(
          signatureValidatorSpy, "validateSignatureDate", clientRequestToken,
          awsSign);

      // Verify
      assertEquals(HttpResponseStatus.FORBIDDEN, response.getResponseStatus());
    }

    @Test public void
    validateSignatureDateTest_ShouldReturnInvalidSignatureDate_WhenBadDate()
        throws Exception {
      // Arrange
      Requestor requestor = mock(Requestor.class);

      AWSSign awsSign = mock(AWSSign.class);
      SignatureValidator signatureValidatorSpy =
          PowerMockito.spy(signatureValidator);
      doReturn(awsSign)
          .when(signatureValidatorSpy, "getSigner", clientRequestToken);
      when(awsSign.authenticate(clientRequestToken, requestor))
          .thenReturn(Boolean.FALSE);

      ServerResponse serverResponse = mock(ServerResponse.class);
      when(serverResponse.getResponseStatus())
          .thenReturn(HttpResponseStatus.OK);

      Map<String, String> requestHeaders = new HashMap<>();
      requestHeaders.put("x-amz-date", "BAD DATE");
      when(clientRequestToken.getRequestHeaders()).thenReturn(requestHeaders);

      // Act
      ServerResponse response = WhiteboxImpl.invokeMethod(
          signatureValidatorSpy, "validateSignatureDate", clientRequestToken,
          awsSign);

      // Verify
      assertEquals(HttpResponseStatus.FORBIDDEN, response.getResponseStatus());
    }

    @Test public void validateSignatureDateTest_ShouldReturnOK()
        throws Exception {
      // Arrange
      Requestor requestor = mock(Requestor.class);

      AWSSign awsSign = mock(AWSSign.class);
      SignatureValidator signatureValidatorSpy =
          PowerMockito.spy(signatureValidator);
      doReturn(awsSign)
          .when(signatureValidatorSpy, "getSigner", clientRequestToken);
      when(awsSign.authenticate(clientRequestToken, requestor))
          .thenReturn(Boolean.FALSE);

      ServerResponse serverResponse = mock(ServerResponse.class);
      when(serverResponse.getResponseStatus())
          .thenReturn(HttpResponseStatus.OK);

      DateTime dtMock = PowerMockito.mock(DateTime.class);
      PowerMockito.when(dtMock.getMillis()).thenReturn(1576673044777L);
      PowerMockito.mockStatic(DateUtil.class);
      PowerMockito.when(DateUtil.class, "getCurrentDateTime")
          .thenReturn(dtMock);
      PowerMockito.when(DateUtil.class, "parseDateString", "20191218T181100Z",
                        "yyyyMMdd'T'HHmmss'Z'")
          .thenReturn(new Date(1576673044777L));

      Map<String, String> requestHeaders = new HashMap<>();
      requestHeaders.put("x-amz-date", "20191218T181100Z");
      when(clientRequestToken.getRequestHeaders()).thenReturn(requestHeaders);

      // Act
      ServerResponse response = WhiteboxImpl.invokeMethod(
          signatureValidatorSpy, "validateSignatureDate", clientRequestToken,
          awsSign);

      // Verify
      assertEquals(HttpResponseStatus.OK, response.getResponseStatus());
    }

    @Test
    public void getSignerTest_AWSSignV2() throws Exception {
        // Arrange
        when(clientRequestToken.getSignVersion()).thenReturn(ClientRequestToken.AWSSigningVersion.V2);

        // Act
        AWSSign awsSign = WhiteboxImpl.invokeMethod(signatureValidator, "getSigner", clientRequestToken);

        // Verify
        assertTrue(awsSign instanceof AWSV2Sign);
        assertFalse(awsSign instanceof AWSV4Sign);
    }

    @Test
    public void getSignerTest_AWSSignV4() throws Exception {
        // Arrange
        when(clientRequestToken.getSignVersion()).thenReturn(ClientRequestToken.AWSSigningVersion.V4);

        // Act
        AWSSign awsSign = WhiteboxImpl.invokeMethod(signatureValidator, "getSigner", clientRequestToken);

        // Verify
        assertTrue(awsSign instanceof AWSV4Sign);
        assertFalse(awsSign instanceof AWSV2Sign);
    }
}
