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

import io.netty.channel.ChannelInitializer;
import io.netty.channel.ChannelPipeline;
import io.netty.channel.socket.SocketChannel;
import io.netty.handler.codec.http.HttpObjectAggregator;
import io.netty.handler.codec.http.HttpRequestDecoder;
import io.netty.handler.codec.http.HttpServerCodec;
import io.netty.handler.ssl.SslContext;
import io.netty.handler.stream.ChunkedWriteHandler;
import io.netty.util.concurrent.EventExecutorGroup;

public class AuthServerHTTPSInitializer extends ChannelInitializer<SocketChannel> {

    private final EventExecutorGroup EXECUTOR_GROUP;

    public AuthServerHTTPSInitializer(EventExecutorGroup executorGroup) {
        EXECUTOR_GROUP = executorGroup;
    }

    @Override
    public void initChannel(SocketChannel ch) {
        SslContext sslContext = SSLContextProvider.getServerContext();

        ChannelPipeline p = ch.pipeline();

        if (sslContext != null) {
            p.addLast(sslContext.newHandler(ch.alloc()));
        }

        p.addLast(new HttpServerCodec());
        p.addLast("decoder", new HttpRequestDecoder());
        p.addLast("aggregator", new HttpObjectAggregator(1048576));
        p.addLast(new ChunkedWriteHandler());
        p.addLast(EXECUTOR_GROUP, new AuthServerHandler());
    }
}
