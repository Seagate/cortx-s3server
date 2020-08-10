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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.BeforeClass;
import org.junit.Test;

import com.seagates3.model.Account;
import com.seagates3.model.Requestor;
import com.seagates3.response.ServerResponse;

import io.netty.handler.codec.http.HttpResponseStatus;

public
class AuthorizationResponseGeneratorTest {

 private
  static Requestor requestor = new Requestor();
 private
  AuthorizationResponseGenerator responseGenerator =
      new AuthorizationResponseGenerator();

 private
  String serverResponseStringWithoutAcl =
      "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>" +
      "<AuthorizeUserResponse " +
      "xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">" +
      "<AuthorizeUserResult>" + "<UserId>456</UserId>" +
      "<UserName>tester</UserName>" + "<AccountId>12345</AccountId>" +
      "<AccountName>testAccount</AccountName>" +
      "<CanonicalId>qWwZGnGYTga8gbpcuY79SA</CanonicalId>" +
      "</AuthorizeUserResult>" + "<ResponseMetadata>" +
      "<RequestId>0000</RequestId>" + "</ResponseMetadata>" +
      "</AuthorizeUserResponse>";

 private
  static String defaultACL =
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n" +
      "<AccessControlPolicy " +
      "xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">\r\n" +
      " <Owner>\r\n" + "  <ID>qWwZGnGYTga8gbpcuY79SA</ID>\r\n" +
      "  <DisplayName>testAccount</DisplayName>\r\n" + " </Owner>\r\n" +
      " <AccessControlList>\r\n" + "  <Grant>\r\n" + "   <Grantee " +
      "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\r\n" +
      "      xsi:type=\"CanonicalUser\">\r\n" +
      "    <ID>qWwZGnGYTga8gbpcuY79SA</ID>\r\n" +
      "    <DisplayName>testAccount</DisplayName>\r\n" + "   </Grantee>\r\n" +
      "   <Permission>Permission</Permission>\r\n" + "  </Grant>\r\n" +
      " </AccessControlList>\r\n" + "</AccessControlPolicy>\r\n";

  @BeforeClass public static void setUpBeforeClass() {
    Account account = new Account();
    account.setCanonicalId("qWwZGnGYTga8gbpcuY79SA");
    account.setId("12345");
    account.setName("testAccount");

    requestor = new Requestor();
    requestor.setId("456");
    requestor.setName("tester");
    requestor.setAccount(account);
  }

  @Test public void testGenerateAuthorizationResponse() {

    ServerResponse actualServerResponse = null;
    actualServerResponse =
        responseGenerator.generateAuthorizationResponse(requestor, defaultACL);
    assertEquals(HttpResponseStatus.OK,
                 actualServerResponse.getResponseStatus());
    assertTrue(actualServerResponse.getResponseBody().matches(
        "[\\s\\S]*\\<ACL\\>[\\s\\S]*\\<\\/ACL\\>[\\s\\S]*"));
  }

  @Test public void testGenerateAuthorizationResponse_NullDefaultACL() {

    ServerResponse actualServerResponse = null;
    actualServerResponse =
        responseGenerator.generateAuthorizationResponse(requestor, null);
    assertEquals(HttpResponseStatus.OK,
                 actualServerResponse.getResponseStatus());
    assertEquals(serverResponseStringWithoutAcl,
                 actualServerResponse.getResponseBody());
    assertFalse(actualServerResponse.getResponseBody().matches(
        "[\\s\\S]*\\<ACL\\>[\\s\\S]*\\<\\/ACL\\>[\\s\\S]*"));
  }
}
