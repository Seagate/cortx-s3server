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

import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyLong;
import static org.mockito.Matchers.anyString;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.powermock.api.mockito.PowerMockito.doNothing;
import static org.powermock.api.mockito.PowerMockito.mockStatic;
import static org.powermock.api.mockito.PowerMockito.spy;
import static org.powermock.api.mockito.PowerMockito.verifyPrivate;
import static org.powermock.api.mockito.PowerMockito.when;
import static org.powermock.api.mockito.PowerMockito.whenNew;

import java.io.File;
import java.io.IOException;
import java.io.RandomAccessFile;

import javax.activation.MimetypesFileTypeMap;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.powermock.api.mockito.mockpolicies.Slf4jMockPolicy;
import org.powermock.core.classloader.annotations.MockPolicy;
import org.powermock.core.classloader.annotations.PowerMockIgnore;
import org.powermock.core.classloader.annotations.PrepareForTest;
import org.powermock.modules.junit4.PowerMockRunner;
import org.powermock.reflect.internal.WhiteboxImpl;

import com.seagates3.controller.SAMLWebSSOController;
import com.seagates3.response.ServerResponse;

import io.netty.buffer.ByteBuf;
import io.netty.buffer.ByteBufUtil;
import io.netty.buffer.Unpooled;
import io.netty.channel.ChannelFuture;
import io.netty.channel.ChannelFutureListener;
import io.netty.channel.ChannelHandlerContext;
import io.netty.channel.ChannelPipeline;
import io.netty.channel.ChannelProgressivePromise;
import io.netty.channel.DefaultFileRegion;
import io.netty.handler.codec.http.DefaultFullHttpRequest;
import io.netty.handler.codec.http.FullHttpRequest;
import io.netty.handler.codec.http.HttpChunkedInput;
import io.netty.handler.codec.http.HttpMethod;
import io.netty.handler.codec.http.HttpResponse;
import io.netty.handler.codec.http.HttpResponseStatus;
import io.netty.handler.codec.http.HttpUtil;
import io.netty.handler.codec.http.HttpVersion;
import io.netty.handler.codec.http.LastHttpContent;
import io.netty.handler.ssl.SslHandler;

@PowerMockIgnore({"javax.management.*"}) @RunWith(PowerMockRunner.class)
    @PrepareForTest({AuthServerGetHandler.class, HttpUtil.class,
                     AuthServerConfig.class})
    @MockPolicy(Slf4jMockPolicy.class) public class AuthServerGetHandlerTest {

    private FullHttpRequest fullHttpRequest;
    private ChannelHandlerContext ctx;
    private AuthServerGetHandler handler;

    @Before
    public void setUp() throws Exception {
      mockStatic(HttpUtil.class);
        ctx = mock(ChannelHandlerContext.class);
        fullHttpRequest =
            new DefaultFullHttpRequest(HttpVersion.HTTP_1_1, HttpMethod.POST,
                                       "/", getRequestBodyAsByteBuf());
        when(HttpUtil.isKeepAlive(fullHttpRequest)).thenReturn(Boolean.FALSE);
    }

    @Test
    public void runTest() {
        ChannelFuture channelFuture = mock(ChannelFuture.class);
        fullHttpRequest.setUri("/");
        when(ctx.write(any(ServerResponse.class))).thenReturn(channelFuture);
        handler = new AuthServerGetHandler(ctx, fullHttpRequest);

        handler.run();

        verify(ctx).write(any(HttpResponse.class));
        verify(channelFuture).addListener(ChannelFutureListener.CLOSE);
    }

    @Test
    public void runTest_KeepAlive() {
        ChannelFuture channelFuture = mock(ChannelFuture.class);
        when(HttpUtil.isKeepAlive(fullHttpRequest)).thenReturn(Boolean.TRUE);
        fullHttpRequest.setUri("/");
        when(ctx.write(any(ServerResponse.class))).thenReturn(channelFuture);
        handler = new AuthServerGetHandler(ctx, fullHttpRequest);

        handler.run();

        verify(ctx).write(any(HttpResponse.class));
        verify(channelFuture).addListener(ChannelFutureListener.CLOSE);
    }

    @Test
    public void runTest_Static() throws Exception {
        RandomAccessFile rf = mock(RandomAccessFile.class);
        fullHttpRequest.setUri("/static");
        whenNew(RandomAccessFile.class).withAnyArguments().thenReturn(rf);
        handler = new AuthServerGetHandler(ctx, fullHttpRequest);
        AuthServerGetHandler getHandlerSpy = spy(handler);
        doNothing().when(getHandlerSpy, "writeHeader", any(File.class), anyLong());
        doNothing().when(getHandlerSpy, "writeContent", any(RandomAccessFile.class), anyLong());

        getHandlerSpy.run();

        verifyPrivate(getHandlerSpy).invoke("writeHeader", any(File.class), anyLong());
        verifyPrivate(getHandlerSpy).invoke("writeContent",  any(RandomAccessFile.class),
                anyLong());
        rf.close();
    }

    @Test
    public void runTest_Static_IOException() throws Exception {
        RandomAccessFile rf = mock(RandomAccessFile.class);
        fullHttpRequest.setUri("/static");
        whenNew(RandomAccessFile.class).withAnyArguments().thenReturn(rf);
        when(rf.length()).thenThrow(IOException.class);
        handler = new AuthServerGetHandler(ctx, fullHttpRequest);
        AuthServerGetHandler getHandlerSpy = spy(handler);
        doNothing().when(getHandlerSpy, "sendErrorResponse",
                HttpResponseStatus.INTERNAL_SERVER_ERROR,
                "Error occurred while reading the file."
        );

        getHandlerSpy.run();

        verifyPrivate(getHandlerSpy).invoke("sendErrorResponse",
                HttpResponseStatus.INTERNAL_SERVER_ERROR, "Error occurred while reading the file."
        );
        rf.close();
    }

    @Test
    public void runTest_Static_FileNotFoundException() throws Exception {
      fullHttpRequest.setUri("/static");
        handler = new AuthServerGetHandler(ctx, fullHttpRequest);
        AuthServerGetHandler getHandlerSpy = spy(handler);
        doNothing().when(getHandlerSpy, "sendErrorResponse", HttpResponseStatus.NOT_FOUND,
                "Resource not found.");

        getHandlerSpy.run();

        verifyPrivate(getHandlerSpy).invoke("sendErrorResponse", HttpResponseStatus.NOT_FOUND,
                "Resource not found.");
    }

    @Test
    public void runTest_SAML_FileNotFoundException() throws Exception {
        SAMLWebSSOController controller = mock(SAMLWebSSOController.class);
        fullHttpRequest.setUri("/saml/session/xyz");
        handler = new AuthServerGetHandler(ctx, fullHttpRequest);
        AuthServerGetHandler getHandlerSpy = spy(handler);
        whenNew(SAMLWebSSOController.class).withAnyArguments().thenReturn(controller);
        doNothing().when(getHandlerSpy, "returnHTTPResponse", any(ServerResponse.class));

        getHandlerSpy.run();

        verifyPrivate(getHandlerSpy).invoke("returnHTTPResponse", any(ServerResponse.class));
    }

    @Test
    public void returnHTTPResponseTest_ServerResponse() throws Exception {
        ServerResponse requestResponse = mock(ServerResponse.class);
        ChannelFuture channelFuture = mock(ChannelFuture.class);
        String responseBody = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>";
        when(requestResponse.getResponseBody()).thenReturn(responseBody);
        when(requestResponse.getResponseStatus()).thenReturn(HttpResponseStatus.OK);
        when(ctx.write(any(ServerResponse.class))).thenReturn(channelFuture);

        handler = new AuthServerGetHandler(ctx, fullHttpRequest);
        WhiteboxImpl.invokeMethod(handler, "returnHTTPResponse",
                requestResponse);

        verify(ctx).write(any(ServerResponse.class));
        verify(channelFuture).addListener(ChannelFutureListener.CLOSE);
    }

    @Test
    public void returnHTTPResponseTest_ServerResponse_KeepAlive() throws Exception {
        ServerResponse requestResponse = mock(ServerResponse.class);
        String responseBody = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>";
        when(requestResponse.getResponseBody()).thenReturn(responseBody);
        when(requestResponse.getResponseStatus()).thenReturn(HttpResponseStatus.OK);
        when(HttpUtil.isKeepAlive(fullHttpRequest)).thenReturn(Boolean.TRUE);

        handler = new AuthServerGetHandler(ctx, fullHttpRequest);
        WhiteboxImpl.invokeMethod(handler, "returnHTTPResponse", requestResponse);

        verify(ctx).writeAndFlush(any(ServerResponse.class));
    }

    @Test
    public void writeHeaderTest() throws Exception {
        File file = mock(File.class);
        MimetypesFileTypeMap mimeTypesMap = mock(MimetypesFileTypeMap.class);
        whenNew(MimetypesFileTypeMap.class).withNoArguments().thenReturn(mimeTypesMap);
        when(mimeTypesMap.getContentType(anyString())).thenReturn("text/plain");

        handler = new AuthServerGetHandler(ctx, fullHttpRequest);
        WhiteboxImpl.invokeMethod(handler, "writeHeader", file, 1024L);

        verify(ctx).write(any(ServerResponse.class));
    }

    @Test
    public void writeHeaderTest_KeepAlive() throws Exception {
        File file = mock(File.class);
        MimetypesFileTypeMap mimeTypesMap = mock(MimetypesFileTypeMap.class);
        whenNew(MimetypesFileTypeMap.class).withNoArguments().thenReturn(mimeTypesMap);
        when(mimeTypesMap.getContentType(anyString())).thenReturn("text/plain");
        when(HttpUtil.isKeepAlive(fullHttpRequest)).thenReturn(Boolean.TRUE);

        handler = new AuthServerGetHandler(ctx, fullHttpRequest);
        WhiteboxImpl.invokeMethod(handler, "writeHeader", file, 1024L);

        verify(ctx).write(any(ServerResponse.class));
    }

    @Test
    public void writeContentTest_SslHandler_Null() throws Exception {
        RandomAccessFile raf = mock(RandomAccessFile.class);
        ChannelPipeline pipeline = mock(ChannelPipeline.class);
        ChannelFuture lastContentFuture = mock(ChannelFuture.class);
        when(ctx.pipeline()).thenReturn(pipeline);
        when(pipeline.get(SslHandler.class)).thenReturn(null);
        when(ctx.writeAndFlush(LastHttpContent.EMPTY_LAST_CONTENT)).thenReturn(lastContentFuture);

        handler = new AuthServerGetHandler(ctx, fullHttpRequest);
        WhiteboxImpl.invokeMethod(handler, "writeContent", raf, 1024L);

        verify(ctx).write(any(DefaultFileRegion.class), any(ChannelProgressivePromise.class));
        verify(ctx).writeAndFlush(LastHttpContent.EMPTY_LAST_CONTENT);
        verify(lastContentFuture).addListener(ChannelFutureListener.CLOSE);
        raf.close();
    }

    @Test
    public void writeContentTest_KeepAlive_SslHandler_Null() throws Exception {
        RandomAccessFile raf = mock(RandomAccessFile.class);
        ChannelPipeline pipeline = mock(ChannelPipeline.class);
        ChannelFuture lastContentFuture = mock(ChannelFuture.class);
        when(ctx.pipeline()).thenReturn(pipeline);
        when(pipeline.get(SslHandler.class)).thenReturn(null);
        when(ctx.writeAndFlush(LastHttpContent.EMPTY_LAST_CONTENT)).thenReturn(lastContentFuture);
        when(HttpUtil.isKeepAlive(fullHttpRequest)).thenReturn(Boolean.TRUE);

        handler = new AuthServerGetHandler(ctx, fullHttpRequest);
        WhiteboxImpl.invokeMethod(handler, "writeContent", raf, 1024L);

        verify(ctx).write(any(DefaultFileRegion.class), any(ChannelProgressivePromise.class));
        verify(ctx).writeAndFlush(LastHttpContent.EMPTY_LAST_CONTENT);
        verify(lastContentFuture, times(0)).addListener(ChannelFutureListener.CLOSE);
        raf.close();
    }

    @Test
    public void writeContentTest() throws Exception {
        RandomAccessFile raf = mock(RandomAccessFile.class);
        ChannelPipeline pipeline = mock(ChannelPipeline.class);
        SslHandler sslHandler = mock(SslHandler.class);
        ChannelFuture lastContentFuture = mock(ChannelFuture.class);
        when(ctx.pipeline()).thenReturn(pipeline);
        when(pipeline.get(SslHandler.class)).thenReturn(sslHandler);
        when(ctx.writeAndFlush(any(HttpChunkedInput.class),
                any(ChannelProgressivePromise.class))).thenReturn(lastContentFuture);

        handler = new AuthServerGetHandler(ctx, fullHttpRequest);
        WhiteboxImpl.invokeMethod(handler, "writeContent", raf, 1024L);

        verify(ctx).writeAndFlush(any(HttpChunkedInput.class),
                any(ChannelProgressivePromise.class));
        verify(lastContentFuture).addListener(ChannelFutureListener.CLOSE);
        raf.close();
    }

    @Test
    public void writeContentTest_keepAlive_IOException() throws Exception {
        RandomAccessFile raf = mock(RandomAccessFile.class);
        ChannelPipeline pipeline = mock(ChannelPipeline.class);
        SslHandler sslHandler = mock(SslHandler.class);
        when(ctx.pipeline()).thenReturn(pipeline);
        when(pipeline.get(SslHandler.class)).thenReturn(sslHandler);
        when(ctx.writeAndFlush(any(HttpChunkedInput.class),
                any(ChannelProgressivePromise.class))).thenThrow(IOException.class);
        when(HttpUtil.isKeepAlive(fullHttpRequest)).thenReturn(Boolean.TRUE);

        handler = new AuthServerGetHandler(ctx, fullHttpRequest);
        WhiteboxImpl.invokeMethod(handler, "writeContent", raf, 1024L);

        verify(ctx).writeAndFlush(any(HttpChunkedInput.class),
                any(ChannelProgressivePromise.class));
        raf.close();
    }

    @Test
    public void sendErrorResponseTest() throws Exception {
        ChannelFuture channelFuture = mock(ChannelFuture.class);
        when(ctx.write(any(ServerResponse.class))).thenReturn(channelFuture);

        handler = new AuthServerGetHandler(ctx, fullHttpRequest);
        WhiteboxImpl.invokeMethod(handler, "sendErrorResponse", HttpResponseStatus.NOT_FOUND,
                "Resource not found.");

        verify(ctx).write(any(ServerResponse.class));
        verify(channelFuture).addListener(ChannelFutureListener.CLOSE);
    }

    @Test
    public void sendErrorResponseTest_KeepAlive() throws Exception {
      when(HttpUtil.isKeepAlive(fullHttpRequest)).thenReturn(Boolean.TRUE);

        handler = new AuthServerGetHandler(ctx, fullHttpRequest);
        WhiteboxImpl.invokeMethod(handler, "sendErrorResponse", HttpResponseStatus.NOT_FOUND,
                "Resource not found.");

        verify(ctx).writeAndFlush(any(ServerResponse.class));
    }
   private
    ByteBuf getRequestBodyAsByteBuf() {
      String params =
          "Action=AuthenticateUser&" + "x-amz-meta-ics.meta-version=1&" +
          "x-amz-meta-ics.meta-version=2&" + "samlAssertion=assertion";
      ByteBuf byteBuf = Unpooled.buffer(params.length());
      ByteBufUtil.writeUtf8(byteBuf, params);

      return byteBuf;
    }
}
