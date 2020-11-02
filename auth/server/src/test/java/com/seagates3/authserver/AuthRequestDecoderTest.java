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

package com.seagates3.authserver;

import io.netty.buffer.ByteBuf;
import io.netty.buffer.ByteBufUtil;
import io.netty.buffer.Unpooled;
import io.netty.handler.codec.http.DefaultFullHttpRequest;
import io.netty.handler.codec.http.FullHttpRequest;
import io.netty.handler.codec.http.HttpMethod;
import io.netty.handler.codec.http.HttpVersion;
import io.netty.handler.codec.http.multipart.Attribute;
import io.netty.handler.codec.http.multipart.InterfaceHttpData;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.powermock.api.mockito.mockpolicies.Slf4jMockPolicy;
import org.powermock.core.classloader.annotations.MockPolicy;
import org.powermock.core.classloader.annotations.PowerMockIgnore;
import org.powermock.core.classloader.annotations.PrepareForTest;
import org.powermock.modules.junit4.PowerMockRunner;
import org.powermock.reflect.internal.WhiteboxImpl;

import java.io.IOException;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

import static org.junit.Assert.*;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

@PowerMockIgnore({"javax.management.*"}) @RunWith(PowerMockRunner.class)
    @PrepareForTest({AuthRequestDecoder.class})
    @MockPolicy(Slf4jMockPolicy.class) public class AuthRequestDecoderTest {

    private FullHttpRequest fullHttpRequest;
    private AuthRequestDecoder authRequestDecoder;

    @Before
    public void setUp() throws Exception {
        fullHttpRequest = new DefaultFullHttpRequest(
                HttpVersion.HTTP_1_1, HttpMethod.POST, "/", getRequestBodyAsByteBuf());

        authRequestDecoder = new AuthRequestDecoder(fullHttpRequest);
    }

    @Test
    public void getRequestBodyAsMapTest() {
        Map<String, String> result = authRequestDecoder.getRequestBodyAsMap();

        assertEquals(getExpectedBody(), result);
    }

    @Test
    public void parseRequestBodyTest() throws Exception {
        List<InterfaceHttpData> datas = mock(List.class);
        Attribute data = mock(Attribute.class);
        Iterator iterator = mock(Iterator.class);

        when(iterator.hasNext()).thenReturn(Boolean.TRUE).thenReturn(Boolean.FALSE);
        when(iterator.next()).thenReturn(data);
        when(datas.iterator()).thenReturn(iterator);
        when(data.getHttpDataType()).thenReturn(InterfaceHttpData.HttpDataType.Attribute);
        when(data.getValue()).thenThrow(IOException.class);

        Map<String, String> result = WhiteboxImpl.invokeMethod(authRequestDecoder,
                "parseRequestBody", datas);

        assertNotNull(result);
        assertTrue(result.isEmpty());
    }

    private ByteBuf getRequestBodyAsByteBuf() {
        String params = "Action=AuthenticateUser&" +
                "x-amz-meta-ics.simpleencryption.status=enabled&" +
                "x-amz-meta-ics.meta-version=1&" +
                "x-amz-meta-ics.meta-version=2&" +
                "samlAssertion=assertion";
        ByteBuf byteBuf = Unpooled.buffer(params.length());
        ByteBufUtil.writeUtf8(byteBuf, params);

        return byteBuf;
    }

    private Map<String, String> getExpectedBody() {
        Map<String, String> expectedRequestBody = new TreeMap<>(String.CASE_INSENSITIVE_ORDER);
        expectedRequestBody.put("Action", "AuthenticateUser");
        expectedRequestBody.put("samlAssertion", "assertion");
        expectedRequestBody.put("x-amz-meta-ics.meta-version", "1,2");
        expectedRequestBody.put("x-amz-meta-ics.simpleencryption.status", "enabled");

        return expectedRequestBody;
    }
}