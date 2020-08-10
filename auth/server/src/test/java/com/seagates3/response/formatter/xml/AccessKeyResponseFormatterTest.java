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

package com.seagates3.response.formatter.xml;

import com.seagates3.response.ServerResponse;
import io.netty.handler.codec.http.HttpResponseStatus;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import org.junit.Assert;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.ExpectedException;

public class AccessKeyResponseFormatterTest {

    @Rule
    public final ExpectedException exception = ExpectedException.none();

    @Test
    public void testFormatListResponse() {
        ArrayList<LinkedHashMap<String, String>> accessKeyMembers = new ArrayList<>();
        LinkedHashMap responseElement1 = new LinkedHashMap();
        responseElement1.put("UserName", "s3user1");
        responseElement1.put("AccessKeyId", "AKIA1213");
        responseElement1.put("Status", "Active");
        responseElement1.put("CreateDate", "2015-12-19T07:20:29.000+0530");

        accessKeyMembers.add(responseElement1);

        LinkedHashMap responseElement2 = new LinkedHashMap();
        responseElement2.put("UserName", "s3user1");
        responseElement2.put("AccessKeyId", "AKIA4567");
        responseElement2.put("Status", "Active");
        responseElement2.put("CreateDate", "2015-12-18T07:20:29.000+0530");

        accessKeyMembers.add(responseElement2);

        final String expectedResponseBody =
            "<?xml version=\"1.0\" " +
            "encoding=\"UTF-8\" standalone=\"no\"?>" +
            "<ListAccessKeysResponse " +
            "xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">" +
            "<ListAccessKeysResult>" + "<UserName>s3user1</UserName>" +
            "<AccessKeyMetadata>" + "<member>" +
            "<UserName>s3user1</UserName>" +
            "<AccessKeyId>AKIA1213</AccessKeyId>" + "<Status>Active</Status>" +
            "<CreateDate>2015-12-19T07:20:29.000+0530</CreateDate>" +
            "</member>" + "<member>" + "<UserName>s3user1</UserName>" +
            "<AccessKeyId>AKIA4567</AccessKeyId>" + "<Status>Active</Status>" +
            "<CreateDate>2015-12-18T07:20:29.000+0530</CreateDate>" +
            "</member>" + "</AccessKeyMetadata>" +
            "<IsTruncated>false</IsTruncated>" + "</ListAccessKeysResult>" +
            "<ResponseMetadata>" + "<RequestId>0000</RequestId>" +
            "</ResponseMetadata>" + "</ListAccessKeysResponse>";

        ServerResponse response = new AccessKeyResponseFormatter().formatListResponse(
                "s3user1", accessKeyMembers, false, "0000");

        Assert.assertEquals(expectedResponseBody, response.getResponseBody());
        Assert.assertEquals(HttpResponseStatus.OK, response.getResponseStatus());
    }

    @Test
    public void testFormatListResponseException() {
        ArrayList< LinkedHashMap< String, String>> responseElements = new ArrayList<>();

        exception.expect(UnsupportedOperationException.class);
        new AccessKeyResponseFormatter().formatListResponse("ListAccessKeys",
                "Accesskey", responseElements, false, "0000");
    }
}
