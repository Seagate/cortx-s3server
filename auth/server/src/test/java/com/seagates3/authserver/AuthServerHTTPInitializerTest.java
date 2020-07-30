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