/*
 * COPYRIGHT 2015 SEAGATE LLC
 *
 * THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
 * HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE TECHNOLOGY
 * LIMITED, ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
 * THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
 * BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
 * USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
 * EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.
 *
 * YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
 * THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
 * http://www.seagate.com/contact
 *
 * Original author:  Arjun Hariharan <arjun.hariharan@seagate.com>
 * Original creation date: 17-Sep-2014
 */

package com.seagates3.authserver;

import java.io.UnsupportedEncodingException;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import io.netty.buffer.Unpooled;
import io.netty.channel.ChannelFuture;
import io.netty.channel.ChannelFutureListener;
import io.netty.channel.ChannelHandlerContext;
import io.netty.channel.ChannelInboundHandlerAdapter;
import io.netty.handler.codec.http.DefaultFullHttpResponse;
import io.netty.handler.codec.http.FullHttpResponse;
import io.netty.handler.codec.http.HttpHeaders;
import io.netty.handler.codec.http.HttpRequest;
import io.netty.handler.codec.http.HttpMethod;
import io.netty.handler.codec.http.HttpVersion;
import io.netty.handler.codec.http.multipart.DefaultHttpDataFactory;
import io.netty.handler.codec.http.multipart.HttpDataFactory;
import io.netty.handler.codec.http.multipart.HttpPostRequestDecoder;
import io.netty.handler.codec.http.multipart.InterfaceHttpData;

import static io.netty.handler.codec.http.HttpHeaders.Names.CONNECTION;
import static io.netty.handler.codec.http.HttpHeaders.Names.CONTENT_LENGTH;
import static io.netty.handler.codec.http.HttpHeaders.Names.CONTENT_TYPE;

import com.seagates3.response.ServerResponse;

/**
 * @author Arjun Hariharan
 */

public class AuthServerHandler extends ChannelInboundHandlerAdapter {
    static final String KEY_PAIR_REGEX = "([a-zA-Z]+)=([a-zA-Z0-9/-]+)";

    @Override
    public void channelReadComplete(ChannelHandlerContext ctx) {
        ctx.flush();
    }

    @Override
    public void channelRead(ChannelHandlerContext ctx, Object msg) {
        if (msg instanceof HttpRequest) {
            HttpRequest httpRequest = (HttpRequest) msg;
            Map<String, String> requestBody;
            ServerResponse serverResponse;

            if(httpRequest.getMethod().equals(HttpMethod.POST)) {
                HttpDataFactory factory = new
                    DefaultHttpDataFactory(DefaultHttpDataFactory.MINSIZE);

                HttpPostRequestDecoder decoder =
                        new HttpPostRequestDecoder(factory, httpRequest);

                requestBody = parseChunkedRequest(decoder.getBodyHttpDatas());

                AuthServerAction authserverAction = new AuthServerAction();
                serverResponse = authserverAction.serve(httpRequest, requestBody);
                sendResponse(ctx, httpRequest, serverResponse);
            }
        }
    }

    @Override
    public void exceptionCaught(ChannelHandlerContext ctx, Throwable cause) {
        ChannelFuture close = ctx.close();
    }

    /*
     * Read the requestResponse object and send the response to the client.
     */
    private void sendResponse(ChannelHandlerContext ctx, HttpRequest request,
            ServerResponse requestResponse) {

        String responseBody = requestResponse.getResponseBody();
        Boolean keepAlive = HttpHeaders.isKeepAlive(request);
        FullHttpResponse response;

        try {
            response = new DefaultFullHttpResponse( HttpVersion.HTTP_1_1,
                            requestResponse.getResponseStatus(),
                            Unpooled.wrappedBuffer(responseBody.getBytes("UTF-8"))
                    );
        } catch (UnsupportedEncodingException ex) {
            Logger.getLogger("authLog").log(Level.SEVERE, null, ex);
            return;
        }

        response.headers().set(CONTENT_TYPE, "text/xml");
        response.headers().set(CONTENT_LENGTH, response.content().readableBytes());

        if (!keepAlive) {
            ctx.write(response).addListener(ChannelFutureListener.CLOSE);
        } else {
            response.headers().set(CONNECTION, HttpHeaders.Values.KEEP_ALIVE);
            ctx.writeAndFlush(response);
        }
    }

    /*
     * S3 Clients sends the request body in chunks. These chunks are then
     * and aggregated using HttpObjectAggregator handler.
     *
     * The request body looks like this -
     * Mixed: Action=CreateUser
     * Mixed: UserName=arjun
     * Mixed: Version=2010-05-08
     *
     * This method should parse this body and return a hashmap of values like
     * [<Action, Createuser>, <UserName, arjun>, <Version, 2010-05-08>]
     *
     */
    private Map<String, String> parseChunkedRequest(List<InterfaceHttpData> datas) {
        Map<String, String> requestBody;
        requestBody = new TreeMap<>(String.CASE_INSENSITIVE_ORDER);
        Pattern pattern = Pattern.compile("([a-zA-Z0-9/-]+)=(.*)");
        Matcher matcher;
        String[] tokens;

        for(InterfaceHttpData data: datas) {
            matcher = pattern.matcher(data.toString());
            if (matcher.find())
            {
                tokens = matcher.group().split("=", 2);
                requestBody.put(tokens[0], tokens[1]);
            }
        }

        return requestBody;
    }
}
