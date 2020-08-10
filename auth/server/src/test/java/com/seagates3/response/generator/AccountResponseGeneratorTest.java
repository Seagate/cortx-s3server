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
import com.seagates3.model.Account;
import com.seagates3.model.User;
import com.seagates3.response.ServerResponse;
import io.netty.handler.codec.http.HttpResponseStatus;
import org.junit.Assert;
import org.junit.Test;

public class AccountResponseGeneratorTest {

    @Test
    public void testCreateResponse() {
        Account account = new Account();
        account.setName("s3test");
        account.setId("98765");
        account.setCanonicalId("C12345");

        User root = new User();
        root.setName("root");

        AccessKey accessKey = new AccessKey();
        accessKey.setId("1234");
        accessKey.setSecretKey("az-f/es");
        accessKey.setStatus(AccessKey.AccessKeyStatus.ACTIVE);

        final String expectedResponseBody = "<?xml version=\"1.0\" "
                + "encoding=\"UTF-8\" standalone=\"no\"?>"
                + "<CreateAccountResponse "
                + "xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">"
                + "<CreateAccountResult>"
                + "<Account>"
                + "<AccountId>98765</AccountId>"
                + "<CanonicalId>C12345</CanonicalId>"
                + "<AccountName>s3test</AccountName>"
                + "<RootUserName>root</RootUserName>"
                + "<AccessKeyId>1234</AccessKeyId>"
                + "<RootSecretKeyId>az-f/es</RootSecretKeyId>"
                + "<Status>Active</Status>"
                + "</Account>"
                + "</CreateAccountResult>"
                + "<ResponseMetadata>"
                + "<RequestId>0000</RequestId>"
                + "</ResponseMetadata>"
                + "</CreateAccountResponse>";

        AccountResponseGenerator responseGenerator = new AccountResponseGenerator();
        ServerResponse response = responseGenerator.generateCreateResponse(
                account, root, accessKey);

        Assert.assertEquals(expectedResponseBody, response.getResponseBody());
        Assert.assertEquals(HttpResponseStatus.CREATED, response.getResponseStatus());
    }

    @Test
    public void testResetAccountAccessKeyResponse() {
        Account account = new Account();
        account.setName("s3test");
        account.setId("98765");
        account.setCanonicalId("C12345");

        User root = new User();
        root.setName("root");

        AccessKey accessKey = new AccessKey();
        accessKey.setId("1234");
        accessKey.setSecretKey("az-f/es");
        accessKey.setStatus(AccessKey.AccessKeyStatus.ACTIVE);

        final String expectedResponseBody = "<?xml version=\"1.0\" "
                + "encoding=\"UTF-8\" standalone=\"no\"?>"
                + "<ResetAccountAccessKeyResponse "
                + "xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">"
                + "<ResetAccountAccessKeyResult>"
                + "<Account>"
                + "<AccountId>98765</AccountId>"
                + "<CanonicalId>C12345</CanonicalId>"
                + "<AccountName>s3test</AccountName>"
                + "<RootUserName>root</RootUserName>"
                + "<AccessKeyId>1234</AccessKeyId>"
                + "<RootSecretKeyId>az-f/es</RootSecretKeyId>"
                + "<Status>Active</Status>"
                + "</Account>"
                + "</ResetAccountAccessKeyResult>"
                + "<ResponseMetadata>"
                + "<RequestId>0000</RequestId>"
                + "</ResponseMetadata>"
                + "</ResetAccountAccessKeyResponse>";

        AccountResponseGenerator responseGenerator =
                                   new AccountResponseGenerator();
        ServerResponse response =
                responseGenerator.generateResetAccountAccessKeyResponse(account,
                                                         root, accessKey);

        Assert.assertEquals(expectedResponseBody,
                                     response.getResponseBody());
        Assert.assertEquals(HttpResponseStatus.CREATED,
                                   response.getResponseStatus());
    }

    @Test
    public void testEntityAlreadyExists() {
        final String expectedResponseBody = "<?xml version=\"1.0\" "
                + "encoding=\"UTF-8\" standalone=\"no\"?>"
                + "<ErrorResponse xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">"
                + "<Error><Code>EntityAlreadyExists</Code>"
                + "<Message>The request was rejected because it attempted to "
                + "create an account that already exists.</Message></Error>"
                + "<RequestId>0000</RequestId>"
                + "</ErrorResponse>";

        AccountResponseGenerator responseGenerator = new AccountResponseGenerator();
        ServerResponse response = responseGenerator.entityAlreadyExists();

        Assert.assertEquals(expectedResponseBody, response.getResponseBody());
        Assert.assertEquals(HttpResponseStatus.CONFLICT, response.getResponseStatus());
    }
}
