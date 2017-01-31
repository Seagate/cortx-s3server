/*
 * COPYRIGHT 2017 SEAGATE LLC
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
 * Original author: Sushant Mane <sushant.mane@seagate.com>
 * Original creation date: 24-Jan-2017
 */
package com.seagates3.authserver;

import com.seagates3.controller.SAMLWebSSOController;
import com.seagates3.response.ServerResponse;
import io.netty.channel.ChannelFuture;
import io.netty.channel.ChannelFutureListener;
import io.netty.channel.ChannelHandlerContext;
import io.netty.channel.ChannelPipeline;
import io.netty.channel.ChannelProgressivePromise;
import io.netty.channel.DefaultFileRegion;
import io.netty.handler.codec.http.FullHttpRequest;
import io.netty.handler.codec.http.HttpChunkedInput;
import io.netty.handler.codec.http.HttpHeaders;
import io.netty.handler.codec.http.HttpResponse;
import io.netty.handler.codec.http.HttpResponseStatus;
import io.netty.handler.codec.http.LastHttpContent;
import io.netty.handler.ssl.SslHandler;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.powermock.api.mockito.mockpolicies.Slf4jMockPolicy;
import org.powermock.core.classloader.annotations.MockPolicy;
import org.powermock.core.classloader.annotations.PrepareForTest;
import org.powermock.modules.junit4.PowerMockRunner;
import org.powermock.reflect.internal.WhiteboxImpl;

import javax.activation.MimetypesFileTypeMap;
import java.io.File;
import java.io.IOException;
import java.io.RandomAccessFile;

import static org.mockito.Matchers.*;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.powermock.api.mockito.PowerMockito.doNothing;
import static org.powermock.api.mockito.PowerMockito.mockStatic;
import static org.powermock.api.mockito.PowerMockito.spy;
import static org.powermock.api.mockito.PowerMockito.verifyPrivate;
import static org.powermock.api.mockito.PowerMockito.when;
import static org.powermock.api.mockito.PowerMockito.whenNew;

@RunWith(PowerMockRunner.class)
@PrepareForTest({AuthServerGetHandler.class, HttpHeaders.class, AuthServerConfig.class})
@MockPolicy(Slf4jMockPolicy.class)
public class AuthServerGetHandlerTest {

    private FullHttpRequest fullHttpRequest;
    private ChannelHandlerContext ctx;
    private AuthServerGetHandler handler;

    @Before
    public void setUp() throws Exception {
        fullHttpRequest = mock(FullHttpRequest.class);
        ctx = mock(ChannelHandlerContext.class);

        mockStatic(HttpHeaders.class);
        when(HttpHeaders.isKeepAlive(fullHttpRequest)).thenReturn(Boolean.FALSE);
    }

    @Test
    public void runTest() {
        when(fullHttpRequest.getUri()).thenReturn("/");
        ChannelFuture channelFuture = mock(ChannelFuture.class);
        when(ctx.write(any(ServerResponse.class))).thenReturn(channelFuture);
        handler = new AuthServerGetHandler(ctx, fullHttpRequest);

        handler.run();

        verify(ctx).write(any(HttpResponse.class));
        verify(channelFuture).addListener(ChannelFutureListener.CLOSE);
    }

    @Test
    public void runTest_KeepAlive() {
        when(HttpHeaders.isKeepAlive(fullHttpRequest)).thenReturn(Boolean.TRUE);
        when(fullHttpRequest.getUri()).thenReturn("/");
        ChannelFuture channelFuture = mock(ChannelFuture.class);
        when(ctx.write(any(ServerResponse.class))).thenReturn(channelFuture);
        handler = new AuthServerGetHandler(ctx, fullHttpRequest);

        handler.run();

        verify(ctx).write(any(HttpResponse.class));
        verify(channelFuture).addListener(ChannelFutureListener.CLOSE);
    }

    @Test
    public void runTest_Static() throws Exception {
        when(fullHttpRequest.getUri()).thenReturn("/static");
        RandomAccessFile rf = mock(RandomAccessFile.class);
        whenNew(RandomAccessFile.class).withAnyArguments().thenReturn(rf);
        handler = new AuthServerGetHandler(ctx, fullHttpRequest);
        AuthServerGetHandler getHandlerSpy = spy(handler);
        doNothing().when(getHandlerSpy, "writeHeader", any(File.class), anyLong());
        doNothing().when(getHandlerSpy, "writeContent", any(RandomAccessFile.class), anyLong());

        getHandlerSpy.run();

        verifyPrivate(getHandlerSpy).invoke("writeHeader", any(File.class), anyLong());
        verifyPrivate(getHandlerSpy).invoke("writeContent",  any(RandomAccessFile.class),
                anyLong());
    }

    @Test
    public void runTest_Static_IOException() throws Exception {
        when(fullHttpRequest.getUri()).thenReturn("/static");
        RandomAccessFile rf = mock(RandomAccessFile.class);
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
                HttpResponseStatus.INTERNAL_SERVER_ERROR,
                "Error occurred while reading the file."
        );
    }

    @Test
    public void runTest_Static_FileNotFoundException() throws Exception {
        when(fullHttpRequest.getUri()).thenReturn("/static");
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
        when(fullHttpRequest.getUri()).thenReturn("/saml/session/xyz");
        handler = new AuthServerGetHandler(ctx, fullHttpRequest);
        AuthServerGetHandler getHandlerSpy = spy(handler);
        SAMLWebSSOController controller = mock(SAMLWebSSOController.class);
        whenNew(SAMLWebSSOController.class).withAnyArguments().thenReturn(controller);
        doNothing().when(getHandlerSpy, "returnHTTPResponse", any(ServerResponse.class));

        getHandlerSpy.run();

        verifyPrivate(getHandlerSpy).invoke("returnHTTPResponse", any(ServerResponse.class));
    }

    @Test
    public void returnHTTPResponseTest_ServerResponse() throws Exception {
        ServerResponse requestResponse = mock(ServerResponse.class);
        String responseBody = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>";
        when(requestResponse.getResponseBody()).thenReturn(responseBody);
        when(requestResponse.getResponseStatus()).thenReturn(HttpResponseStatus.OK);

        ChannelFuture channelFuture = mock(ChannelFuture.class);
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
        when(HttpHeaders.isKeepAlive(fullHttpRequest)).thenReturn(Boolean.TRUE);

        handler = new AuthServerGetHandler(ctx, fullHttpRequest);
        WhiteboxImpl.invokeMethod(handler, "returnHTTPResponse", requestResponse);

        verify(ctx).writeAndFlush(any(ServerResponse.class));
    }

    @Test
    public void writeHeaderTest() throws Exception {
        File file = mock(File.class);
        long fileLength = 1024L;
        MimetypesFileTypeMap mimeTypesMap = mock(MimetypesFileTypeMap.class);
        whenNew(MimetypesFileTypeMap.class).withNoArguments().thenReturn(mimeTypesMap);
        when(mimeTypesMap.getContentType(anyString())).thenReturn("text/plain");

        handler = new AuthServerGetHandler(ctx, fullHttpRequest);
        WhiteboxImpl.invokeMethod(handler, "writeHeader", file, fileLength);

        verify(ctx).write(any(ServerResponse.class));
    }

    @Test
    public void writeHeaderTest_KeepAlive() throws Exception {
        File file = mock(File.class);
        long fileLength = 1024L;
        MimetypesFileTypeMap mimeTypesMap = mock(MimetypesFileTypeMap.class);
        whenNew(MimetypesFileTypeMap.class).withNoArguments().thenReturn(mimeTypesMap);
        when(mimeTypesMap.getContentType(anyString())).thenReturn("text/plain");
        when(HttpHeaders.isKeepAlive(fullHttpRequest)).thenReturn(Boolean.TRUE);

        handler = new AuthServerGetHandler(ctx, fullHttpRequest);
        WhiteboxImpl.invokeMethod(handler, "writeHeader", file, fileLength);

        verify(ctx).write(any(ServerResponse.class));
    }

    @Test
    public void writeContentTest_SslHandler_Null() throws Exception {
        RandomAccessFile raf = mock(RandomAccessFile.class);
        long fileLength = 1024L;
        ChannelPipeline pipeline = mock(ChannelPipeline.class);
        when(ctx.pipeline()).thenReturn(pipeline);
        when(pipeline.get(SslHandler.class)).thenReturn(null);
        ChannelFuture lastContentFuture = mock(ChannelFuture.class);
        when(ctx.writeAndFlush(LastHttpContent.EMPTY_LAST_CONTENT)).thenReturn(lastContentFuture);

        handler = new AuthServerGetHandler(ctx, fullHttpRequest);
        WhiteboxImpl.invokeMethod(handler, "writeContent", raf, fileLength);

        verify(ctx).write(any(DefaultFileRegion.class), any(ChannelProgressivePromise.class));
        verify(ctx).writeAndFlush(LastHttpContent.EMPTY_LAST_CONTENT);
        verify(lastContentFuture).addListener(ChannelFutureListener.CLOSE);
    }

    @Test
    public void writeContentTest_KeepAlive_SslHandler_Null() throws Exception {
        RandomAccessFile raf = mock(RandomAccessFile.class);
        long fileLength = 1024L;
        ChannelPipeline pipeline = mock(ChannelPipeline.class);
        when(ctx.pipeline()).thenReturn(pipeline);
        when(pipeline.get(SslHandler.class)).thenReturn(null);
        ChannelFuture lastContentFuture = mock(ChannelFuture.class);
        when(ctx.writeAndFlush(LastHttpContent.EMPTY_LAST_CONTENT)).thenReturn(lastContentFuture);
        when(HttpHeaders.isKeepAlive(fullHttpRequest)).thenReturn(Boolean.TRUE);

        handler = new AuthServerGetHandler(ctx, fullHttpRequest);
        WhiteboxImpl.invokeMethod(handler, "writeContent", raf, fileLength);

        verify(ctx).write(any(DefaultFileRegion.class), any(ChannelProgressivePromise.class));
        verify(ctx).writeAndFlush(LastHttpContent.EMPTY_LAST_CONTENT);
        verify(lastContentFuture, times(0)).addListener(ChannelFutureListener.CLOSE);
    }

    @Test
    public void writeContentTest() throws Exception {
        RandomAccessFile raf = mock(RandomAccessFile.class);
        long fileLength = 1024L;
        ChannelPipeline pipeline = mock(ChannelPipeline.class);
        when(ctx.pipeline()).thenReturn(pipeline);
        SslHandler sslHandler = mock(SslHandler.class);
        when(pipeline.get(SslHandler.class)).thenReturn(sslHandler);
        ChannelFuture lastContentFuture = mock(ChannelFuture.class);
        when(ctx.writeAndFlush(any(HttpChunkedInput.class),
                any(ChannelProgressivePromise.class))).thenReturn(lastContentFuture);

        handler = new AuthServerGetHandler(ctx, fullHttpRequest);
        WhiteboxImpl.invokeMethod(handler, "writeContent", raf, fileLength);

        verify(ctx).writeAndFlush(any(HttpChunkedInput.class),
                any(ChannelProgressivePromise.class));
        verify(lastContentFuture).addListener(ChannelFutureListener.CLOSE);
    }

    @Test
    public void writeContentTest_keepAlive_IOException() throws Exception {
        RandomAccessFile raf = mock(RandomAccessFile.class);
        long fileLength = 1024L;
        ChannelPipeline pipeline = mock(ChannelPipeline.class);
        when(ctx.pipeline()).thenReturn(pipeline);
        SslHandler sslHandler = mock(SslHandler.class);
        when(pipeline.get(SslHandler.class)).thenReturn(sslHandler);
        when(ctx.writeAndFlush(any(HttpChunkedInput.class),
                any(ChannelProgressivePromise.class))).thenThrow(IOException.class);
        when(HttpHeaders.isKeepAlive(fullHttpRequest)).thenReturn(Boolean.TRUE);

        handler = new AuthServerGetHandler(ctx, fullHttpRequest);
        WhiteboxImpl.invokeMethod(handler, "writeContent", raf, fileLength);

        verify(ctx).writeAndFlush(any(HttpChunkedInput.class),
                any(ChannelProgressivePromise.class));
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
        when(HttpHeaders.isKeepAlive(fullHttpRequest)).thenReturn(Boolean.TRUE);

        handler = new AuthServerGetHandler(ctx, fullHttpRequest);
        WhiteboxImpl.invokeMethod(handler, "sendErrorResponse", HttpResponseStatus.NOT_FOUND,
                "Resource not found.");

        verify(ctx).writeAndFlush(any(ServerResponse.class));
    }
}