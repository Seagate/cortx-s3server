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
 * Original creation date: 24-Dec-2015
 */
package com.seagates3.authserver;

import io.netty.channel.ChannelHandlerContext;
import io.netty.handler.codec.http.FullHttpRequest;
import io.netty.handler.codec.http.HttpMethod;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mockito;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;
import static org.powermock.api.mockito.PowerMockito.whenNew;
import org.powermock.core.classloader.annotations.PowerMockIgnore;
import org.powermock.core.classloader.annotations.PrepareForTest;
import org.powermock.modules.junit4.PowerMockRunner;

@RunWith(PowerMockRunner.class)
@PrepareForTest(AuthServerHandler.class)
@PowerMockIgnore({"javax.management.*"})
public class AuthServerHandlerTest {

    AuthServerHandler testHandler;
    ChannelHandlerContext ctx;
    FullHttpRequest msg;

    @Before
    public void setUp() {
        ctx = Mockito.mock(ChannelHandlerContext.class);
        msg = mock(FullHttpRequest.class);
        testHandler = new AuthServerHandler();
    }

    @Test
    public void channelReadTest() throws Exception {
        AuthServerPostHandler postHandler = mock(AuthServerPostHandler.class);

        when(msg.getMethod()).thenReturn(HttpMethod.POST);
        whenNew(AuthServerPostHandler.class).withArguments(
                ctx, msg).thenReturn(postHandler);

        testHandler.channelRead(ctx, msg);
        Mockito.verify(postHandler).run();
    }
}
