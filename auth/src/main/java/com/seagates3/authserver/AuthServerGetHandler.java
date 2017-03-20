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

import com.seagates3.controller.SAMLWebSSOController;
import com.seagates3.response.ServerResponse;
import com.seagates3.util.IEMUtil;
import io.netty.buffer.Unpooled;
import io.netty.channel.ChannelFuture;
import io.netty.channel.ChannelFutureListener;
import io.netty.channel.ChannelHandlerContext;
import io.netty.channel.DefaultFileRegion;
import io.netty.handler.codec.http.DefaultFullHttpResponse;
import io.netty.handler.codec.http.DefaultHttpResponse;
import io.netty.handler.codec.http.FullHttpRequest;
import io.netty.handler.codec.http.FullHttpResponse;
import io.netty.handler.codec.http.HttpChunkedInput;
import io.netty.handler.codec.http.HttpHeaders;
import static io.netty.handler.codec.http.HttpHeaders.Names.CONNECTION;
import static io.netty.handler.codec.http.HttpHeaders.Names.CONTENT_LENGTH;
import static io.netty.handler.codec.http.HttpHeaders.Names.CONTENT_TYPE;
import io.netty.handler.codec.http.HttpResponse;
import io.netty.handler.codec.http.HttpResponseStatus;
import static io.netty.handler.codec.http.HttpResponseStatus.OK;
import io.netty.handler.codec.http.HttpVersion;
import static io.netty.handler.codec.http.HttpVersion.HTTP_1_1;
import io.netty.handler.codec.http.LastHttpContent;
import io.netty.handler.ssl.SslHandler;
import io.netty.handler.stream.ChunkedFile;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.io.UnsupportedEncodingException;
import java.nio.file.Path;
import java.nio.file.Paths;
import javax.activation.MimetypesFileTypeMap;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class AuthServerGetHandler {

    private final Logger LOGGER = LoggerFactory.getLogger(
            AuthServerGetHandler.class.getName());

    final ChannelHandlerContext ctx;
    final FullHttpRequest httpRequest;
    final Boolean keepAlive;

    public AuthServerGetHandler(ChannelHandlerContext ctx,
            FullHttpRequest httpRequest) {
        this.ctx = ctx;
        this.httpRequest = httpRequest;
        keepAlive = HttpHeaders.isKeepAlive(httpRequest);
    }

    public void run() {
        LOGGER.debug("Get handler called.");

        if (httpRequest.getUri().startsWith("/static")) {
            Path staticFilePath = Paths.get("", "resources", httpRequest.getUri());
            File file = staticFilePath.toFile();

            LOGGER.debug("Static file path - " + staticFilePath);

            RandomAccessFile raf;
            long fileLength;
            try {
                raf = new RandomAccessFile(file, "r");
                fileLength = raf.length();

                LOGGER.debug("Static file length - " + fileLength);
            } catch (FileNotFoundException ex) {
                LOGGER.debug("File not found.");
                sendErrorResponse(HttpResponseStatus.NOT_FOUND, "Resource not found.");
                return;
            } catch (IOException ex) {
                LOGGER.error("Error occurred while reading file.\n"
                        + ex.getMessage());
                sendErrorResponse(HttpResponseStatus.INTERNAL_SERVER_ERROR,
                        "Error occurred while reading the file.");
                return;
            }

            writeHeader(file, fileLength);
            writeContent(raf, fileLength);
        } else if (httpRequest.getUri().startsWith("/saml/session")) {
            LOGGER.debug("Calling SAML WEB SSO Controller");

            ServerResponse severReponse = new SAMLWebSSOController(null)
                    .createSession(httpRequest);
            returnHTTPResponse(severReponse);
        } else {
            LOGGER.debug("Bad request.");
            HttpResponse response = new DefaultHttpResponse(HTTP_1_1,
                    HttpResponseStatus.BAD_REQUEST);
            if (keepAlive) {
                response.headers().set(CONNECTION, HttpHeaders.Values.KEEP_ALIVE);
            }
            ctx.write(response).addListener(ChannelFutureListener.CLOSE);
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

            LOGGER.debug("HTTP Response sent.");
        } catch (UnsupportedEncodingException ex) {
            IEMUtil.log(IEMUtil.Level.ERROR, IEMUtil.UTF8_UNAVAILABLE,
                    "UTF-8 encoding is not supported", null);
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

    /**
     * Send the first line and the header before transferring the rest of the
     * file.
     *
     * @param file
     * @param fileLength
     *
     */
    private void writeHeader(File file, long fileLength) {
        HttpResponse response = new DefaultHttpResponse(HTTP_1_1, OK);
        HttpHeaders.setContentLength(response, fileLength);
        setContentTypeHeader(response, file);
        if (keepAlive) {
            response.headers().set(CONNECTION, HttpHeaders.Values.KEEP_ALIVE);
        }

        ctx.write(response);
    }

    private void writeContent(RandomAccessFile raf, long fileLength) {
        ChannelFuture sendFileFuture = null;
        ChannelFuture lastContentFuture;

        if (ctx.pipeline().get(SslHandler.class) == null) {
            ctx.write(new DefaultFileRegion(raf.getChannel(), 0, fileLength),
                    ctx.newProgressivePromise());
            lastContentFuture = ctx.writeAndFlush(LastHttpContent.EMPTY_LAST_CONTENT);
        } else {
            try {
                sendFileFuture
                        = ctx.writeAndFlush(new HttpChunkedInput(
                                        new ChunkedFile(raf, 0, fileLength, 4)),
                                ctx.newProgressivePromise());
                lastContentFuture = sendFileFuture;

                LOGGER.debug("Static resource sent to client.");
            } catch (IOException ex) {
                lastContentFuture = null;
            }
        }

        /**
         * Add listener to send file future if required.
         */
        // Decide whether to close the connection or not.
        if (!keepAlive) {
            // Close the connection when the whole content is written out.
            lastContentFuture.addListener(ChannelFutureListener.CLOSE);
            LOGGER.debug("Connection closed.");
        }
    }

    private void sendErrorResponse(HttpResponseStatus status,
            String responseBody) {
        FullHttpResponse response;

        try {
            response = new DefaultFullHttpResponse(HttpVersion.HTTP_1_1,
                    status, Unpooled.wrappedBuffer(responseBody.getBytes("UTF-8"))
            );

            LOGGER.debug("Error response sent.");
        } catch (UnsupportedEncodingException ex) {
            IEMUtil.log(IEMUtil.Level.ERROR, IEMUtil.UTF8_UNAVAILABLE,
                    "UTF-8 encoding is not supported", null);
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

    private void setContentTypeHeader(HttpResponse response, File file) {
        MimetypesFileTypeMap mimeTypesMap = new MimetypesFileTypeMap();
        response.headers().set(CONTENT_TYPE, mimeTypesMap.getContentType(file.getPath()));
    }

}
