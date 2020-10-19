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

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.powermock.api.mockito.PowerMockito.whenNew;

import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mockito;
import org.powermock.api.mockito.PowerMockito;
import org.powermock.api.mockito.mockpolicies.Slf4jMockPolicy;
import org.powermock.core.classloader.annotations.MockPolicy;
import org.powermock.core.classloader.annotations.PowerMockIgnore;
import org.powermock.core.classloader.annotations.PrepareForTest;
import org.powermock.modules.junit4.PowerMockRunner;

import io.netty.channel.ChannelHandlerContext;
import io.netty.handler.codec.http.FullHttpRequest;
import io.netty.handler.codec.http.HttpMethod;

@RunWith(PowerMockRunner.class)
@PrepareForTest({AuthServerHandler.class, AuthServerConfig.class})
@PowerMockIgnore({"javax.management.*"})
@MockPolicy(Slf4jMockPolicy.class)
public class AuthServerHandlerTest {

    private AuthServerHandler testHandler;
    private ChannelHandlerContext ctx;
    private FullHttpRequest httpRequest;

    @BeforeClass
    public static void setUpStatic() throws Exception {
        PowerMockito.mockStatic(AuthServerConfig.class);
        PowerMockito.doReturn(false).when(AuthServerConfig.class,
                "isPerfEnabled");
        PowerMockito.doReturn("/tmp/perf.log").when(AuthServerConfig.class,
                "getPerfLogFile");
    }

    @Before
    public void setUp() {
        ctx = Mockito.mock(ChannelHandlerContext.class);
        httpRequest = mock(FullHttpRequest.class);
        testHandler = new AuthServerHandler();
    }

    @Test
    public void channelReadTest() throws Exception {
        AuthServerPostHandler postHandler = mock(AuthServerPostHandler.class);
        when(httpRequest.method()).thenReturn(HttpMethod.POST);
        whenNew(AuthServerPostHandler.class).withArguments(
                ctx, httpRequest).thenReturn(postHandler);

        testHandler.channelRead(ctx, httpRequest);
        Mockito.verify(postHandler).run();
    }

    @Test
    public void channelReadTest_GetRequest() throws Exception {
        AuthServerGetHandler getHandler = mock(AuthServerGetHandler.class);
        when(httpRequest.method()).thenReturn(HttpMethod.GET);
        whenNew(AuthServerGetHandler.class)
                .withArguments(ctx, httpRequest).thenReturn(getHandler);

        testHandler.channelRead(ctx, httpRequest);

        Mockito.verify(getHandler).run();
    }

    @Test
    public void channelReadCompleteTest() {
        testHandler.channelReadComplete(ctx);

        verify(ctx).flush();
    }

    @Test
    public void exceptionCaughtTest() {
        Throwable cause = mock(Throwable.class);

        testHandler.exceptionCaught(ctx, cause);

        verify(ctx).close();
    }

}
