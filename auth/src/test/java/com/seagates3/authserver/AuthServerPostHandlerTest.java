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

import com.seagates3.controller.FaultPointsController;
import com.seagates3.controller.IAMController;
import com.seagates3.controller.SAMLWebSSOController;
import com.seagates3.response.ServerResponse;
import io.netty.buffer.ByteBuf;
import io.netty.buffer.ByteBufUtil;
import io.netty.buffer.Unpooled;
import io.netty.channel.ChannelFuture;
import io.netty.channel.ChannelFutureListener;
import io.netty.channel.ChannelHandlerContext;
import io.netty.handler.codec.http.DefaultFullHttpRequest;
import io.netty.handler.codec.http.DefaultFullHttpResponse;
import io.netty.handler.codec.http.FullHttpRequest;
import io.netty.handler.codec.http.FullHttpResponse;
import io.netty.handler.codec.http.HttpHeaders;
import io.netty.handler.codec.http.HttpMethod;
import io.netty.handler.codec.http.HttpResponseStatus;
import io.netty.handler.codec.http.HttpVersion;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.powermock.api.mockito.mockpolicies.Slf4jMockPolicy;
import org.powermock.core.classloader.annotations.MockPolicy;
import org.powermock.core.classloader.annotations.PrepareForTest;
import org.powermock.modules.junit4.PowerMockRunner;
import org.powermock.reflect.internal.WhiteboxImpl;

import java.util.Map;
import java.util.TreeMap;

import static org.junit.Assert.*;
import static org.mockito.Matchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.powermock.api.mockito.PowerMockito.mockStatic;
import static org.powermock.api.mockito.PowerMockito.verifyPrivate;
import static org.powermock.api.mockito.PowerMockito.when;
import static org.powermock.api.mockito.PowerMockito.whenNew;

@RunWith(PowerMockRunner.class)
@PrepareForTest({AuthServerPostHandler.class, HttpHeaders.class, AuthServerConfig.class})
@MockPolicy(Slf4jMockPolicy.class)
public class AuthServerPostHandlerTest {

    private ChannelHandlerContext ctx;
    private FullHttpRequest fullHttpRequest;
    private AuthServerPostHandler handler;

    @Before
    public void setUp() throws Exception {
        fullHttpRequest = new DefaultFullHttpRequest(
                HttpVersion.HTTP_1_1, HttpMethod.POST, "/", getRequestBodyAsByteBuf());
        ctx = mock(ChannelHandlerContext.class);

        mockStatic(AuthServerConfig.class);
    }

    @Test
    public void runTest() throws Exception {
        handler = new AuthServerPostHandler(ctx, fullHttpRequest);
        handler.run();

        verifyPrivate(handler).invoke("serveIamRequest", getExpectedBody());
    }

    @Test
    public void runTest_SAML_Mock() throws Exception {
        FullHttpResponse response = mock(FullHttpResponse.class);
        SAMLWebSSOController controller = mock(SAMLWebSSOController.class);
        Map<String, String> requestBody = getExpectedBody();
        whenNew(SAMLWebSSOController.class).withArguments(requestBody).thenReturn(controller);
        when(controller.samlSignIn()).thenReturn(response);
        HttpHeaders httpHeaders = mock(HttpHeaders.class);
        when(response.headers()).thenReturn(httpHeaders);
        fullHttpRequest.setUri("/saml");

        handler = new AuthServerPostHandler(ctx, fullHttpRequest);
        handler.run();

        verifyPrivate(handler).invoke("returnHTTPResponse", response);
    }

    @Test
    public void returnHTTPResponseTest_ServerResponse_KeepAlive() throws Exception {
        ServerResponse requestResponse = mock(ServerResponse.class);
        String responseBody = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>";
        when(requestResponse.getResponseBody()).thenReturn(responseBody);
        when(requestResponse.getResponseStatus()).thenReturn(HttpResponseStatus.OK);

        handler = new AuthServerPostHandler(ctx, fullHttpRequest);
        WhiteboxImpl.invokeMethod(handler, "returnHTTPResponse", requestResponse);

        verify(ctx).writeAndFlush(any(ServerResponse.class));
    }

    @Test
    public void returnHTTPResponseTest_ServerResponse() throws Exception {
        ServerResponse requestResponse = mock(ServerResponse.class);
        String responseBody = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>";
        when(requestResponse.getResponseBody()).thenReturn(responseBody);
        when(requestResponse.getResponseStatus()).thenReturn(HttpResponseStatus.OK);

        mockStatic(HttpHeaders.class);
        when(HttpHeaders.isKeepAlive(fullHttpRequest)).thenReturn(Boolean.FALSE);
        ChannelFuture channelFuture = mock(ChannelFuture.class);
        when(ctx.write(any(ServerResponse.class))).thenReturn(channelFuture);

        handler = new AuthServerPostHandler(ctx, fullHttpRequest);
        WhiteboxImpl.invokeMethod(handler, "returnHTTPResponse", requestResponse);

        verify(ctx).write(any(ServerResponse.class));
        verify(channelFuture).addListener(ChannelFutureListener.CLOSE);
    }

    @Test
    public void returnHTTPResponseTest_FullHttpResponse_KeepAlive() throws Exception {
        FullHttpResponse response = new DefaultFullHttpResponse(HttpVersion.HTTP_1_1,
                HttpResponseStatus.OK);

        handler = new AuthServerPostHandler(ctx, fullHttpRequest);
        WhiteboxImpl.invokeMethod(handler, "returnHTTPResponse", response);

        verify(ctx).writeAndFlush(response);
    }

    @Test
    public void returnHTTPResponseTest_FullHttpResponse() throws Exception {
        FullHttpResponse response = new DefaultFullHttpResponse(HttpVersion.HTTP_1_1,
                HttpResponseStatus.OK);
        mockStatic(HttpHeaders.class);
        when(HttpHeaders.isKeepAlive(fullHttpRequest)).thenReturn(Boolean.FALSE);
        ChannelFuture channelFuture = mock(ChannelFuture.class);
        when(ctx.write(any(ServerResponse.class))).thenReturn(channelFuture);

        handler = new AuthServerPostHandler(ctx, fullHttpRequest);
        WhiteboxImpl.invokeMethod(handler, "returnHTTPResponse", response);

        verify(ctx).write(response);
        verify(channelFuture).addListener(ChannelFutureListener.CLOSE);
    }

    @Test
    public void getHttpRequestBodyAsMapTest() throws Exception {
        handler = new AuthServerPostHandler(ctx, fullHttpRequest);
        Map<String, String> result = WhiteboxImpl.invokeMethod(handler,
                "getHttpRequestBodyAsMap");

        assertEquals(getExpectedBody(), result);
    }

    @Test
    public void isFiRequestTest() throws Exception {
        handler = new AuthServerPostHandler(ctx, fullHttpRequest);
        Boolean result = WhiteboxImpl.invokeMethod(handler, "isFiRequest", "Authenticate");

        assertFalse(result);
    }

    @Test
    public void isFiRequestTest_True() throws Exception {
        handler = new AuthServerPostHandler(ctx, fullHttpRequest);
        Boolean result = WhiteboxImpl.invokeMethod(handler, "isFiRequest", "ResetFault");

        assertTrue(result);
    }

    @Test
    public void serveFiRequestTest_FiDisabled_BadRequest() throws Exception {
        String expectedResponse = "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>" +
                "<Error xmlns=\"https://iam.seagate.com/doc/2010-05-08/\"><Code>BadRequest</Code>" +
                "<Message>Invalid Request</Message><RequestId>0000</RequestId></Error>";

        handler = new AuthServerPostHandler(ctx, fullHttpRequest);
        ServerResponse result = WhiteboxImpl.invokeMethod(handler, "serveFiRequest",
                getExpectedBody());

        assertEquals(HttpResponseStatus.BAD_REQUEST, result.getResponseStatus());
        assertEquals(expectedResponse, result.getResponseBody());
    }

    @Test
    public void serveFiRequestTest_FiEnabled_InjectFault() throws Exception {
        Map<String, String> requestMap = getExpectedBody();
        requestMap.put("Action", "InjectFault");
        FaultPointsController faultPointsController = mock(FaultPointsController.class);
        whenNew(FaultPointsController.class).withNoArguments().thenReturn(faultPointsController);
        when(AuthServerConfig.isFaultInjectionEnabled()).thenReturn(Boolean.TRUE);

        handler = new AuthServerPostHandler(ctx, fullHttpRequest);
        WhiteboxImpl.invokeMethod(handler, "serveFiRequest", requestMap);

        verify(faultPointsController).set(requestMap);
        verify(faultPointsController, times(0)).reset(requestMap);
    }

    @Test
    public void serveFiRequestTest_FiEnabled_ResetFault() throws Exception {
        Map<String, String> requestMap = getExpectedBody();
        requestMap.put("Action", "ResetFault");
        FaultPointsController faultPointsController = mock(FaultPointsController.class);
        whenNew(FaultPointsController.class).withNoArguments().thenReturn(faultPointsController);
        when(AuthServerConfig.isFaultInjectionEnabled()).thenReturn(Boolean.TRUE);

        handler = new AuthServerPostHandler(ctx, fullHttpRequest);
        WhiteboxImpl.invokeMethod(handler, "serveFiRequest", requestMap);

        verify(faultPointsController).reset(requestMap);
        verify(faultPointsController, times(0)).set(requestMap);
    }

    @Test
    public void serveIamRequestTest() throws Exception {
        Map<String, String> requestMap = null;
        IAMController iamController = mock(IAMController.class);
        whenNew(IAMController.class).withNoArguments().thenReturn(iamController);

        handler = new AuthServerPostHandler(ctx, fullHttpRequest);
        WhiteboxImpl.invokeMethod(handler, "serveIamRequest", requestMap);

        verify(iamController).serve(fullHttpRequest, requestMap);
    }

    private ByteBuf getRequestBodyAsByteBuf() {
        String params = "Action=AuthenticateUser&" +
                "x-amz-meta-ics.simpleencryption.keyinfo=" +
                "Wrapped%3AAES%3A256%3Ax%2B7OEfeP0NgG1yG96%2FiD9F5hsyU060FWyjSqo1LjzBY%3D&" +
                "x-amz-meta-ics.simpleencryption.status=enabled&" +
                "x-amz-meta-ics.etagintegrity=S3%3Anone&" +
                "x-amz-meta-ics.simpleencryption.plaintextcontentlength=568&" +
                "x-amz-meta-ics.simpleencryption.numwrpkeys=1&" +
                "x-amz-meta-ics.simpleencryption.key0=k000%3Alkm.keybackend0&" +
                "x-amz-meta-ics.simpleencryption.wrpparams=" +
                "AES%3A256%3ACBC%3ACtpgYEqfK5Z12zpEoVxl4w%3D%3D&" +
                "x-amz-meta-ics.simpleencryption.cmbparams=xor%3AHmacSHA256&" +
                "x-amz-meta-ics.stack-description=ICStore.1%28SimpleEncryption." +
                "1%28EtagIntegrity.1%28BlobStoreConnection.1%29%29%29&" +
                "x-amz-meta-ics.meta-digest=71e7bb5fdf6784e6f9b889b2d0d39818&" +
                "x-amz-meta-ics.etagintegrity.status=enabled&" +
                "x-amz-meta-ics.meta-version=1&" +
                "x-amz-meta-ics.meta-version=2&" +
                "samlAssertion=assertion";
        ByteBuf byteBuf = Unpooled.buffer(params.length());
        ByteBufUtil.writeUtf8(byteBuf, params);

        return byteBuf;
    }

    private Map<String, String> getExpectedBody() {
        Map<String, String> expectedRequestBody = new TreeMap<>(String.CASE_INSENSITIVE_ORDER);
        expectedRequestBody.put("Action", "AuthenticateUser");
        expectedRequestBody.put("samlAssertion", "assertion");
        expectedRequestBody.put("x-amz-meta-ics.simpleencryption.keyinfo",
                "Wrapped:AES:256:x+7OEfeP0NgG1yG96/iD9F5hsyU060FWyjSqo1LjzBY=");
        expectedRequestBody.put("x-amz-meta-ics.simpleencryption.status", "enabled");
        expectedRequestBody.put("x-amz-meta-ics.etagintegrity", "S3:none");
        expectedRequestBody.put("x-amz-meta-ics.simpleencryption.plaintextcontentlength", "568");
        expectedRequestBody.put("x-amz-meta-ics.simpleencryption.numwrpkeys", "1");
        expectedRequestBody.put("x-amz-meta-ics.simpleencryption.key0", "k000:lkm.keybackend0");
        expectedRequestBody.put("x-amz-meta-ics.simpleencryption.wrpparams",
                "AES:256:CBC:CtpgYEqfK5Z12zpEoVxl4w==");
        expectedRequestBody.put("x-amz-meta-ics.simpleencryption.cmbparams", "xor:HmacSHA256");
        expectedRequestBody.put("x-amz-meta-ics.stack-description",
                "ICStore.1(SimpleEncryption.1(EtagIntegrity.1(BlobStoreConnection.1)))");
        expectedRequestBody.put("x-amz-meta-ics.meta-digest", "71e7bb5fdf6784e6f9b889b2d0d39818");
        expectedRequestBody.put("x-amz-meta-ics.etagintegrity.status", "enabled");
        expectedRequestBody.put("x-amz-meta-ics.meta-version", "1,2");

        return expectedRequestBody;
    }
}