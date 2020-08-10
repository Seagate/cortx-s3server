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

import org.junit.Assert;
import org.junit.Test;

import com.seagates3.model.Account;
import com.seagates3.response.ServerResponse;

import io.netty.handler.codec.http.HttpResponseStatus;

public
class AccountLoginProfileResponseGeneratorTest {

  /**
   * Below method will test generateGetResponse method
   */
  @Test public void generateGetResponseTest() {
    Account account = new Account();
    account.setName("testUser");
    account.setProfileCreateDate("2019-06-16 15:38:53+00:00");
    account.setPwdResetRequired("false");
    final String expectedResponseBody =
        "<?xml version=\"1.0\" " + "encoding=\"UTF-8\" standalone=\"no\"?>" +
        "<GetAccountLoginProfileResponse " +
        "xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">" +
        "<GetAccountLoginProfileResult>" + "<LoginProfile>" +
        "<AccountName>testUser</AccountName>" +
        "<CreateDate>2019-06-16 15:38:53+00:00</CreateDate>" +
        "<PasswordResetRequired>false</PasswordResetRequired>" +
        "</LoginProfile>" + "</GetAccountLoginProfileResult>" +
        "<ResponseMetadata>" + "<RequestId>0000</RequestId>" +
        "</ResponseMetadata>" + "</GetAccountLoginProfileResponse>";

    AccountLoginProfileResponseGenerator responseGenerator =
        new AccountLoginProfileResponseGenerator();
    ServerResponse response = responseGenerator.generateGetResponse(account);

    Assert.assertEquals(expectedResponseBody, response.getResponseBody());
    Assert.assertEquals(HttpResponseStatus.OK, response.getResponseStatus());
  }
}
