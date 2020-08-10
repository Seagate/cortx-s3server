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

import io.netty.channel.ChannelPipeline;
import io.netty.channel.socket.SocketChannel;
import io.netty.util.concurrent.EventExecutorGroup;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.powermock.core.classloader.annotations.PowerMockIgnore;
import org.powermock.modules.junit4.PowerMockRunner;

import static org.mockito.Matchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

@RunWith(PowerMockRunner.class)
@PowerMockIgnore({"javax.management.*"})
public class AuthServerHTTPInitializerTest {

    private AuthServerHTTPInitializer httpInitializer;

    @Test
    public void initChannelTest() {
        EventExecutorGroup executorGroup = mock(EventExecutorGroup.class);
        SocketChannel socketChannel = mock(SocketChannel.class);
        ChannelPipeline channelPipeline = mock(ChannelPipeline.class);
        when(socketChannel.pipeline()).thenReturn(channelPipeline);

        httpInitializer = new AuthServerHTTPInitializer(executorGroup);
        httpInitializer.initChannel(socketChannel);

        verify(socketChannel).pipeline();
        verify(channelPipeline).addLast(any(EventExecutorGroup.class),
                any(AuthServerHandler.class));
    }
}