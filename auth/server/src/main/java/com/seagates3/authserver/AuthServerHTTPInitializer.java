package com.seagates3.authserver;

import io.netty.channel.ChannelInitializer;
import io.netty.channel.ChannelPipeline;
import io.netty.channel.socket.SocketChannel;
import io.netty.handler.codec.http.HttpObjectAggregator;
import io.netty.handler.codec.http.HttpRequestDecoder;
import io.netty.handler.codec.http.HttpServerCodec;
import io.netty.handler.stream.ChunkedWriteHandler;
import io.netty.util.concurrent.EventExecutorGroup;

public class AuthServerHTTPInitializer extends ChannelInitializer<SocketChannel> {

    private final EventExecutorGroup EXECUTOR_GROUP;

    public AuthServerHTTPInitializer(EventExecutorGroup executorGroup) {
        EXECUTOR_GROUP = executorGroup;
    }

    @Override
    public void initChannel(SocketChannel ch) {
        ChannelPipeline p = ch.pipeline();

        p.addLast(new HttpServerCodec());
        p.addLast("decoder", new HttpRequestDecoder());
        p.addLast("aggregator", new HttpObjectAggregator(1048576));
        p.addLast(new ChunkedWriteHandler());
        p.addLast(EXECUTOR_GROUP, new AuthServerHandler());
    }
}
