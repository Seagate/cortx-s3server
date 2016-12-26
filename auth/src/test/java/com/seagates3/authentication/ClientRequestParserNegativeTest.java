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
 * Original creation date: 15-Dec-2016
 */
package com.seagates3.authentication;

import io.netty.handler.codec.http.DefaultFullHttpRequest;
import io.netty.handler.codec.http.FullHttpRequest;
import io.netty.handler.codec.http.HttpMethod;
import io.netty.handler.codec.http.HttpVersion;
import junitparams.JUnitParamsRunner;
import junitparams.Parameters;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import java.util.Map;
import java.util.TreeMap;

import static org.junit.Assert.assertNull;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

@RunWith(JUnitParamsRunner.class)
public class ClientRequestParserNegativeTest {

    private FullHttpRequest httpRequest;
    private Map<String, String> requestBody;

    @Before
    public void setUp() {
        httpRequest = mock(FullHttpRequest.class);
        requestBody = mock(Map.class);
    }

    @Test
    @Parameters({"AuthenticateUser",
            "AuthorizeUser"})
    public void parseTest_AuthorizationHeaderNull(String requestAction) {
        when(requestBody.get("Action")).thenReturn(requestAction);

        assertNull(ClientRequestParser.parse(httpRequest, requestBody));
    }

    @Test
    @Parameters({"OtherAction"})
    public void parseTest_OtherAction_AuthorizationHeaderNull(String requestAction) {
        FullHttpRequest fullHttpRequest
                = new DefaultFullHttpRequest(HttpVersion.HTTP_1_1, HttpMethod.GET, "/");
        fullHttpRequest.headers().add("XYZ", "");
        when(requestBody.get("Action")).thenReturn(requestAction);
        when(httpRequest.headers()).thenReturn(fullHttpRequest.headers());

        assertNull(ClientRequestParser.parse(httpRequest, requestBody));
    }

    @Test(expected = NullPointerException.class)
    public void parseNullPointerExceptionTest() {
        FullHttpRequest httpRequest = null;
        Map<String, String> requestBody = null;

        ClientRequestParser.parse(httpRequest, requestBody);
    }

    @Test(expected = NullPointerException.class)
    public void parseNullPointerExceptionTest1() {
        FullHttpRequest httpRequest = null;
        Map<String, String> requestBody = getRequestHeadersPathStyle();

        ClientRequestParser.parse(httpRequest, requestBody);
    }

    private static Map<String, String> getRequestHeadersPathStyle() {
        Map<String, String> requestBody
                = new TreeMap<>(String.CASE_INSENSITIVE_ORDER);

        requestBody.put("Accept-Encoding", "identity");
        requestBody.put("Action", "AuthenticateUser");
        requestBody.put("Authorization", "AWS AKIAJTYX36YCKQSAJT7Q:uDWiVvxwCUR"
                + "9YJ8EGJgbtW9tjFM=");
        requestBody.put("ClientAbsoluteUri", "/seagatebucket/test.txt");
        requestBody.put("ClientQueryParams", "");
        requestBody.put("Content-Length", "8");
        requestBody.put("content-type", "text/plain");
        requestBody.put("Host", "s3.seagate.com");
        requestBody.put("Method", "PUT");
        requestBody.put("Version", "2010-05-08");
        requestBody.put("x-amz-meta-s3cmd-attrs", "uid:0/gname:root/uname:root/"
                + "gid:0/mode:33188/mtime:1458639989/atime:1458640002/md5:eb1a"
                + "3227cdc3fedbaec2fe38bf6c044a/ctime:1458639989");
        requestBody.put("x-amz-date", "Tue, 22 Mar 2016 09:46:54 +0000");
        requestBody.put("x-amz-storage-class", "STANDARD");

        return requestBody;
    }
}
