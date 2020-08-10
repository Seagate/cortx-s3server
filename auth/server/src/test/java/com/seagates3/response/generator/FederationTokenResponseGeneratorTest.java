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
import com.seagates3.model.User;
import com.seagates3.response.ServerResponse;
import io.netty.handler.codec.http.HttpResponseStatus;
import org.junit.Assert;
import org.junit.Test;

public class FederationTokenResponseGeneratorTest {

    @Test
    public void testGenerateCreateResponse() {
        User user = new User();
        user.setPath("/");
        user.setName("s3user");
        user.setId("123");

        AccessKey accessKey = new AccessKey();
        accessKey.setId("AKIA1234");
        accessKey.setSecretKey("kas-1aks12");
        accessKey.setToken("/siauaAsatOTkes");
        accessKey.setExpiry("2015-12-19T07:20:29.000+0530");
        accessKey.setStatus(AccessKey.AccessKeyStatus.ACTIVE);

        final String expectedResponseBody = "<?xml version=\"1.0\" "
                + "encoding=\"UTF-8\" standalone=\"no\"?>"
                + "<GetFederationTokenResponse "
                + "xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">"
                + "<GetFederationTokenResult>"
                + "<Credentials>"
                + "<SessionToken>/siauaAsatOTkes</SessionToken>"
                + "<SecretAccessKey>kas-1aks12</SecretAccessKey>"
                + "<Expiration>2015-12-19T07:20:29.000+0530</Expiration>"
                + "<AccessKeyId>AKIA1234</AccessKeyId>"
                + "</Credentials>"
                + "<FederatedUser>"
                + "<Arn>arn:aws:sts::1:federated-user/s3user</Arn>"
                + "<FederatedUserId>123:s3user</FederatedUserId>"
                + "</FederatedUser>"
                + "<PackedPolicySize>6</PackedPolicySize>"
                + "</GetFederationTokenResult>"
                + "<ResponseMetadata>"
                + "<RequestId>0000</RequestId>"
                + "</ResponseMetadata>"
                + "</GetFederationTokenResponse>";

        FederationTokenResponseGenerator responseGenerator = new FederationTokenResponseGenerator();
        ServerResponse response = responseGenerator.generateCreateResponse(user, accessKey);

        Assert.assertEquals(expectedResponseBody, response.getResponseBody());
        Assert.assertEquals(HttpResponseStatus.CREATED, response.getResponseStatus());

    }
}
