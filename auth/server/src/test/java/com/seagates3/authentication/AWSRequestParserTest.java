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

import java.util.Map;
import java.util.TreeMap;

import io.netty.handler.codec.http.DefaultHttpHeaders;
import io.netty.handler.codec.http.FullHttpRequest;
import io.netty.handler.codec.http.HttpMethod;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mockito;
import org.powermock.api.mockito.PowerMockito;
import org.powermock.core.classloader.annotations.PowerMockIgnore;
import org.powermock.core.classloader.annotations.PrepareForTest;
import org.powermock.modules.junit4.PowerMockRunner;

import com.seagates3.authserver.AuthServerConfig;

import static org.mockito.Matchers.anyMap;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

@PowerMockIgnore({"javax.management.*"}) @RunWith(PowerMockRunner.class)
    @PrepareForTest(AuthServerConfig.class) public class AWSRequestParserTest {

    AWSRequestParser awsRequestParser;
    ClientRequestToken clientRequestToken;
    Map<String, String> requestBody;

    @Before
    public void setup() throws Exception {
        awsRequestParser = Mockito.mock(AWSRequestParser.class,
                Mockito.CALLS_REAL_METHODS);
        clientRequestToken = new ClientRequestToken();
        requestBody = new TreeMap<>(String.CASE_INSENSITIVE_ORDER);

        String defaultEndPoints = "s3.seagate.com";
        String[] endpoints = {"s3-us.seagate.com", "s3-europe.seagate.com",
                "s3-asia.seagate.com"};

        PowerMockito.mockStatic(AuthServerConfig.class);
        PowerMockito.doReturn(endpoints).when(AuthServerConfig.class,
                "getEndpoints");
        PowerMockito.doReturn(defaultEndPoints).when(AuthServerConfig.class,
                "getDefaultEndpoint");
    }

    @Test
    public void parseRequestHeadersTest_VerifyBucketName() {
        requestBody.put("Host", "seagatebucket.s3.seagate.com");
        awsRequestParser.parseRequestHeaders(requestBody, clientRequestToken);
        Assert.assertTrue(clientRequestToken.isVirtualHost());
        Assert.assertEquals("seagatebucket",
                clientRequestToken.getBucketName());
    }

    @Test
    public void parseRequestHeadersTest_VerifyBucketName_WithPeriod() {
        requestBody.put("Host", "seagate.bucket.s3.seagate.com");
        awsRequestParser.parseRequestHeaders(requestBody, clientRequestToken);
        Assert.assertTrue(clientRequestToken.isVirtualHost());
        Assert.assertEquals("seagate.bucket",
                clientRequestToken.getBucketName());
    }

    @Test
    public void parseRequestHeadersTest_VerifyBucketName_WithHyphen() {
        requestBody.put("Host", "seagate-100200300.s3.seagate.com");
        awsRequestParser.parseRequestHeaders(requestBody, clientRequestToken);
        Assert.assertTrue(clientRequestToken.isVirtualHost());
        Assert.assertEquals("seagate-100200300",
                clientRequestToken.getBucketName());
    }

    @Test
    public void parseRequestHeadersTest_VerifyIsVirtualHost_False() {
        requestBody.put("Host", "s3.seagate.com");
        awsRequestParser.parseRequestHeaders(requestBody, clientRequestToken);
        Assert.assertFalse(clientRequestToken.isVirtualHost());
        Assert.assertEquals(null, clientRequestToken.getBucketName());
    }

    @Test
    public void parseRequestHeadersTest_HttpRequest() {
        // Arrange
        FullHttpRequest httpRequest = Mockito.mock(FullHttpRequest.class);
        ClientRequestToken clientRequestToken = Mockito.mock(ClientRequestToken.class);

        when(httpRequest.headers()).thenReturn(new DefaultHttpHeaders().add("host", "s3.seagate.com"));
        when(httpRequest.method()).thenReturn(HttpMethod.GET);
        when(httpRequest.uri()).thenReturn("/");

        // Act
        awsRequestParser.parseRequestHeaders(httpRequest, clientRequestToken);

        // Verify
        verify(clientRequestToken).setHttpMethod("GET");
        verify(clientRequestToken).setUri("/");
        verify(clientRequestToken).setQuery("");
        verify(clientRequestToken).setRequestHeaders(anyMap());
    }
}
