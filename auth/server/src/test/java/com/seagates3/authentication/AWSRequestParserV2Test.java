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

import com.seagates3.authserver.AuthServerConfig;
import com.seagates3.exception.InvalidTokenException;

import io.netty.handler.codec.http.DefaultHttpHeaders;
import io.netty.handler.codec.http.FullHttpRequest;
import io.netty.handler.codec.http.HttpHeaders;
import io.netty.handler.codec.http.HttpMethod;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.powermock.api.mockito.PowerMockito;
import org.powermock.core.classloader.annotations.PowerMockIgnore;
import org.powermock.core.classloader.annotations.PrepareForTest;
import org.powermock.modules.junit4.PowerMockRunner;

import java.util.Map;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyString;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.spy;
import static org.mockito.Mockito.when;

@RunWith(PowerMockRunner.class)
@PrepareForTest(AuthServerConfig.class)
@PowerMockIgnore( {"javax.management.*"})

public class AWSRequestParserV2Test {

    private AWSRequestParserV2 awsRequestParserV2;

    @Before
    public void setUp() {
        awsRequestParserV2 = new AWSRequestParserV2();
    }

    @Test(expected = NullPointerException.class)
    public void parseTest_ShouldThrowExceptionIfNullParamPassed() throws InvalidTokenException {
        // Arrange
        FullHttpRequest fullHttpRequest = mock(FullHttpRequest.class);

        // Act
        awsRequestParserV2.parse(fullHttpRequest);
    }

    @Test
    public void parseTest_ForFullHttpRequest() throws Exception {
        // Arrange
        FullHttpRequest fullHttpRequest = mock(FullHttpRequest.class);
        HttpHeaders httpHeaders = new DefaultHttpHeaders();
        httpHeaders.add("host", "s3.seagate.com");
        httpHeaders.add("authorization", "AWS AKIAJTYX36YCKQSAJT7Q:uDWiVvxwCUR9YJ8EGJgbtW9tjFM=");

        when(fullHttpRequest.headers()).thenReturn(httpHeaders);
        when(fullHttpRequest.method()).thenReturn(HttpMethod.GET);
        when(fullHttpRequest.uri()).thenReturn("/");

        String[] endpoints = {"s3-us.seagate.com", "s3-europe.seagate.com",
                "s3-asia.seagate.com"};
        PowerMockito.mockStatic(AuthServerConfig.class);
        PowerMockito.doReturn(endpoints).when(AuthServerConfig.class, "getEndpoints");

        // Act
        ClientRequestToken clientRequestToken = awsRequestParserV2.parse(fullHttpRequest);

        // Verify
        assertEquals("GET", clientRequestToken.getHttpMethod().toString());
        assertEquals(ClientRequestToken.AWSSigningVersion.V2, clientRequestToken.getSignVersion());
    }

    @Test
    public void parseTest_ForRequestBody() throws InvalidTokenException {
        // Arrange
        Map<String, String> requestBody = mock(Map.class);

        AWSRequestParserV2 awsRequestParserV2Spy = spy(awsRequestParserV2);
        doNothing().when(awsRequestParserV2Spy).parseRequestHeaders(any(Map.class), any(ClientRequestToken.class));
        doNothing().when(awsRequestParserV2Spy).authHeaderParser(anyString(), any(ClientRequestToken.class));

        // Act && Verify
        assertTrue(awsRequestParserV2Spy.parse(requestBody) instanceof ClientRequestToken);
    }

    @Test
    public void authHeaderParserTest() throws InvalidTokenException {
        // Arrange
        String authorizationHeaderValue = "AWS AKIAJTYX36YCKQSAJT7Q:uDWiVvxwCUR9YJ8EGJgbtW9tjFM=";
        ClientRequestToken clientRequestToken = new ClientRequestToken();

        // Act
        awsRequestParserV2.authHeaderParser(authorizationHeaderValue, clientRequestToken);

        // Verify
        assertEquals("AKIAJTYX36YCKQSAJT7Q", clientRequestToken.accessKeyId);
        assertEquals("uDWiVvxwCUR9YJ8EGJgbtW9tjFM=", clientRequestToken.getSignature());
    }

    @Test(expected = InvalidTokenException.class)
    public void authHeaderParserTest_InvalidAuthorizationHeader() throws InvalidTokenException {
        // Arrange
        String authorizationHeaderValue = "AWSAKIAJTYX36YCKQSAJT7Q:uDWiVvxwCUR9YJ8EGJgbtW9tjFM=";
        ClientRequestToken clientRequestToken = new ClientRequestToken();

        // Act
        awsRequestParserV2.authHeaderParser(authorizationHeaderValue, clientRequestToken);
    }

    @Test(expected = NullPointerException.class)
    public void authHeaderParserTest_NULLAuthorizationHeader() throws InvalidTokenException {
        // Arrange
        String authorizationHeaderValue = null;
        ClientRequestToken clientRequestToken = new ClientRequestToken();

        // Act
        awsRequestParserV2.authHeaderParser(authorizationHeaderValue, clientRequestToken);
    }

    @Test(expected = InvalidTokenException.class)
    public void authHeaderParserTest_InvalidAuthorizationHeader_V4() throws InvalidTokenException {
        // Arrange
        String authorizationHeaderValue = "AWS4-HMAC-SHA256 Credential=" +
                "AKIAIOSFODNN7EXAMPLE/20160321/US/s3/aws4_request,SignedHeaders=host;" +
                "x-amz-content-sha256;x-amz-date,Signature=b9d04cd83010685a1085ece386" +
                "57125c02a6f29093f5bd21dcd6e717f1996852";
        ClientRequestToken clientRequestToken = new ClientRequestToken();

        // Act
        awsRequestParserV2.authHeaderParser(authorizationHeaderValue, clientRequestToken);
    }
}
