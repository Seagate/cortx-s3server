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

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import io.netty.channel.ChannelFutureListener;
import io.netty.channel.ChannelHandlerContext;
import io.netty.handler.codec.http.DefaultFullHttpResponse;
import io.netty.handler.codec.http.FullHttpRequest;
import io.netty.handler.codec.http.FullHttpResponse;
import io.netty.handler.codec.http.HttpHeaderNames;
import io.netty.handler.codec.http.HttpHeaderValues;
import io.netty.handler.codec.http.HttpResponseStatus;
import io.netty.handler.codec.http.HttpUtil;
import io.netty.handler.codec.http.HttpVersion;

public
class AuthServerHeadHandler {

 private
  final Logger LOGGER =
      LoggerFactory.getLogger(AuthServerHeadHandler.class.getName());

  final ChannelHandlerContext ctx;
  final FullHttpRequest httpRequest;
  final Boolean keepAlive;

 public
  AuthServerHeadHandler(ChannelHandlerContext ctx,
                        FullHttpRequest httpRequest) {
    this.ctx = ctx;
    this.httpRequest = httpRequest;
    keepAlive = HttpUtil.isKeepAlive(httpRequest);
  }

 public
  void run() {
    if (httpRequest.uri().startsWith("/auth/health")) {
      // Generate Auth Server health check response with Status 200
      FullHttpResponse response;
      response = new DefaultFullHttpResponse(HttpVersion.HTTP_1_1,
                                             HttpResponseStatus.OK);
      LOGGER.debug("Sending Auth server health check response with status [" +
                   response.status() + "]");

      response.headers().set(HttpHeaderNames.CONTENT_TYPE, "text/xml");
      response.headers().set(HttpHeaderNames.CONTENT_LENGTH,
                             response.content().readableBytes());

      if (!keepAlive) {
        ctx.write(response).addListener(ChannelFutureListener.CLOSE);
        LOGGER.debug("Response sent successfully and Connection was closed.");
      } else {
        response.headers().set(HttpHeaderNames.CONNECTION,
                               HttpHeaderValues.KEEP_ALIVE);
        ctx.writeAndFlush(response);
        LOGGER.debug("Response sent successfully and Connection kept alive.");
      }
    }
  }
}
