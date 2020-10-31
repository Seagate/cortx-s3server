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

import static io.netty.handler.codec.http.HttpResponseStatus.OK;
import static io.netty.handler.codec.http.HttpVersion.HTTP_1_1;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.io.UnsupportedEncodingException;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Map;

import javax.activation.MimetypesFileTypeMap;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.seagates3.controller.SAMLWebSSOController;
import com.seagates3.response.ServerResponse;
import com.seagates3.util.BinaryUtil;
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
import io.netty.handler.codec.http.HttpHeaderNames;
import io.netty.handler.codec.http.HttpHeaderValues;
import io.netty.handler.codec.http.HttpResponse;
import io.netty.handler.codec.http.HttpResponseStatus;
import io.netty.handler.codec.http.HttpUtil;
import io.netty.handler.codec.http.HttpVersion;
import io.netty.handler.codec.http.LastHttpContent;
import io.netty.handler.ssl.SslHandler;
import io.netty.handler.stream.ChunkedFile;

public
class AuthServerGetHandler {

 private
  final Logger LOGGER =
      LoggerFactory.getLogger(AuthServerGetHandler.class.getName());

  final ChannelHandlerContext ctx;
  final FullHttpRequest httpRequest;
  final Boolean keepAlive;

 public
  AuthServerGetHandler(ChannelHandlerContext ctx, FullHttpRequest httpRequest) {
    this.ctx = ctx;
    this.httpRequest = httpRequest;
    keepAlive = HttpUtil.isKeepAlive(httpRequest);
  }

 public
  void run() {
    LOGGER.debug("Get handler called.");
    // Set Request ID
    Map<String, String> requestBody = getHttpRequestBodyAsMap();
    if (!(requestBody.get("Request_id") == null ||
          (requestBody.get("Request_id")).isEmpty())) {
      AuthServerConfig.setReqId(requestBody.get("Request_id"));
    } else {
      AuthServerConfig.setReqId(BinaryUtil.getAlphaNumericUUID());
    }

    if (httpRequest.uri().startsWith("/static")) {
      Path staticFilePath =
          Paths.get(AuthServerConstants.RESOURCE_DIR, httpRequest.uri());
      File file = staticFilePath.toFile();

      LOGGER.debug("Static file path - " + staticFilePath);

      long fileLength;
      try(RandomAccessFile raf = new RandomAccessFile(file, "r")) {

        fileLength = raf.length();
        LOGGER.debug("Static file length - " + fileLength);
        writeHeader(file, fileLength);
        writeContent(raf, fileLength);
      }
      catch (FileNotFoundException ex) {
        LOGGER.debug("File not found.");
        sendErrorResponse(HttpResponseStatus.NOT_FOUND, "Resource not found.");
        return;
      }
      catch (IOException ex) {
        LOGGER.error("Error occurred while reading file.\n" + ex.getMessage());
        sendErrorResponse(HttpResponseStatus.INTERNAL_SERVER_ERROR,
                          "Error occurred while reading the file.");
        return;
      }

    } else if (httpRequest.uri().startsWith("/saml/session")) {
      LOGGER.debug("Calling SAML WEB SSO Controller");

      ServerResponse severReponse =
          new SAMLWebSSOController(null).createSession(httpRequest);
      returnHTTPResponse(severReponse);
    } else {
      LOGGER.debug("Bad request.");
      HttpResponse response =
          new DefaultHttpResponse(HTTP_1_1, HttpResponseStatus.BAD_REQUEST);
      if (keepAlive) {
        response.headers().set(HttpHeaderNames.CONNECTION,
                               HttpHeaderValues.KEEP_ALIVE);
      }
      ctx.write(response).addListener(ChannelFutureListener.CLOSE);
    }
  }

  /**
   * Read the requestResponse object and send the response to the client.
   */
 private
  void returnHTTPResponse(ServerResponse requestResponse) {
    String responseBody = requestResponse.getResponseBody();
    FullHttpResponse response;

    try {
      response = new DefaultFullHttpResponse(
          HttpVersion.HTTP_1_1, requestResponse.getResponseStatus(),
          Unpooled.wrappedBuffer(responseBody.getBytes("UTF-8")));

      LOGGER.info("Sending HTTP Response code [" + response.status() + "]");
    }
    catch (UnsupportedEncodingException ex) {
      IEMUtil.log(IEMUtil.Level.ERROR, IEMUtil.UTF8_UNAVAILABLE,
                  "UTF-8 encoding is not supported", null);
      response = null;
    }
    if (response != null) {
      response.headers().set(HttpHeaderNames.CONTENT_TYPE, "text/xml");
      response.headers().set(HttpHeaderNames.CONTENT_LENGTH,
                             response.content().readableBytes());

      if (!keepAlive) {
        ctx.write(response).addListener(ChannelFutureListener.CLOSE);

        LOGGER.debug("Connection closed.");
      } else {
        response.headers().set(HttpHeaderNames.CONNECTION,
                               HttpHeaderValues.KEEP_ALIVE);
        ctx.writeAndFlush(response);

        LOGGER.debug("Connection kept alive.");
      }
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
 private
  void writeHeader(File file, long fileLength) {
    HttpResponse response = new DefaultHttpResponse(HTTP_1_1, OK);
    HttpUtil.setContentLength(response, fileLength);
    setContentTypeHeader(response, file);
    if (keepAlive) {
      response.headers().set(HttpHeaderNames.CONNECTION,
                             HttpHeaderValues.KEEP_ALIVE);
    }

    ctx.write(response);
  }

 private
  void writeContent(RandomAccessFile raf, long fileLength) {
    ChannelFuture sendFileFuture = null;
    ChannelFuture lastContentFuture;

    if (ctx.pipeline().get(SslHandler.class) == null) {
      ctx.write(new DefaultFileRegion(raf.getChannel(), 0, fileLength),
                ctx.newProgressivePromise());
      lastContentFuture = ctx.writeAndFlush(LastHttpContent.EMPTY_LAST_CONTENT);
    } else {
      try {
        sendFileFuture = ctx.writeAndFlush(
            new HttpChunkedInput(new ChunkedFile(raf, 0, fileLength, 4)),
            ctx.newProgressivePromise());
        lastContentFuture = sendFileFuture;

        LOGGER.debug("Static resource sent to client.");
      }
      catch (IOException ex) {
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

 private
  void sendErrorResponse(HttpResponseStatus status, String responseBody) {
    FullHttpResponse httpResponse;

    try {
      httpResponse = new DefaultFullHttpResponse(
          HttpVersion.HTTP_1_1, status,
          Unpooled.wrappedBuffer(responseBody.getBytes("UTF-8")));

      LOGGER.debug("Error response sent.");
    }
    catch (UnsupportedEncodingException ex) {
      IEMUtil.log(IEMUtil.Level.ERROR, IEMUtil.UTF8_UNAVAILABLE,
                  "UTF-8 encoding is not supported", null);
      httpResponse = null;
    }
    if (httpResponse != null) {
      httpResponse.headers().set(HttpHeaderNames.CONTENT_TYPE, "text/xml");
      httpResponse.headers().set(HttpHeaderNames.CONTENT_LENGTH,
                                 httpResponse.content().readableBytes());

      if (!keepAlive) {
        ctx.write(httpResponse).addListener(ChannelFutureListener.CLOSE);

        LOGGER.debug("Connection closed.");
      } else {
        httpResponse.headers().set(HttpHeaderNames.CONNECTION,
                                   HttpHeaderValues.KEEP_ALIVE);
        ctx.writeAndFlush(httpResponse);

        LOGGER.debug("Connection kept alive.");
      }
    }
  }
 private
  void setContentTypeHeader(HttpResponse response, File file) {
    MimetypesFileTypeMap mimeTypesMap = new MimetypesFileTypeMap();
    response.headers().set(HttpHeaderNames.CONTENT_TYPE,
                           mimeTypesMap.getContentType(file.getPath()));
  }
 private
  Map<String, String> getHttpRequestBodyAsMap() {
    AuthRequestDecoder decoder = new AuthRequestDecoder(httpRequest);
    return decoder.getRequestBodyAsMap();
  }
}
