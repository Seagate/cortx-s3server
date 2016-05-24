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
 * Original creation date: 23-Nov-2015
 */
package com.seagates3.authserver;

import com.seagates3.controller.IAMController;
import com.seagates3.controller.SAMLWebSSOController;
import com.seagates3.response.ServerResponse;
import io.netty.buffer.Unpooled;
import io.netty.channel.ChannelFutureListener;
import io.netty.channel.ChannelHandlerContext;
import io.netty.handler.codec.http.DefaultFullHttpResponse;
import io.netty.handler.codec.http.FullHttpRequest;
import io.netty.handler.codec.http.FullHttpResponse;
import io.netty.handler.codec.http.HttpHeaders;
import static io.netty.handler.codec.http.HttpHeaders.Names.CONNECTION;
import static io.netty.handler.codec.http.HttpHeaders.Names.CONTENT_LENGTH;
import static io.netty.handler.codec.http.HttpHeaders.Names.CONTENT_TYPE;
import io.netty.handler.codec.http.HttpVersion;
import io.netty.handler.codec.http.multipart.DefaultHttpDataFactory;
import io.netty.handler.codec.http.multipart.HttpDataFactory;
import io.netty.handler.codec.http.multipart.HttpPostRequestDecoder;
import io.netty.handler.codec.http.multipart.InterfaceHttpData;
import java.io.UnsupportedEncodingException;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class AuthServerPostHandler {

    private final Logger LOGGER = LoggerFactory.getLogger(
            AuthServerHandler.class.getName());

    final String KEY_PAIR_REGEX = "([a-zA-Z0-9/-]+)=([\\w\\W]*)";
    final ChannelHandlerContext ctx;
    final FullHttpRequest httpRequest;
    final Boolean keepAlive;

    public AuthServerPostHandler(ChannelHandlerContext ctx,
            FullHttpRequest httpRequest) {
        this.httpRequest = httpRequest;
        this.ctx = ctx;
        keepAlive = HttpHeaders.isKeepAlive(httpRequest);
    }

    public void run() {
        Map<String, String> requestBody;
        ServerResponse serverResponse;
        HttpDataFactory factory = new DefaultHttpDataFactory(
                DefaultHttpDataFactory.MINSIZE);

        HttpPostRequestDecoder decoder
                = new HttpPostRequestDecoder(factory, httpRequest);

        requestBody = parseChunkedRequest(decoder.getBodyHttpDatas());

        if (httpRequest.getUri().startsWith("/saml")) {
            LOGGER.debug("Calling SAML WebSSOControler.");

            FullHttpResponse response = new SAMLWebSSOController(requestBody)
                    .samlSignIn();
            returnHTTPResponse(response);
        } else {
            LOGGER.debug("Requested Action - " + requestBody.get("Action"));

            IAMController iamController = new IAMController();
            serverResponse = iamController.serve(httpRequest, requestBody);
            returnHTTPResponse(serverResponse);
        }
    }

    /**
     * Read the requestResponse object and send the response to the client.
     */
    private void returnHTTPResponse(ServerResponse requestResponse) {
        String responseBody = requestResponse.getResponseBody();
        FullHttpResponse response;

        try {
            response = new DefaultFullHttpResponse(HttpVersion.HTTP_1_1,
                    requestResponse.getResponseStatus(),
                    Unpooled.wrappedBuffer(responseBody.getBytes("UTF-8"))
            );

            LOGGER.debug("Response sent successfully.");
        } catch (UnsupportedEncodingException ex) {
            response = null;
        }

        response.headers().set(CONTENT_TYPE, "text/xml");
        response.headers().set(CONTENT_LENGTH, response.content().readableBytes());

        if (!keepAlive) {
            ctx.write(response).addListener(ChannelFutureListener.CLOSE);

            LOGGER.debug("Connection closed.");
        } else {
            response.headers().set(CONNECTION, HttpHeaders.Values.KEEP_ALIVE);
            ctx.writeAndFlush(response);

            LOGGER.debug("Connection kept alive.");
        }
    }

    private void returnHTTPResponse(FullHttpResponse response) {
        if (!keepAlive) {
            ctx.write(response).addListener(ChannelFutureListener.CLOSE);
        } else {
            response.headers().set(CONNECTION, HttpHeaders.Values.KEEP_ALIVE);
            ctx.writeAndFlush(response);
        }
    }

    /**
     * S3 Clients sends the request body in chunks. These chunks are then and
     * aggregated using HttpObjectAggregator handler.
     *
     * The request body looks like this - Mixed: Action=CreateUser Mixed:
     * UserName=admin Mixed: Version=2010-05-08
     *
     * This method should parse this body and return a hash map of values like
     * [<Action, Createuser>, <UserName, arjun>, <Version, 2010-05-08>]
     *
     * @param datas
     * @return hash map
     */
    private Map<String, String> parseChunkedRequest(List<InterfaceHttpData> datas) {
        Map<String, String> requestBody;
        requestBody = new TreeMap<>(String.CASE_INSENSITIVE_ORDER);
        Pattern pattern = Pattern.compile(KEY_PAIR_REGEX);
        Matcher matcher;
        String[] tokens;

        for (InterfaceHttpData data : datas) {
            matcher = pattern.matcher(data.toString());
            if (matcher.find()) {
                tokens = matcher.group().split("=", 2);
                requestBody.put(tokens[0], tokens[1]);
            }
        }

        LOGGER.debug("Request Body - " + requestBody.toString());
        return requestBody;
    }
}
