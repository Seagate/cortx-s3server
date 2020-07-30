package com.seagates3.authserver;

import com.seagates3.perf.S3Perf;
import io.netty.channel.ChannelFuture;
import io.netty.channel.ChannelHandlerContext;
import io.netty.channel.ChannelInboundHandlerAdapter;
import io.netty.handler.codec.http.FullHttpRequest;
import io.netty.handler.codec.http.HttpMethod;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Extend the Channel Inbound Handler Adapter to create a custom handler for
 * authentication and authorization server.
 */
public class AuthServerHandler extends ChannelInboundHandlerAdapter {

    private final Logger LOGGER = LoggerFactory.getLogger(
            AuthServerHandler.class.getName());

    /**
     *
     * @param ctx
     */
    @Override
    public void channelReadComplete(ChannelHandlerContext ctx) {
        ctx.flush();
    }

    /**
     * Entry point to the server handler. Perform basic validation like check if
     * the request is an "FullHttpRequest" object etc and then pass the control
     * to "AuthServerPostHandler" to handle 'Post' requests and
     * "AuthServerGetHandler" to handle 'Get' requests.
     *
     * @param ctx Channel Hander Context object.
     * @param msg Instance of FullHttpRequest
     */
    @Override
    public void channelRead(ChannelHandlerContext ctx, Object msg) {
        S3Perf perf = new S3Perf();
        perf.startClock();
        if (msg instanceof FullHttpRequest) {
            FullHttpRequest httpRequest = (FullHttpRequest) msg;

            LOGGER.debug("Channel read succesful.");
            LOGGER.debug("Http method - " + httpRequest.getMethod());
            LOGGER.debug("URI - " + httpRequest.getUri());

            try {
                if (httpRequest.getMethod().equals(HttpMethod.POST)) {
                    new AuthServerPostHandler(ctx, httpRequest).run();
                } else if (httpRequest.getMethod().equals(HttpMethod.GET)) {
                    new AuthServerGetHandler(ctx, httpRequest).run();
                } else if (httpRequest.getMethod().equals(HttpMethod.HEAD)) {
                  new AuthServerHeadHandler(ctx, httpRequest).run();
                }
            } finally {
                httpRequest.release();
            }

        }
        perf.endClock();
        perf.printTime("AuthServerHandler");
    }

    @Override
    public void exceptionCaught(ChannelHandlerContext ctx, Throwable cause) {
        ChannelFuture close = ctx.close();
    }

    private AuthServerPostHandler createPostHandler(ChannelHandlerContext ctx,
            FullHttpRequest httpRequest) {
        return new AuthServerPostHandler(ctx, httpRequest);
    }
}
