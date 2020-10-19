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

import com.seagates3.perf.S3Perf;

import io.netty.channel.ChannelHandlerContext;
import io.netty.channel.SimpleChannelInboundHandler;
import io.netty.handler.codec.http.FullHttpRequest;
import io.netty.handler.codec.http.HttpMethod;

/**
 * Extend the Simple Channel Inbound Handler to create a custom handler for
 * authentication and authorization server.
 */
public
class AuthServerHandler extends SimpleChannelInboundHandler<FullHttpRequest> {

 private
  final Logger LOGGER =
      LoggerFactory.getLogger(AuthServerHandler.class.getName());

  /**
   *
   * @param ctx
   */
  @Override public void channelReadComplete(ChannelHandlerContext ctx) {
    ctx.flush();
  }

  /**
   * Entry point to the server handler. Perform basic validation like check if
   *the
   * request is an "FullHttpRequest" object etc and then pass the control to
   * "AuthServerPostHandler" to handle 'Post' requests and
   *"AuthServerGetHandler"
   * to handle 'Get' requests.
   *
   * @param ctx         Channel Hander Context object.
   * @param httpRequest Instance of FullHttpRequest
   */
  @Override public void channelRead0(ChannelHandlerContext ctx,
                                     FullHttpRequest httpRequest) {
    S3Perf perf = new S3Perf();
    perf.startClock();
    LOGGER.debug("Channel read succesful.");
    LOGGER.debug("Http method - " + httpRequest.method());
    LOGGER.debug("URI - " + httpRequest.uri());

    if (httpRequest.method().equals(HttpMethod.POST)) {
      new AuthServerPostHandler(ctx, httpRequest).run();
    } else if (httpRequest.method().equals(HttpMethod.GET)) {
      new AuthServerGetHandler(ctx, httpRequest).run();
    } else if (httpRequest.method().equals(HttpMethod.HEAD)) {
      new AuthServerHeadHandler(ctx, httpRequest).run();
    }
    perf.endClock();
    perf.printTime("AuthServerHandler");
  }

  @Override public void exceptionCaught(ChannelHandlerContext ctx,
                                        Throwable cause) {
    ctx.close();
  }
}
