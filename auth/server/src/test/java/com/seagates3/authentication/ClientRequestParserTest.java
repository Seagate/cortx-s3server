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
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.mockito.Mockito.mock;
import static org.powermock.api.mockito.PowerMockito.mockStatic;
import static org.powermock.api.mockito.PowerMockito.when;

import java.util.Map;
import java.util.TreeMap;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mockito;
import org.powermock.api.mockito.PowerMockito;
import org.powermock.api.mockito.mockpolicies.Slf4jMockPolicy;
import org.powermock.core.classloader.annotations.MockPolicy;
import org.powermock.core.classloader.annotations.PowerMockIgnore;
import org.powermock.core.classloader.annotations.PrepareForTest;
import org.powermock.modules.junit4.PowerMockRunner;

import com.seagates3.authserver.AuthServerConfig;
import com.seagates3.aws.AWSV2RequestHelper;
import com.seagates3.aws.AWSV4RequestHelper;
import com.seagates3.exception.InvalidAccessKeyException;
import com.seagates3.exception.InvalidArgumentException;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.ResponseGenerator;

import io.netty.handler.codec.http.DefaultFullHttpRequest;
import io.netty.handler.codec.http.FullHttpRequest;
import io.netty.handler.codec.http.HttpMethod;
import io.netty.handler.codec.http.HttpResponseStatus;
import io.netty.handler.codec.http.HttpVersion;

@PowerMockIgnore({"javax.management.*"}) @RunWith(PowerMockRunner.class)
    @PrepareForTest({AuthServerConfig.class, ClientRequestParser.class})
    @MockPolicy(Slf4jMockPolicy.class) public class ClientRequestParserTest {

    private FullHttpRequest httpRequest;
    private Map<String, String> requestBody;

    @Before
    public void setUp() {
        String[] endpoints = {"s3-us.seagate.com",
                "s3-europe.seagate.com", "s3-asia.seagate.com"};
        requestBody = new TreeMap<>();
        httpRequest = mock(FullHttpRequest.class);
        mockStatic(AuthServerConfig.class);
        when(AuthServerConfig.getEndpoints()).thenReturn(endpoints);
    }

    @Test
    public void parseTest_AWSSign2_PathStyle() throws InvalidAccessKeyException,
            InvalidArgumentException{
        // Arrange
        requestBody = AWSV2RequestHelper.getRequestHeadersPathStyle();

        // Act
        ClientRequestToken token =  ClientRequestParser.parse(httpRequest, requestBody);

        // Verify
        assertNotNull(token);
        assertEquals("v_accessKeyId", token.getAccessKeyId());
    }

    @Test
    public void parseTest_AWSSign4_VirtualHostStyle() throws InvalidAccessKeyException,
            InvalidArgumentException{
        // Arrange
        requestBody = AWSV4RequestHelper.getRequestHeadersVirtualHostStyle();

        // Act
        ClientRequestToken token =  ClientRequestParser.parse(httpRequest, requestBody);

        // Verify
        assertNotNull(token);
        assertEquals("AKIAIOSFODNN7EXAMPLE", token.getAccessKeyId());
    }

    @Test(expected = NullPointerException.class)
    public void parseTest_RequestBodyNull_NullPointerException() throws InvalidAccessKeyException,
            InvalidArgumentException {

        ClientRequestParser.parse(httpRequest, null);
    }

    @Test(
        expected =
            InvalidArgumentException
                .class) public void parseTest_AuthenticateUser_AuthorizationHeaderNull()
        throws Exception {

        requestBody.put("Action", "AuthenticateUser");
        ServerResponse serverResponse = mock(ServerResponse.class);
        serverResponse.setResponseStatus(HttpResponseStatus.FORBIDDEN);
        serverResponse.setResponseBody("Access Denied.");
        ResponseGenerator mockResponseGenerator = mock(ResponseGenerator.class);
        PowerMockito.whenNew(ResponseGenerator.class)
            .withNoArguments()
            .thenReturn(mockResponseGenerator);
        Mockito.when(mockResponseGenerator.AccessDenied())
            .thenReturn(serverResponse);

        ClientRequestParser.parse(httpRequest, requestBody);
    }

    @Test(
        expected =
            InvalidArgumentException
                .class) public void parseTest_AuthorizeUser_AuthorizationHeaderNull()
        throws Exception {

        requestBody.put("Action", "AuthorizeUser");
        ServerResponse serverResponse = mock(ServerResponse.class);
        serverResponse.setResponseStatus(HttpResponseStatus.FORBIDDEN);
        serverResponse.setResponseBody("Access Denied.");
        ResponseGenerator mockResponseGenerator = mock(ResponseGenerator.class);
        PowerMockito.whenNew(ResponseGenerator.class)
            .withNoArguments()
            .thenReturn(mockResponseGenerator);
        Mockito.when(mockResponseGenerator.AccessDenied())
            .thenReturn(serverResponse);

        ClientRequestParser.parse(httpRequest, requestBody);
    }

    @Test(
        expected =
            InvalidArgumentException
                .class) public void parseTest_OtherAction_AuthorizationHeaderNull()
        throws Exception {
        requestBody.put("Action", "OtherAction");
        FullHttpRequest fullHttpRequest
                = new DefaultFullHttpRequest(HttpVersion.HTTP_1_1, HttpMethod.GET, "/");
        fullHttpRequest.headers().add("XYZ", "");
        when(httpRequest.headers()).thenReturn(fullHttpRequest.headers());
        ServerResponse serverResponse = mock(ServerResponse.class);
        serverResponse.setResponseStatus(HttpResponseStatus.FORBIDDEN);
        serverResponse.setResponseBody("Access Denied.");
        ResponseGenerator mockResponseGenerator = mock(ResponseGenerator.class);
        PowerMockito.whenNew(ResponseGenerator.class)
            .withNoArguments()
            .thenReturn(mockResponseGenerator);
        Mockito.when(mockResponseGenerator.AccessDenied())
            .thenReturn(serverResponse);

        ClientRequestParser.parse(httpRequest, requestBody);
    }

    @Test
    public void parseTest_InvalidToken_AuthenticateUser() throws InvalidAccessKeyException,
            InvalidArgumentException{
        requestBody.put("Action", "AuthenticateUser");
        requestBody.put("host", "iam.seagate.com:28051");
        requestBody.put("authorization","AWS4-HMAC-SHA256 Credential=vcevceece/"
                + "2018 1001/us-west2/iam/aws4_request, SignedHeaders=host;x-amz-date, "
                + "Signature=676b4c41ad34f611ada591072cd2977cf948d4556ffca32164af1cf1b8d4f181");

        assertNull(ClientRequestParser.parse(httpRequest, requestBody));
    }
}
