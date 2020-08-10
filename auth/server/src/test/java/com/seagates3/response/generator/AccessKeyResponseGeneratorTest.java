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

package com.seagates3.response.generator;

import com.seagates3.model.AccessKey;
import com.seagates3.response.ServerResponse;
import io.netty.handler.codec.http.HttpResponseStatus;
import java.util.ArrayList;
import org.junit.Assert;
import org.junit.Test;

public class AccessKeyResponseGeneratorTest {

    @Test
    public void testAccessKeyCreateResponse() {
        AccessKey accessKey = new AccessKey();
        accessKey.setId("AKIA1234");
        accessKey.setStatus(AccessKey.AccessKeyStatus.ACTIVE);
        accessKey.setSecretKey("kas-1aks12");

        final String expectedResponseBody = "<?xml version=\"1.0\" "
                + "encoding=\"UTF-8\" standalone=\"no\"?>"
                + "<CreateAccessKeyResponse "
                + "xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">"
                + "<CreateAccessKeyResult>"
                + "<AccessKey>"
                + "<UserName>s3user</UserName>"
                + "<AccessKeyId>AKIA1234</AccessKeyId>"
                + "<Status>Active</Status>"
                + "<SecretAccessKey>kas-1aks12</SecretAccessKey>"
                + "</AccessKey>"
                + "</CreateAccessKeyResult>"
                + "<ResponseMetadata>"
                + "<RequestId>0000</RequestId>"
                + "</ResponseMetadata>"
                + "</CreateAccessKeyResponse>";

        AccessKeyResponseGenerator responseGenerator = new AccessKeyResponseGenerator();
        ServerResponse response = responseGenerator
                .generateCreateResponse("s3user", accessKey);

        Assert.assertEquals(expectedResponseBody, response.getResponseBody());
        Assert.assertEquals(HttpResponseStatus.CREATED, response.getResponseStatus());
    }

    @Test
    public void testAccessKeyListResponse() {
        ArrayList<AccessKey> accessKeys = new ArrayList<>();

        AccessKey accessKey1 = new AccessKey();
        accessKey1.setId("AKIA1234");
        accessKey1.setStatus(AccessKey.AccessKeyStatus.ACTIVE);
        accessKey1.setSecretKey("aks03-12");
        accessKey1.setCreateDate("2015-12-19T07:20:29.000+0530");

        AccessKey accessKey2 = new AccessKey();
        accessKey2.setId("AKIA5678");
        accessKey2.setStatus(AccessKey.AccessKeyStatus.ACTIVE);
        accessKey2.setSecretKey("kas-1aks12");
        accessKey2.setCreateDate("2015-12-18T07:20:29.000+0530");

        accessKeys.add(accessKey1);
        accessKeys.add(accessKey2);

        final String expectedResponseBody =
            "<?xml version=\"1.0\" " +
            "encoding=\"UTF-8\" standalone=\"no\"?>" +
            "<ListAccessKeysResponse " +
            "xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">" +
            "<ListAccessKeysResult>" + "<UserName>s3user1</UserName>" +
            "<AccessKeyMetadata>" + "<member>" +
            "<UserName>s3user1</UserName>" +
            "<AccessKeyId>AKIA1234</AccessKeyId>" + "<Status>Active</Status>" +
            "<CreateDate>2015-12-19T07:20:29.000+0530</CreateDate>" +
            "</member>" + "<member>" + "<UserName>s3user1</UserName>" +
            "<AccessKeyId>AKIA5678</AccessKeyId>" + "<Status>Active</Status>" +
            "<CreateDate>2015-12-18T07:20:29.000+0530</CreateDate>" +
            "</member>" + "</AccessKeyMetadata>" +
            "<IsTruncated>false</IsTruncated>" + "</ListAccessKeysResult>" +
            "<ResponseMetadata>" + "<RequestId>0000</RequestId>" +
            "</ResponseMetadata>" + "</ListAccessKeysResponse>";

        AccessKey[] accessKeyList = new AccessKey[accessKeys.size()];
        AccessKeyResponseGenerator responseGenerator = new AccessKeyResponseGenerator();
        ServerResponse response = responseGenerator.generateListResponse(
                "s3user1", accessKeys.toArray(accessKeyList)
        );

        Assert.assertEquals(expectedResponseBody, response.getResponseBody());
        Assert.assertEquals(HttpResponseStatus.OK, response.getResponseStatus());
    }

    @Test
    public void testAccessKeyDeleteResponse() {
        final String expectedResponseBody = "<?xml version=\"1.0\" "
                + "encoding=\"UTF-8\" standalone=\"no\"?>"
                + "<DeleteAccessKeyResponse "
                + "xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">"
                + "<ResponseMetadata>"
                + "<RequestId>0000</RequestId>"
                + "</ResponseMetadata>"
                + "</DeleteAccessKeyResponse>";

        AccessKeyResponseGenerator responseGenerator = new AccessKeyResponseGenerator();
        ServerResponse response = responseGenerator.generateDeleteResponse();

        Assert.assertEquals(expectedResponseBody, response.getResponseBody());
        Assert.assertEquals(HttpResponseStatus.OK, response.getResponseStatus());
    }

    @Test
    public void testAccessKeyUpdateResponse() {
        final String expectedResponseBody = "<?xml version=\"1.0\" "
                + "encoding=\"UTF-8\" standalone=\"no\"?>"
                + "<UpdateAccessKeyResponse "
                + "xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">"
                + "<ResponseMetadata>"
                + "<RequestId>0000</RequestId>"
                + "</ResponseMetadata>"
                + "</UpdateAccessKeyResponse>";

        AccessKeyResponseGenerator responseGenerator = new AccessKeyResponseGenerator();
        ServerResponse response = responseGenerator.generateUpdateResponse();

        Assert.assertEquals(expectedResponseBody, response.getResponseBody());
        Assert.assertEquals(HttpResponseStatus.OK, response.getResponseStatus());
    }

    @Test
    public void testNoSuchEntityResponse() {
        final String expectedResponseBody = "<?xml version=\"1.0\" encoding=\"UTF-8\" " +
                "standalone=\"no\"?><ErrorResponse xmlns=\"https://iam.seagate.com/" +
                "doc/2010-05-08/\"><Error><Code>NoSuchEntity</Code><Message>The request" +
                " was rejected because it referenced an entity that does not exist. " +
                "</Message></Error><RequestId>0000</RequestId></ErrorResponse>";

        AccessKeyResponseGenerator responseGenerator = new AccessKeyResponseGenerator();
        ServerResponse response = responseGenerator.noSuchEntity();

        Assert.assertEquals(expectedResponseBody, response.getResponseBody());
        Assert.assertEquals(HttpResponseStatus.UNAUTHORIZED, response.getResponseStatus());
    }

    @Test
    public void testAccessKeyQuotaExceeded() {
        final String expectedResponseBody = "<?xml version=\"1.0\" "
                + "encoding=\"UTF-8\" standalone=\"no\"?>"
                + "<ErrorResponse xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">"
                + "<Error><Code>AccessKeyQuotaExceeded</Code>"
                + "<Message>The request was rejected because the number of "
                + "access keys allowed for the user has exceeded quota.</Message></Error>"
                + "<RequestId>0000</RequestId>"
                + "</ErrorResponse>";

        AccessKeyResponseGenerator responseGenerator = new AccessKeyResponseGenerator();
        ServerResponse response = responseGenerator.accessKeyQuotaExceeded();

        Assert.assertEquals(expectedResponseBody, response.getResponseBody());
        Assert.assertEquals(HttpResponseStatus.BAD_REQUEST, response.getResponseStatus());
    }
}
