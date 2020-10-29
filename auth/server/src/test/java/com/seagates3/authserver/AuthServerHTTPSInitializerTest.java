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

import io.netty.buffer.ByteBufAllocator;
import io.netty.channel.ChannelPipeline;
import io.netty.channel.socket.SocketChannel;
import io.netty.handler.ssl.SslContext;
import io.netty.handler.ssl.SslHandler;
import io.netty.util.concurrent.EventExecutorGroup;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.powermock.api.mockito.PowerMockito;
import org.powermock.core.classloader.annotations.PowerMockIgnore;
import org.powermock.core.classloader.annotations.PrepareForTest;
import org.powermock.modules.junit4.PowerMockRunner;

import static org.mockito.Matchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.powermock.api.mockito.PowerMockito.mockStatic;
import static org.powermock.api.mockito.PowerMockito.verifyStatic;

@RunWith(PowerMockRunner.class)
@PrepareForTest({SSLContextProvider.class, SslContext.class,
        AuthServerHTTPSInitializer.class})
@PowerMockIgnore({"javax.management.*"})
public class AuthServerHTTPSInitializerTest {

    private EventExecutorGroup executorGroup;
    private SocketChannel socketChannel;
    private ChannelPipeline channelPipeline;
    private AuthServerHTTPSInitializer httpsInitializer;

    @Before
    public void setUp() throws Exception {
        executorGroup = mock(EventExecutorGroup.class);
        socketChannel = mock(SocketChannel.class);
        channelPipeline = mock(ChannelPipeline.class);
        when(socketChannel.pipeline()).thenReturn(channelPipeline);
    }

    @Test
    public void initChannelTest() {
        httpsInitializer = new AuthServerHTTPSInitializer(executorGroup);
        httpsInitializer.initChannel(socketChannel);

        verify(socketChannel).pipeline();
        verify(channelPipeline).addLast(any(EventExecutorGroup.class),
                any(AuthServerHandler.class));
    }

    @Test
    public void initChannelTest_SslContextNotNull() {
        mockStatic(SslContext.class);
        mockStatic(SSLContextProvider.class);
        SslContext sslContext = PowerMockito.mock(SslContext.class);
        SslHandler sslHandler = mock(SslHandler.class);
        when(SSLContextProvider.getServerContext()).thenReturn(sslContext);
        when(sslContext.newHandler(any(ByteBufAllocator.class))).thenReturn(sslHandler);

        httpsInitializer = new AuthServerHTTPSInitializer(executorGroup);
        httpsInitializer.initChannel(socketChannel);

        verifyStatic();
        SSLContextProvider.getServerContext();
        verify(socketChannel).pipeline();
        verify(channelPipeline).addLast(sslHandler);
        verify(channelPipeline).addLast(any(EventExecutorGroup.class),
                any(AuthServerHandler.class));
    }
}