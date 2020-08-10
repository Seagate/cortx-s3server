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

package com.seagates3.s3service;

import static org.junit.Assert.*;

import java.io.IOException;

import org.apache.http.HttpStatus;
import org.apache.logging.log4j.core.config.Configurator;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.powermock.api.mockito.PowerMockito;
import org.powermock.api.mockito.mockpolicies.Slf4jMockPolicy;
import org.powermock.core.classloader.annotations.MockPolicy;
import org.powermock.core.classloader.annotations.PowerMockIgnore;
import org.powermock.core.classloader.annotations.PrepareForTest;
import org.powermock.modules.junit4.PowerMockRunner;
import org.slf4j.LoggerFactory;

import com.seagates3.authserver.AuthServerConfig;
import com.seagates3.exception.S3RequestInitializationException;
import com.seagates3.response.ServerResponse;

import io.netty.handler.codec.http.HttpResponseStatus;

import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyInt;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.powermock.api.mockito.PowerMockito.doNothing;
import static org.powermock.api.mockito.PowerMockito.doReturn;
import static org.powermock.api.mockito.PowerMockito.doThrow;
import static org.powermock.api.mockito.PowerMockito.mockStatic;
import static org.powermock.api.mockito.PowerMockito.spy;
import static org.powermock.api.mockito.PowerMockito.verifyPrivate;
import static org.powermock.api.mockito.PowerMockito.verifyStatic;
import static org.powermock.api.mockito.PowerMockito.whenNew;

@RunWith(PowerMockRunner.class)
@PrepareForTest({S3RestClient.class, S3AccountNotifier.class,
                                     AuthServerConfig.class})
@MockPolicy(Slf4jMockPolicy.class)
@PowerMockIgnore({"javax.management.*"})

public class S3AccountNotifierTest {

    @Test
    public void testNotifyNewAccountCreated() throws Exception,
                                                  IOException {
        mockStatic(AuthServerConfig.class);
        doReturn("s3endpoint").when(AuthServerConfig.class,
                                      "getDefaultEndpoint");
        doReturn(true).when(AuthServerConfig.class,
                                        "isEnableHttpsToS3");

        S3HttpResponse s3Resp = new S3HttpResponse();

        s3Resp.setHttpCode(HttpStatus.SC_CREATED);

        S3RestClient s3Cli = mock(S3RestClient.class);

        whenNew(S3RestClient.class).withNoArguments().thenReturn(s3Cli);
        doReturn(s3Resp).when(s3Cli).postRequest();

        S3AccountNotifier s3AccountNotifier = new S3AccountNotifier();
        ServerResponse resp = s3AccountNotifier.notifyNewAccount("12345",
        "AKIAJTYX36YCKQSAJT7Q", "A6k2z84BqwXmee4WUUS2oWwM/tha7Wrd4Hc/8yRt");

        assertEquals(HttpResponseStatus.OK, resp.getResponseStatus());
    }

    @Test
    public void testNotifyNewAccountInternalError() throws Exception,
                                                        IOException {
        mockStatic(AuthServerConfig.class);
        doReturn("s3endpoint").when(AuthServerConfig.class,
                                      "getDefaultEndpoint");
        doReturn(true).when(AuthServerConfig.class,
                                        "isEnableHttpsToS3");
        doReturn("0000").when(AuthServerConfig.class, "getReqId");
        S3HttpResponse s3Resp = new S3HttpResponse();

        s3Resp.setHttpCode(HttpStatus.SC_INTERNAL_SERVER_ERROR);

        S3RestClient s3Cli = mock(S3RestClient.class);

        whenNew(S3RestClient.class).withNoArguments().thenReturn(s3Cli);
        doReturn(s3Resp).when(s3Cli).postRequest();

        S3AccountNotifier s3AccountNotifier = new S3AccountNotifier();
        ServerResponse resp = s3AccountNotifier.notifyNewAccount("12345",
        "AKIAJTYX36YCKQSAJT7Q", "A6k2z84BqwXmee4WUUS2oWwM/tha7Wrd4Hc/8yRt");

        assertEquals(HttpResponseStatus.INTERNAL_SERVER_ERROR,
                                    resp.getResponseStatus());
    }

    @Test
    public void testNotifyNewAccountError() throws Exception,
                                                IOException {
        mockStatic(AuthServerConfig.class);
        doReturn("s3endpoint").when(AuthServerConfig.class,
                                      "getDefaultEndpoint");
        doReturn(true).when(AuthServerConfig.class,
                                       "isEnableHttpsToS3");
        doReturn("0000").when(AuthServerConfig.class, "getReqId");
        S3HttpResponse s3Resp = new S3HttpResponse();

        s3Resp.setHttpCode(HttpStatus.SC_INTERNAL_SERVER_ERROR);

        S3RestClient s3Cli = mock(S3RestClient.class);

        whenNew(S3RestClient.class).withNoArguments().thenReturn(s3Cli);
        doReturn(s3Resp).when(s3Cli).postRequest();

        S3AccountNotifier s3AccountNotifier = new S3AccountNotifier();
        ServerResponse resp = s3AccountNotifier.notifyNewAccount("12345",
        "AKIAJTYX36YCKQSAJT7Q", "A6k2z84BqwXmee4WUUS2oWwM/tha7Wrd4Hc/8yRt");

        assertEquals(HttpResponseStatus.INTERNAL_SERVER_ERROR,
                                    resp.getResponseStatus());
    }

    @Test
    public void testNotifyNewAccountInvalidAcct() throws Exception,
                                                      IOException {
        mockStatic(AuthServerConfig.class);
        doReturn("s3endpoint").when(AuthServerConfig.class,
                                      "getDefaultEndpoint");
        doReturn(true).when(AuthServerConfig.class,
                                       "isEnableHttpsToS3");
        doReturn("0000").when(AuthServerConfig.class, "getReqId");
        S3HttpResponse s3Resp = new S3HttpResponse();

        s3Resp.setHttpCode(HttpStatus.SC_BAD_REQUEST);

        S3RestClient s3Cli = mock(S3RestClient.class);

        whenNew(S3RestClient.class).withNoArguments().thenReturn(s3Cli);
        doReturn(s3Resp).when(s3Cli).postRequest();

        S3AccountNotifier s3AccountNotifier = new S3AccountNotifier();
        ServerResponse resp = s3AccountNotifier.notifyNewAccount("12345",
        "AKIAJTYX36YCKQSAJT7Q", "A6k2z84BqwXmee4WUUS2oWwM/tha7Wrd4Hc/8yRt");

        assertEquals(HttpResponseStatus.INTERNAL_SERVER_ERROR,
                                    resp.getResponseStatus());
    }

    @Test
    public void testNotifyNewAccountIOException() throws Exception,
                                                      IOException {
        mockStatic(AuthServerConfig.class);
        doReturn("s3endpoint").when(AuthServerConfig.class,
                                      "getDefaultEndpoint");
        doReturn(true).when(AuthServerConfig.class,
                                       "isEnableHttpsToS3");
        doReturn("0000").when(AuthServerConfig.class, "getReqId");
        S3HttpResponse s3Resp = new S3HttpResponse();

        s3Resp.setHttpCode(HttpStatus.SC_BAD_REQUEST);

        S3RestClient s3Cli = mock(S3RestClient.class);
        IOException e = new IOException("S3 Connect Failed");
        whenNew(S3RestClient.class).withNoArguments().thenReturn(s3Cli);
        when(s3Cli.postRequest()).thenThrow(e);

        S3AccountNotifier s3AccountNotifier = new S3AccountNotifier();
        ServerResponse resp = s3AccountNotifier.notifyNewAccount("12345",
        "AKIAJTYX36YCKQSAJT7Q", "A6k2z84BqwXmee4WUUS2oWwM/tha7Wrd4Hc/8yRt");

        assertEquals(HttpResponseStatus.INTERNAL_SERVER_ERROR,
                                    resp.getResponseStatus());
    }

    @Test
    public void testNotifyNewAccountRequestException() throws Exception,
                                                           IOException {
        mockStatic(AuthServerConfig.class);
        doReturn("s3endpoint").when(AuthServerConfig.class,
                                      "getDefaultEndpoint");
        doReturn(true).when(AuthServerConfig.class,
                                       "isEnableHttpsToS3");
        doReturn("0000").when(AuthServerConfig.class, "getReqId");
        S3HttpResponse s3Resp = new S3HttpResponse();

        s3Resp.setHttpCode(HttpStatus.SC_NO_CONTENT);

        S3RestClient s3Cli = mock(S3RestClient.class);
        S3RequestInitializationException e =
                new S3RequestInitializationException("Invalid URL");
        whenNew(S3RestClient.class).withNoArguments().thenReturn(s3Cli);
        when(s3Cli.postRequest()).thenThrow(e);

        S3AccountNotifier s3AccountNotifier = new S3AccountNotifier();
        ServerResponse resp = s3AccountNotifier.notifyNewAccount("12345",
        "AKIAJTYX36YCKQSAJT7Q", "A6k2z84BqwXmee4WUUS2oWwM/tha7Wrd4Hc/8yRt");

        assertEquals(HttpResponseStatus.INTERNAL_SERVER_ERROR,
                                    resp.getResponseStatus());
    }

    @Test
    public void testNotifyDeleteAccountSucess() throws Exception,
                                                    IOException {
        mockStatic(AuthServerConfig.class);
        doReturn("s3endpoint").when(AuthServerConfig.class,
                                      "getDefaultEndpoint");
        doReturn(true).when(AuthServerConfig.class,
                                       "isEnableHttpsToS3");

        S3HttpResponse s3Resp = new S3HttpResponse();

        s3Resp.setHttpCode(HttpStatus.SC_NO_CONTENT);

        S3RestClient s3Cli = mock(S3RestClient.class);

        whenNew(S3RestClient.class).withNoArguments().thenReturn(s3Cli);
        doReturn(s3Resp).when(s3Cli).deleteRequest();

        S3AccountNotifier s3AccountNotifier = new S3AccountNotifier();
        ServerResponse resp = s3AccountNotifier.notifyDeleteAccount(
            "12345", "AKIAJTYX36YCKQSAJT7Q",
            "A6k2z84BqwXmee4WUUS2oWwM/tha7Wrd4Hc/8yRt", null);

        assertEquals(HttpResponseStatus.OK, resp.getResponseStatus());
    }

    @Test
    public void testNotifyDeleteAccountInternalError() throws Exception,
                                                           IOException {
        mockStatic(AuthServerConfig.class);
        doReturn("s3endpoint").when(AuthServerConfig.class,
                                      "getDefaultEndpoint");
        doReturn(true).when(AuthServerConfig.class,
                                     "isEnableHttpsToS3");
        doReturn("0000").when(AuthServerConfig.class, "getReqId");
        S3HttpResponse s3Resp = new S3HttpResponse();

        s3Resp.setHttpCode(HttpStatus.SC_INTERNAL_SERVER_ERROR);

        S3RestClient s3Cli = mock(S3RestClient.class);

        whenNew(S3RestClient.class).withNoArguments().thenReturn(s3Cli);
        doReturn(s3Resp).when(s3Cli).deleteRequest();

        S3AccountNotifier s3AccountNotifier = new S3AccountNotifier();
        ServerResponse resp = s3AccountNotifier.notifyDeleteAccount(
            "12345", "AKIAJTYX36YCKQSAJT7Q",
            "A6k2z84BqwXmee4WUUS2oWwM/tha7Wrd4Hc/8yRt", null);

        assertEquals(HttpResponseStatus.INTERNAL_SERVER_ERROR,
                                              resp.getResponseStatus());
    }

    @Test
    public void testNotifyDeleteAccountNotEmpty() throws Exception,
                                                      IOException {
        mockStatic(AuthServerConfig.class);
        doReturn("s3endpoint").when(AuthServerConfig.class,
                                      "getDefaultEndpoint");
        doReturn(true).when(AuthServerConfig.class,
                                       "isEnableHttpsToS3");
        doReturn("0000").when(AuthServerConfig.class, "getReqId");
        S3HttpResponse s3Resp = new S3HttpResponse();

        s3Resp.setHttpCode(HttpStatus.SC_CONFLICT);

        S3RestClient s3Cli = mock(S3RestClient.class);

        whenNew(S3RestClient.class).withNoArguments().thenReturn(s3Cli);
        doReturn(s3Resp).when(s3Cli).deleteRequest();

        S3AccountNotifier s3AccountNotifier = new S3AccountNotifier();
        ServerResponse resp = s3AccountNotifier.notifyDeleteAccount(
            "12345", "AKIAJTYX36YCKQSAJT7Q",
            "A6k2z84BqwXmee4WUUS2oWwM/tha7Wrd4Hc/8yRt", null);

        assertEquals(HttpResponseStatus.CONFLICT, resp.getResponseStatus());
    }

    @Test
    public void testNotifyDeleteAccountInvalidAcct() throws Exception,
                                                         IOException {
        mockStatic(AuthServerConfig.class);
        doReturn("s3endpoint").when(AuthServerConfig.class,
                                      "getDefaultEndpoint");
        doReturn(true).when(AuthServerConfig.class,
                                       "isEnableHttpsToS3");
        doReturn("0000").when(AuthServerConfig.class, "getReqId");
        S3HttpResponse s3Resp = new S3HttpResponse();

        s3Resp.setHttpCode(HttpStatus.SC_BAD_REQUEST);

        S3RestClient s3Cli = mock(S3RestClient.class);

        whenNew(S3RestClient.class).withNoArguments().thenReturn(s3Cli);
        doReturn(s3Resp).when(s3Cli).deleteRequest();

        S3AccountNotifier s3AccountNotifier = new S3AccountNotifier();
        ServerResponse resp = s3AccountNotifier.notifyDeleteAccount(
            "12345", "AKIAJTYX36YCKQSAJT7Q",
            "A6k2z84BqwXmee4WUUS2oWwM/tha7Wrd4Hc/8yRt", null);

        assertEquals(HttpResponseStatus.INTERNAL_SERVER_ERROR,
                                    resp.getResponseStatus());
    }

    @Test
    public void testNotifyNewDeleteIOException() throws Exception,
                                                     IOException {
        mockStatic(AuthServerConfig.class);
        doReturn("s3endpoint").when(AuthServerConfig.class,
                                      "getDefaultEndpoint");
        doReturn(true).when(AuthServerConfig.class,
                                       "isEnableHttpsToS3");
        doReturn("0000").when(AuthServerConfig.class, "getReqId");
        S3HttpResponse s3Resp = new S3HttpResponse();

        s3Resp.setHttpCode(HttpStatus.SC_BAD_REQUEST);

        S3RestClient s3Cli = mock(S3RestClient.class);
        IOException e = new IOException("S3 Connect Failed");
        whenNew(S3RestClient.class).withNoArguments().thenReturn(s3Cli);
        when(s3Cli.deleteRequest()).thenThrow(e);

        S3AccountNotifier s3AccountNotifier = new S3AccountNotifier();
        ServerResponse resp = s3AccountNotifier.notifyDeleteAccount(
            "12345", "AKIAJTYX36YCKQSAJT7Q",
            "A6k2z84BqwXmee4WUUS2oWwM/tha7Wrd4Hc/8yRt", null);

        assertEquals(HttpResponseStatus.INTERNAL_SERVER_ERROR,
                                    resp.getResponseStatus());
    }

    @Test
    public void testNotifyDeleteAccountRequestException() throws Exception,
                                                              IOException {
        mockStatic(AuthServerConfig.class);
        doReturn("s3endpoint").when(AuthServerConfig.class,
                                      "getDefaultEndpoint");
        doReturn(true).when(AuthServerConfig.class,
                                       "isEnableHttpsToS3");
        doReturn("0000").when(AuthServerConfig.class, "getReqId");
        S3HttpResponse s3Resp = new S3HttpResponse();

        s3Resp.setHttpCode(HttpStatus.SC_BAD_REQUEST);

        S3RestClient s3Cli = mock(S3RestClient.class);
        S3RequestInitializationException e =
                new S3RequestInitializationException("Invalid URL");
        whenNew(S3RestClient.class).withNoArguments().thenReturn(s3Cli);
        when(s3Cli.deleteRequest()).thenThrow(e);

        S3AccountNotifier s3AccountNotifier = new S3AccountNotifier();
        ServerResponse resp = s3AccountNotifier.notifyDeleteAccount(
            "12345", "AKIAJTYX36YCKQSAJT7Q",
            "A6k2z84BqwXmee4WUUS2oWwM/tha7Wrd4Hc/8yRt", null);

        assertEquals(HttpResponseStatus.INTERNAL_SERVER_ERROR,
                                    resp.getResponseStatus());
    }
}


