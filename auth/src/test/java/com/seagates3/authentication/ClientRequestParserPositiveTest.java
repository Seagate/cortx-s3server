/*
 * COPYRIGHT 2016 SEAGATE LLC
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
 * Original creation date: 26-Dec-2016
 */
package com.seagates3.authentication;

import com.seagates3.authserver.AuthServerConfig;
import com.seagates3.aws.AWSV2RequestHelper;
import com.seagates3.aws.AWSV4RequestHelper;
import io.netty.handler.codec.http.FullHttpRequest;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.powermock.api.mockito.PowerMockito;
import org.powermock.core.classloader.annotations.PrepareForTest;
import org.powermock.modules.junit4.PowerMockRunner;

import java.util.Map;

import static junit.framework.TestCase.assertEquals;
import static junit.framework.TestCase.assertNotNull;
import static org.mockito.Mockito.mock;

@RunWith(PowerMockRunner.class)
@PrepareForTest(AuthServerConfig.class)
public class ClientRequestParserPositiveTest {

    private FullHttpRequest httpRequest = mock(FullHttpRequest.class);
    private Map<String, String> requestBody;

    @Before
    public void setUp() {
        String[] endpoints = {"s3-us.seagate.com",
                "s3-europe.seagate.com", "s3-asia.seagate.com"};
        PowerMockito.mockStatic(AuthServerConfig.class);
        PowerMockito.when(AuthServerConfig.getEndpoints()).thenReturn(endpoints);
    }

    @Test
    public void parseTest_AWSSign2_PathStyle(){
        // Arrange
        requestBody = AWSV2RequestHelper.getRequestHeadersPathStyle();

        // Act
        ClientRequestToken token =  ClientRequestParser.parse(httpRequest, requestBody);

        // Verify
        assertNotNull(token);
        assertEquals("AKIAJTYX36YCKQSAJT7Q", token.getAccessKeyId());
    }

    @Test
    public void parseTest_AWSSign4_VirtualHostStyle(){
        // Arrange
        requestBody = AWSV4RequestHelper.getRequestHeadersVirtualHostStyle();

        // Act
        ClientRequestToken token =  ClientRequestParser.parse(httpRequest, requestBody);

        // Verify
        assertNotNull(token);
        assertEquals("AKIAIOSFODNN7EXAMPLE", token.getAccessKeyId());
    }
}

