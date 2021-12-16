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

package com.seagates3.authorization;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import java.util.List;
import java.util.Map;
import java.util.TreeMap;
import java.util.ArrayList;
import java.util.Collections;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mockito;
import org.powermock.api.mockito.PowerMockito;
import org.powermock.core.classloader.annotations.PowerMockIgnore;
import org.powermock.core.classloader.annotations.PrepareForTest;
import org.powermock.modules.junit4.PowerMockRunner;
import java.util.HashMap;
import com.seagates3.acl.ACLValidation;
import com.seagates3.authserver.AuthServerConfig;
import com.seagates3.dao.ldap.AccountImpl;
import com.seagates3.dao.ldap.LDAPUtils;
import com.seagates3.model.Account;
import com.seagates3.model.Requestor;
import com.seagates3.model.User;
import com.seagates3.policy.BucketPolicyAuthorizer;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.AuthorizationResponseGenerator;
import com.seagates3.util.BinaryUtil;
import com.seagates3.dao.ldap.PolicyImpl;
import com.seagates3.policy.IAMPolicyAuthorizer;

import io.netty.handler.codec.http.HttpResponseStatus;

@RunWith(PowerMockRunner.class)
    @PrepareForTest({LDAPUtils.class,       ACLValidation.class,
                     PolicyImpl.class,      IAMPolicyAuthorizer.class,
                     Authorizer.class,      BucketPolicyAuthorizer.class,
                     AuthServerConfig.class})
    @PowerMockIgnore({"javax.management.*"}) public class AuthorizerTest {

 private
  Map<String, String> requestBody = null;
 private
  static Requestor requestor = new Requestor();
 private
  String requestHeaderName = "Request-ACL";
 private
  AccountImpl mockAccountImpl;
 private
  Authorizer authorizer;
  AuthorizationResponseGenerator responseGenerator =
      new AuthorizationResponseGenerator();
 private
  Account mockAccount;
  static Account account = new Account();
  static Account secondaryAccount = new Account();
  User iamUser2;
 private
  static User user2;
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

  @BeforeClass public static void setUpBeforeClass() {
    Account account = new Account();
    account.setCanonicalId("qWwZGnGYTga8gbpcuY79SA");
    account.setId("12345");
    account.setName("testAccount");
    user2 = new User();
    user2.setId("456");
    user2.setName("root");
    user2.setPolicyIds(Collections.EMPTY_LIST);
    user2.setAccountName("testAccount");

    secondaryAccount.setCanonicalId("ACDDDFfeffTga8gbpcuY79SA");
    secondaryAccount.setId("123456");
    secondaryAccount.setName("testAccount2");

    requestor = new Requestor();
    requestor.setId("456");
    requestor.setName("tester");
    requestor.setAccount(account);
    requestor.setUser(user2);
    AuthServerConfig.authResourceDir = "../resources";
  }

  @Before public void setup() throws Exception {
    authorizer = new Authorizer();
    PowerMockito.mockStatic(LDAPUtils.class);
    PowerMockito.mockStatic(ACLValidation.class);
    mockAccountImpl = Mockito.mock(AccountImpl.class);
    mockAccount = Mockito.mock(Account.class);
    iamUser2 = new User();
    iamUser2.setName("user2");
    iamUser2.setAccountName("testAccount");
    iamUser2.setId("userid2");
    iamUser2.setPolicyIds(Collections.EMPTY_LIST);
    PolicyImpl mockPolicyImpl = Mockito.mock(PolicyImpl.class);
    PowerMockito.whenNew(PolicyImpl.class).withNoArguments().thenReturn(
        mockPolicyImpl);
    Mockito.when(mockPolicyImpl.findAll((Account)Mockito.any(),
                                        (HashMap)Mockito.any()))
        .thenReturn(Collections.EMPTY_LIST);
    PowerMockito.mockStatic(AuthServerConfig.class);
    PowerMockito.doReturn(true)
        .when(AuthServerConfig.class, "isEnableIamPolicy");
    PowerMockito.doReturn("0000").when(AuthServerConfig.class, "getReqId");
  }

  @Test public void validateServerResponseWithRequestHeaderAsTrue() {

    ServerResponse actualServerResponse = null;
    requestBody = new TreeMap<>();
    requestBody.put(requestHeaderName, "true");
    requestBody.put("S3Action", "dummyAction");
    actualServerResponse = authorizer.authorize(requestor, requestBody);
    assertEquals(HttpResponseStatus.OK,
                 actualServerResponse.getResponseStatus());
    assertTrue(actualServerResponse.getResponseBody().matches(
        "[\\s\\S]*\\<ACL\\>[\\s\\S]*\\<\\/ACL\\>[\\s\\S]*"));
  }

  @Test public void validateServerResponseWithRequestHeaderAsFalse() {

    ServerResponse actualServerResponse = null;
    requestBody = new TreeMap<>();
    requestBody.put(requestHeaderName, "false");
    requestBody.put("S3Action", "dummyAction");
    actualServerResponse = authorizer.authorize(requestor, requestBody);
    assertEquals(HttpResponseStatus.OK,
                 actualServerResponse.getResponseStatus());
    assertEquals(serverResponseStringWithoutAcl,
                 actualServerResponse.getResponseBody());
    assertFalse(actualServerResponse.getResponseBody().matches(
        "[\\s\\S]*\\<ACL\\>[\\s\\S]*\\<\\/ACL\\>[\\s\\S]*"));
  }

  @Test public void testAuthorize_defaultACPXmlNotNull() {

    ServerResponse actualServerResponse = null;
    requestBody = new TreeMap<>();
    requestBody.put(requestHeaderName, "true");
    requestBody.put("S3Action", "dummyAction");
    actualServerResponse = authorizer.authorize(requestor, requestBody);
    assertEquals(HttpResponseStatus.OK,
                 actualServerResponse.getResponseStatus());
    assertTrue(actualServerResponse.getResponseBody().matches(
        "[\\s\\S]*\\<ACL\\>[\\s\\S]*\\<\\/ACL\\>[\\s\\S]*"));
  }

  @Test public void authorize_validate_acl_internal_server_error() {
    ServerResponse actualServerResponse = null;
    String acl = "";
    requestBody = new TreeMap<>();
    requestBody.put("ACL", acl);
    actualServerResponse = authorizer.validateACL(requestBody);
    assertEquals(HttpResponseStatus.BAD_REQUEST,
                 actualServerResponse.getResponseStatus());
  }

  @Test public void authorize_validate_acl_invalid_permission() {
    ServerResponse actualServerResponse = null;
    String acl_invalid_permission =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
        "<AccessControlPolicy" +
        " xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">" + "<Owner><ID>" +
        "b103e16d027d24270d8facf37a48b141fd88ac8f43f9f942b91ba1cf1dc33f71" +
        "</ID>" +
        "<DisplayName>kirungeb</DisplayName></Owner><AccessControlList>" +
        "<Grant>" + "<Grantee " +
        "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" " +
        "xsi:type=\"CanonicalUser\"><ID>" +
        "b103e16d027d24270d8facf37a48b141fd88ac8f43f9f942b91ba1cf1dc33f71" +
        "</ID>" + "<DisplayName>kirungeb</DisplayName></Grantee>" +
        "<Permission>CONTROL</Permission></Grant></AccessControlList>" +
        "</AccessControlPolicy>";
    requestBody = new TreeMap<>();
    requestBody.put("ACL",
                    BinaryUtil.encodeToBase64String(acl_invalid_permission));
    actualServerResponse = authorizer.validateACL(requestBody);
    assertEquals(HttpResponseStatus.BAD_REQUEST,
                 actualServerResponse.getResponseStatus());
  }

  @Test public void authorize_validate_acl_malformed() {
    ServerResponse actualServerResponse = null;
    String acl_malformed =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
        "<AccessControlPolicy" +
        " xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">" + "<Owner><ID>" +
        "b103e16d027d24270d8facf37a48b141fd88ac8f43f9f942b91ba1cf1dc33f71" +
        "</ID>" +
        "<DisplayName>kirungeb</DisplayName></Owner><AccessControlList>" +
        "<Grant>" + "<Grantee " +
        "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" " +
        "xsi:type=\"CanonicalUser\"><ID>" +
        "b103e16d027d24270d8facf37a48b141fd88ac8f43f9f942b91ba1cf1dc33f71" +
        "</ID>" + "<DisplayName>kirungeb</DisplayName></Grantee>" +
        "<Permission>READ</Permission></Grant>" +
        "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" " +
        "xsi:type=\"CanonicalUser\"><ID>" +
        "24270d8facf37a48b141fd88ac8f43f9f942b91ba1cf1dc33f71b103e16d027d" +
        "</ID>" + "<DisplayName>myapp</DisplayName></Grantee>" +
        "<Permission>READ</Permission></Grant></AccessControlList>" +
        "</AccessControlPolicy>";
    requestBody = new TreeMap<>();
    requestBody.put("ACL", BinaryUtil.encodeToBase64String(acl_malformed));
    actualServerResponse = authorizer.validateACL(requestBody);
    assertEquals(HttpResponseStatus.BAD_REQUEST,
                 actualServerResponse.getResponseStatus());
  }

  @Test public void authorize_validate_acl_missing_owner() {
    ServerResponse actualServerResponse = null;
    String acl_missing_owner =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
        "<AccessControlPolicy" +
        " xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">" +
        "<AccessControlList>" + "<Grant>" + "<Grantee " +
        "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" " +
        "xsi:type=\"CanonicalUser\"><ID>" +
        "b103e16d027d24270d8facf37a48b141fd88ac8f43f9f942b91ba1cf1dc33f71" +
        "</ID>" + "<DisplayName>kirungeb</DisplayName></Grantee>" +
        "<Permission>FULL_CONTROL</Permission></Grant></AccessControlList>" +
        "</AccessControlPolicy>";
    requestBody = new TreeMap<>();
    requestBody.put("ACL", BinaryUtil.encodeToBase64String(acl_missing_owner));
    actualServerResponse = authorizer.validateACL(requestBody);
    assertEquals(HttpResponseStatus.BAD_REQUEST,
                 actualServerResponse.getResponseStatus());
  }

  @Test public void authorize_validate_acl_invalid_nograntee() {
    ServerResponse actualServerResponse = null;
    String acl_invalid_nograntee =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
        "<AccessControlPolicy" +
        " xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">" + "<Owner><ID>" +
        "b103e16d027d24270d8facf37a48b141fd88ac8f43f9f942b91ba1cf1dc33f71" +
        "</ID>" +
        "<DisplayName>kirungeb</DisplayName></Owner><AccessControlList>" +
        "<Grant>" +
        "<Permission>READ</Permission></Grant></AccessControlList>" +
        "</AccessControlPolicy>";
    requestBody = new TreeMap<>();
    requestBody.put("ACL",
                    BinaryUtil.encodeToBase64String(acl_invalid_nograntee));
    actualServerResponse = authorizer.validateACL(requestBody);
    assertEquals(HttpResponseStatus.BAD_REQUEST,
                 actualServerResponse.getResponseStatus());
  }

  @Test public void authorize_validate_acl_invalid_nopermission() {
    ServerResponse actualServerResponse = null;
    String acl_no_permission =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
        "<AccessControlPolicy" +
        " xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">" + "<Owner><ID>" +
        "b103e16d027d24270d8facf37a48b141fd88ac8f43f9f942b91ba1cf1dc33f71" +
        "</ID>" +
        "<DisplayName>kirungeb</DisplayName></Owner><AccessControlList>" +
        "<Grant>" + "<Grantee " +
        "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" " +
        "xsi:type=\"CanonicalUser\"><ID>" +
        "b103e16d027d24270d8facf37a48b141fd88ac8f43f9f942b91ba1cf1dc33f71" +
        "</ID>" + "<DisplayName>kirungeb</DisplayName></Grantee>" +
        "</Grant></AccessControlList>" + "</AccessControlPolicy>";
    requestBody = new TreeMap<>();
    requestBody.put("ACL", BinaryUtil.encodeToBase64String(acl_no_permission));
    actualServerResponse = authorizer.validateACL(requestBody);
    assertEquals(HttpResponseStatus.BAD_REQUEST,
                 actualServerResponse.getResponseStatus());
  }

  @Test public void authorize_validate_acl_valid_id() throws Exception {
    ServerResponse actualServerResponse = null;
    String acl =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
        "<AccessControlPolicy" +
        " xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">" + "<Owner><ID>" +
        "b103e16d027d24270d8facf37a48b141fd88ac8f43f9f942b91ba1cf1dc33f71" +
        "</ID>" +
        "<DisplayName>kirungeb</DisplayName></Owner><AccessControlList>" +
        "<Grant>" + "<Grantee " +
        "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" " +
        "xsi:type=\"CanonicalUser\"><ID>" +
        "b103e16d027d24270d8facf37a48b141fd88ac8f43f9f942b91ba1cf1dc33f71" +
        "</ID>" + "<DisplayName>kirungeb</DisplayName></Grantee>" +
        "<Permission>FULL_CONTROL</Permission></Grant></AccessControlList>" +
        "</AccessControlPolicy>";
    requestBody = new TreeMap<>();
    requestBody.put("ACL", acl);

    PowerMockito
        .when(
             ACLValidation.class, "checkIdExists",
             "b103e16d027d24270d8facf37a48b141fd88ac8f43f9f942b91ba1cf1dc33f71",
             "kirungeb")
        .thenReturn(true);

    actualServerResponse = authorizer.validateACL(requestBody);
    assertEquals(HttpResponseStatus.OK,
                 actualServerResponse.getResponseStatus());
  }

  @Test public void authorize_validate_acl_invalid_id() throws Exception {
    ServerResponse actualServerResponse = null;
    String acl =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
        "<AccessControlPolicy" +
        " xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">" + "<Owner><ID>" +
        "b103e16d027d24270d8facf37a48b141fd88ac8f43f9f942b91ba1cf1dc33f71" +
        "</ID>" +
        "<DisplayName>kirungeb</DisplayName></Owner><AccessControlList>" +
        "<Grant>" + "<Grantee " +
        "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" " +
        "xsi:type=\"CanonicalUser\"><ID>" +
        "b103e16d027d24270d8facf37a48b141fd88ac8f43f9f942b91ba1cf1dc33f71" +
        "</ID>" + "<DisplayName>kirungeb</DisplayName></Grantee>" +
        "<Permission>FULL_CONTROL</Permission></Grant></AccessControlList>" +
        "</AccessControlPolicy>";
    requestBody = new TreeMap<>();
    requestBody.put("ACL", acl);

    PowerMockito
        .when(
             ACLValidation.class, "checkIdExists",
             "b103e16d027d24270d8facf37a48b141fd88ac8f43f9f942b91ba1cf1dc33f71",
             "kirungeb")
        .thenReturn(false);

    actualServerResponse = authorizer.validateACL(requestBody);
    assertEquals(HttpResponseStatus.BAD_REQUEST,
                 actualServerResponse.getResponseStatus());
  }

  @Test public void authorize_acl_requestacl_check() throws Exception {
    ServerResponse actualServerResponse = null;
    requestBody = new TreeMap<>();
    requestBody.put("S3Action", "CreateBucket");
    requestBody.put("Request-ACL", "true");
    actualServerResponse = authorizer.authorize(requestor, requestBody);
    assertEquals(HttpResponseStatus.OK,
                 actualServerResponse.getResponseStatus());
  }

  /**
   * Below will test- PutBucketPolicy first time call
   */
  @Test public void authorize_policy_positive() {
    String acl =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
        "<AccessControlPolicy" +
        " xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">" + "<Owner><ID>" +
        "qWwZGnGYTga8gbpcuY79SA" + "</ID>" +
        "<DisplayName>kirungeb</DisplayName></Owner><AccessControlList>" +
        "<Grant>" + "<Grantee " +
        "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" " +
        "xsi:type=\"CanonicalUser\"><ID>" +
        "b103e16d027d24270d8facf37a48b141fd88ac8f43f9f942b91ba1cf1dc33f71" +
        "</ID>" + "<DisplayName>kirungeb</DisplayName></Grantee>" +
        "<Permission>FULL_CONTROL</Permission></Grant></AccessControlList>" +
        "</AccessControlPolicy>";
    requestBody = new TreeMap<>();
    requestBody.put("S3Action", "PutBucketPolicy");
    requestBody.put("Auth-ACL", BinaryUtil.encodeToBase64String(acl));
    ServerResponse actualServerResponse = null;
    requestBody.put("ClientQueryParams", "policy");
    requestBody.put("Method", "PUT");
    actualServerResponse = authorizer.authorize(requestor, requestBody);
    assertEquals(HttpResponseStatus.OK,
                 actualServerResponse.getResponseStatus());
  }

  /**
   * Below will test- PutBucketPolicy second time call with Allow effect
   *
   * @throws Exception
   */
  @Test public void authorize_policy_when_reuploaded_with_allow_permission()
      throws Exception {
    String policy =
        "{\r\n" + "  \"Id\": \"Policy1571741920713\",\r\n" +
        "  \"Version\": \"2012-10-17\",\r\n" + "  \"Statement\": [\r\n" +
        "    {\r\n" + "      \"Sid\": \"Stmt1571741573370\",\r\n" +
        "      \"Resource\": \"arn:aws:s3:::try1\",\r\n" +
        "          \"Action\": [\r\n" +
        "                  \"s3:PutBucketPolicy\"\r\n" + "      ],\r\n" +
        "      \"Effect\": \"Allow\",\r\n" + "          \"Principal\": {\r\n" +
        "        \"AWS\": [\r\n" + "          \"*\"\r\n" + "        ]\r\n" +
        "      }\r\n" + "    }\r\n" + "  ]\r\n" + "}";

    String acl =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
        "<AccessControlPolicy" +
        " xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">" + "<Owner><ID>" +
        "qWwZGnGYTga8gbpcuY79SA" + "</ID>" +
        "<DisplayName>kirungeb</DisplayName></Owner><AccessControlList>" +
        "<Grant>" + "<Grantee " +
        "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" " +
        "xsi:type=\"CanonicalUser\"><ID>" +
        "b103e16d027d24270d8facf37a48b141fd88ac8f43f9f942b91ba1cf1dc33f71" +
        "</ID>" + "<DisplayName>kirungeb</DisplayName></Grantee>" +
        "<Permission>FULL_CONTROL</Permission></Grant></AccessControlList>" +
        "</AccessControlPolicy>";
    requestBody = new TreeMap<>();
    requestBody.put("Auth-ACL", BinaryUtil.encodeToBase64String(acl));
    requestBody.put("Policy", policy);
    ServerResponse actualServerResponse = null;
    requestBody.put("ClientQueryParams", "policy");
    requestBody.put("Method", "PUT");
    requestBody.put("ClientAbsoluteUri", "/try1");
    requestBody.put("S3Action", "PutBucketPolicy");
    PowerMockito.whenNew(AccountImpl.class).withNoArguments().thenReturn(
        mockAccountImpl);
    Mockito.when(mockAccountImpl.findByCanonicalID("qWwZGnGYTga8gbpcuY79SA"))
        .thenReturn(mockAccount);
    Mockito.when(mockAccount.getName()).thenReturn("testAccount");
    actualServerResponse = authorizer.authorize(requestor, requestBody);
    assertEquals(HttpResponseStatus.OK,
                 actualServerResponse.getResponseStatus());
  }

  /**
   * Below will test owner access being DENY in policy
   *
   * @throws Exception
   */
  @Test public void
  authorize_policy_success_for_owner_with_DENY_permission_in_policy()
      throws Exception {

    String policy =
        "{\r\n" + "  \"Id\": \"Policy1571741920713\",\r\n" +
        "  \"Version\": \"2012-10-17\",\r\n" + "  \"Statement\": [\r\n" +
        "    {\r\n" + "      \"Sid\": \"Stmt1571741573370\",\r\n" +
        "      \"Resource\": \"arn:aws:s3:::try1\",\r\n" +
        "          \"Action\": [\r\n" +
        "                  \"s3:PutBucketPolicy\"\r\n" + "      ],\r\n" +
        "      \"Effect\": \"Deny\",\r\n" + "          \"Principal\": {\r\n" +
        "        \"AWS\": [\r\n" + "          \"12345\"\r\n" + "        ]\r\n" +
        "      }\r\n" + "    }\r\n" + "  ]\r\n" + "}";

    String acl =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
        "<AccessControlPolicy" +
        " xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">" + "<Owner><ID>" +
        "qWwZGnGYTga8gbpcuY79SA" + "</ID>" +
        "<DisplayName>kirungeb</DisplayName></Owner><AccessControlList>" +
        "<Grant>" + "<Grantee " +
        "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" " +
        "xsi:type=\"CanonicalUser\"><ID>" +
        "b103e16d027d24270d8facf37a48b141fd88ac8f43f9f942b91ba1cf1dc33f71" +
        "</ID>" + "<DisplayName>kirungeb</DisplayName></Grantee>" +
        "<Permission>FULL_CONTROL</Permission></Grant></AccessControlList>" +
        "</AccessControlPolicy>";
    requestBody = new TreeMap<>();
    requestBody.put("Auth-ACL", BinaryUtil.encodeToBase64String(acl));
    requestBody.put("Policy", policy);
    ServerResponse actualServerResponse = null;
    requestBody.put("ClientQueryParams", "policy");
    requestBody.put("Method", "PUT");
    requestBody.put("ClientAbsoluteUri", "/try1");
    requestBody.put("S3Action", "PutBucketPolicy");
    Mockito.when(mockAccount.getName()).thenReturn("testAccount");
    actualServerResponse = authorizer.authorize(requestor, requestBody);
    assertEquals(HttpResponseStatus.OK,
                 actualServerResponse.getResponseStatus());
  }

  /**
   * Authorize Delete bucket policy for owner w.o permission
   *
   * @throws Exception
   */
  @Test public void authorize_delete_bucket_policy_success_for_owner()
      throws Exception {

    String policy =
        "{\r\n" + "  \"Id\": \"Policy1571741920713\",\r\n" +
        "  \"Version\": \"2012-10-17\",\r\n" + "  \"Statement\": [\r\n" +
        "    {\r\n" + "      \"Sid\": \"Stmt1571741573370\",\r\n" +
        "      \"Resource\": \"arn:aws:s3:::try1\",\r\n" +
        "          \"Action\": [\r\n" +
        "                  \"s3:PutBucketPolicy\"\r\n" + "      ],\r\n" +
        "      \"Effect\": \"Deny\",\r\n" + "          \"Principal\": {\r\n" +
        "        \"AWS\": [\r\n" + "          \"12345\"\r\n" + "        ]\r\n" +
        "      }\r\n" + "    }\r\n" + "  ]\r\n" + "}";

    String acl =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
        "<AccessControlPolicy" +
        " xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">" + "<Owner><ID>" +
        "qWwZGnGYTga8gbpcuY79SA" + "</ID>" +
        "<DisplayName>kirungeb</DisplayName></Owner><AccessControlList>" +
        "<Grant>" + "<Grantee " +
        "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" " +
        "xsi:type=\"CanonicalUser\"><ID>" +
        "b103e16d027d24270d8facf37a48b141fd88ac8f43f9f942b91ba1cf1dc33f71" +
        "</ID>" + "<DisplayName>kirungeb</DisplayName></Grantee>" +
        "<Permission>FULL_CONTROL</Permission></Grant></AccessControlList>" +
        "</AccessControlPolicy>";
    requestBody = new TreeMap<>();
    requestBody.put("Auth-ACL", BinaryUtil.encodeToBase64String(acl));
    requestBody.put("Policy", policy);
    ServerResponse actualServerResponse = null;
    requestBody.put("ClientQueryParams", "policy");
    requestBody.put("Method", "DELETE");
    requestBody.put("ClientAbsoluteUri", "/try1");
    requestBody.put("S3Action", "DeleteBucketPolicy");
    Mockito.when(mockAccount.getName()).thenReturn("testAccount");
    actualServerResponse = authorizer.authorize(requestor, requestBody);
    assertEquals(HttpResponseStatus.OK,
                 actualServerResponse.getResponseStatus());
  }

  /**
   * MethodNotAllowed response for cross accounts
   *
   * @throws Exception
   */
  @Test public void authorize_put_bucket_policy_for_cross_account_scenario()
      throws Exception {

    String policy =
        "{\r\n" + "  \"Id\": \"Policy1571741920713\",\r\n" +
        "  \"Version\": \"2012-10-17\",\r\n" + "  \"Statement\": [\r\n" +
        "    {\r\n" + "      \"Sid\": \"Stmt1571741573370\",\r\n" +
        "      \"Resource\": \"arn:aws:s3:::try1\",\r\n" +
        "          \"Action\": [\r\n" +
        "                  \"s3:PutBucketPolicy\"\r\n" + "      ],\r\n" +
        "      \"Effect\": \"Allow\",\r\n" + "          \"Principal\": {\r\n" +
        "        \"AWS\": [\r\n" + "          \"12345\"\r\n" + "        ]\r\n" +
        "      }\r\n" + "    }\r\n" + "  ]\r\n" + "}";

    String acl =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
        "<AccessControlPolicy" +
        " xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">" + "<Owner><ID>" +
        "qWwZGnGYTga8gbpcuY79SA" + "</ID>" +
        "<DisplayName>kirungeb</DisplayName></Owner><AccessControlList>" +
        "<Grant>" + "<Grantee " +
        "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" " +
        "xsi:type=\"CanonicalUser\"><ID>" +
        "b103e16d027d24270d8facf37a48b141fd88ac8f43f9f942b91ba1cf1dc33f71" +
        "</ID>" + "<DisplayName>kirungeb</DisplayName></Grantee>" +
        "<Permission>FULL_CONTROL</Permission></Grant></AccessControlList>" +
        "</AccessControlPolicy>";
    requestBody = new TreeMap<>();
    requestBody.put("Auth-ACL", BinaryUtil.encodeToBase64String(acl));
    requestBody.put("Policy", policy);
    ServerResponse actualServerResponse = null;
    requestBody.put("ClientQueryParams", "policy");
    requestBody.put("Method", "PUT");
    requestBody.put("ClientAbsoluteUri", "/try1");
    requestBody.put("S3Action", "PutBucketPolicy");
    PowerMockito.whenNew(AccountImpl.class).withNoArguments().thenReturn(
        mockAccountImpl);
    Mockito.when(mockAccountImpl.findByCanonicalID("qWwZGnGYTga8gbpcuY79SA"))
        .thenReturn(mockAccount);
    Mockito.when(mockAccount.getName()).thenReturn("abcd");
    actualServerResponse = authorizer.authorize(requestor, requestBody);
    assertEquals(HttpResponseStatus.METHOD_NOT_ALLOWED,
                 actualServerResponse.getResponseStatus());
  }

  /**
   * Below will test- PutBucketPolicy with other IAM user restricted
   *
   * @throws Exception
   */
  @Test public void
  authorize_policy_one_IAM_user_allowed_otherIAM_user_restricted()
      throws Exception {
    String policy =
        "{\r\n" + "   \"Statement\": [\r\n" + "      {\r\n" +
        "         \"Effect\": \"Allow\",\r\n" +
        "         \"Principal\": {\r\n" + "        \"AWS\": [\r\n" +
        "          \"userid1\"\r\n" + "        ]\r\n" + "      },\r\n" +
        "         \"Action\": \"s3:PutBucketPolicy\",\r\n" +
        "         \"Resource\": \"arn:aws:s3:::try1\"\r\n" + "      },\r\n" +
        "      {\r\n" + "         \"Effect\": \"Deny\",\r\n" +
        "         \"Principal\": {\r\n" + "        \"AWS\": [\r\n" +
        "          \"userid2\"\r\n" + "        ]\r\n" + "      },\r\n" +
        "         \"Action\": \"s3:PutBucketPolicy\",\r\n" +
        "         \"Resource\": \"arn:aws:s3:::try1\"\r\n" + "      }\r\n" +
        "   ]\r\n" + "}";

    String acl =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
        "<AccessControlPolicy" +
        " xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">" + "<Owner><ID>" +
        "qWwZGnGYTga8gbpcuY79SA" + "</ID>" +
        "<DisplayName>kirungeb</DisplayName></Owner><AccessControlList>" +
        "<Grant>" + "<Grantee " +
        "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" " +
        "xsi:type=\"CanonicalUser\"><ID>" + "ACDDDFfeffTga8gbpcuY79SA" +
        "</ID>" + "<DisplayName>kirungeb</DisplayName></Grantee>" +
        "<Permission>FULL_CONTROL</Permission></Grant></AccessControlList>" +
        "</AccessControlPolicy>";

    requestBody = new TreeMap<>();
    requestBody.put("Auth-ACL", BinaryUtil.encodeToBase64String(acl));
    requestBody.put("Policy", policy);
    ServerResponse actualServerResponse = null;
    requestBody.put("ClientQueryParams", "policy");
    requestBody.put("Method", "PUT");
    requestBody.put("ClientAbsoluteUri", "/try1");
    requestBody.put("S3Action", "PutBucketPolicy");
    requestor.setUser(iamUser2);
    requestor.getAccount().setId("userid2");
    PowerMockito.whenNew(AccountImpl.class).withNoArguments().thenReturn(
        mockAccountImpl);
    Mockito.when(mockAccountImpl.findByCanonicalID("qWwZGnGYTga8gbpcuY79SA"))
        .thenReturn(mockAccount);
    Mockito.when(mockAccount.getName()).thenReturn("testAccount");
    actualServerResponse = authorizer.authorize(requestor, requestBody);
    assertEquals(HttpResponseStatus.FORBIDDEN,
                 actualServerResponse.getResponseStatus());

    requestor.getAccount().setId("12345");
    requestor.setUser(user2);
  }

  /**
   * Below will test- GetObjectAcl operation DENY in policy but ALLOW in ACL
   *
   * @throws Exception
   */
  @Test public void getObjectAcl_deny_inPolicy_allow_inAcl() throws Exception {
    Requestor requestor2 = new Requestor();
    requestor2.setId("456");
    requestor2.setName("tester2");
    requestor2.setAccount(secondaryAccount);
    User user3 = new User();
    user3.setId("456");
    user3.setName("root");
    user3.setPolicyIds(Collections.EMPTY_LIST);
    user3.setAccountName("testAccount2");
    requestor2.setUser(user3);
    String policy = "{\r\n" + "  \"Id\": \"Policy1571741920713\",\r\n" +
                    "  \"Version\": \"2012-10-17\",\r\n" + "\r\n" +
                    "  \"Statement\": [\r\n" + "        {\r\n" +
                    "      \"Sid\": \"Stmt1571741573370\",\r\n" +
                    "      \"Resource\": \"arn:aws:s3:::try1\",\r\n" +
                    "      \"Action\": \"s3:GetObjectAcl\",\r\n" +
                    "      \"Effect\": \"Deny\",\r\n" +
                    "     \"Principal\": {\r\n" + "        \"AWS\": [\r\n" +
                    "          \"123456\"\r\n" + "        ]\r\n" +
                    "      }\r\n" + "    }\r\n" + "    ]\r\n" + "}\r\n" +
                    "\r\n" + "\r\n" + "";

    String acl =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
        "<AccessControlPolicy" +
        " xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">" + "<Owner><ID>" +
        "qWwZGnGYTga8gbpcuY79SA" + "</ID>" +
        "<DisplayName>kirungeb</DisplayName></Owner><AccessControlList>" +
        "<Grant>" + "<Grantee " +
        "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" " +
        "xsi:type=\"CanonicalUser\"><ID>" + "ACDDDFfeffTga8gbpcuY79SA" +
        "</ID>" + "<DisplayName>kirungeb</DisplayName></Grantee>" +
        "<Permission>FULL_CONTROL</Permission></Grant></AccessControlList>" +
        "</AccessControlPolicy>";

    requestBody = new TreeMap<>();
    requestBody.put("Auth-ACL", BinaryUtil.encodeToBase64String(acl));
    requestBody.put("Policy", policy);
    ServerResponse actualServerResponse = null;
    requestBody.put("ClientQueryParams", "acl");
    requestBody.put("Method", "GET");
    requestBody.put("ClientAbsoluteUri", "/try1/a.txt");
    requestBody.put("S3Action", "GetObjectAcl");
    PowerMockito.whenNew(AccountImpl.class).withNoArguments().thenReturn(
        mockAccountImpl);
    Mockito.when(mockAccountImpl.findByCanonicalID("qWwZGnGYTga8gbpcuY79SA"))
        .thenReturn(mockAccount);
    Mockito.when(mockAccount.getName()).thenReturn("testAccount");
    actualServerResponse = authorizer.authorize(requestor2, requestBody);
    assertEquals(HttpResponseStatus.OK,
                 actualServerResponse.getResponseStatus());
  }

  /**
   * Below will test- PutObject operation DENY in policy but ALLOW in ACL
   *
   * @throws Exception
   */
  @Test public void putObject_allow_inPolicy_deny_inAcl() throws Exception {
    Requestor requestor2 = new Requestor();
    requestor2.setId("456");
    requestor2.setName("tester2");
    requestor2.setAccount(secondaryAccount);
    User user3 = new User();
    user3.setId("456");
    user3.setName("root");
    user3.setPolicyIds(Collections.EMPTY_LIST);
    user3.setAccountName("testAccount2");
    requestor2.setUser(user3);
    String policy = "{\r\n" + "  \"Id\": \"Policy1571741920713\",\r\n" +
                    "  \"Version\": \"2012-10-17\",\r\n" + "\r\n" +
                    "  \"Statement\": [\r\n" + "        {\r\n" +
                    "      \"Sid\": \"Stmt1571741573370\",\r\n" +
                    "      \"Resource\": \"arn:aws:s3:::try1/a.txt\",\r\n" +
                    "      \"Action\": \"s3:PutObject\",\r\n" +
                    "      \"Effect\": \"Allow\",\r\n" +
                    "     \"Principal\": {\r\n" + "        \"AWS\": [\r\n" +
                    "          \"123456\"\r\n" + "        ]\r\n" +
                    "      }\r\n" + "    }\r\n" + "    ]\r\n" + "}\r\n" +
                    "\r\n" + "\r\n" + "";

    String acl =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
        "<AccessControlPolicy" +
        " xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">" + "<Owner><ID>" +
        "qWwZGnGYTga8gbpcuY79SA" + "</ID>" +
        "<DisplayName>kirungeb</DisplayName></Owner><AccessControlList>" +
        "<Grant>" + "<Grantee " +
        "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" " +
        "xsi:type=\"CanonicalUser\"><ID>" + "ACDDDFfeffTga8gbpcuY79SA" +
        "</ID>" + "<DisplayName>kirungeb</DisplayName></Grantee>" +
        "<Permission>READ</Permission></Grant></AccessControlList>" +
        "</AccessControlPolicy>";

    requestBody = new TreeMap<>();
    requestBody.put("Auth-ACL", BinaryUtil.encodeToBase64String(acl));
    requestBody.put("Policy", policy);
    ServerResponse actualServerResponse = null;
    requestBody.put("ClientQueryParams", "");
    requestBody.put("Method", "PUT");
    requestBody.put("ClientAbsoluteUri", "/try1/a.txt");
    requestBody.put("S3Action", "PutObject");
    PowerMockito.whenNew(AccountImpl.class).withNoArguments().thenReturn(
        mockAccountImpl);
    Mockito.when(mockAccountImpl.findByCanonicalID("qWwZGnGYTga8gbpcuY79SA"))
        .thenReturn(mockAccount);
    Mockito.when(mockAccount.getName()).thenReturn("testAccount");
    actualServerResponse = authorizer.authorize(requestor2, requestBody);
    assertEquals(HttpResponseStatus.OK,
                 actualServerResponse.getResponseStatus());
  }

  /**
   * Below will test- GetObjectAcl operation not present in policy and not
   *present
   * in ACL
   *
   * @throws Exception
   */
  @Test public void getObjectAcl_notpresent_inPolicy_notpresent_inAcl()
      throws Exception {
    Requestor requestor2 = new Requestor();
    requestor2.setId("456");
    requestor2.setName("tester2");
    requestor2.setAccount(secondaryAccount);
    User user3 = new User();
    user3.setId("456");
    user3.setName("root");
    user3.setPolicyIds(Collections.EMPTY_LIST);
    user3.setAccountName("testAccount2");
    requestor2.setUser(user3);
    String policy = "{\r\n" + "  \"Id\": \"Policy1571741920713\",\r\n" +
                    "  \"Version\": \"2012-10-17\",\r\n" + "\r\n" +
                    "  \"Statement\": [\r\n" + "        {\r\n" +
                    "      \"Sid\": \"Stmt1571741573370\",\r\n" +
                    "      \"Resource\": \"arn:aws:s3:::try1/a.txt\",\r\n" +
                    "      \"Action\": \"s3:PutObject\",\r\n" +
                    "      \"Effect\": \"Allow\",\r\n" +
                    "     \"Principal\": {\r\n" + "        \"AWS\": [\r\n" +
                    "          \"123456\"\r\n" + "        ]\r\n" +
                    "      }\r\n" + "    }\r\n" + "    ]\r\n" + "}\r\n" +
                    "\r\n" + "\r\n" + "";

    String acl =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
        "<AccessControlPolicy" +
        " xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">" + "<Owner><ID>" +
        "qWwZGnGYTga8gbpcuY79SA" + "</ID>" +
        "<DisplayName>kirungeb</DisplayName></Owner><AccessControlList>" +
        "<Grant>" + "<Grantee " +
        "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" " +
        "xsi:type=\"CanonicalUser\"><ID>" + "ACDDDFfeffTga8gbpcuY79SA" +
        "</ID>" + "<DisplayName>kirungeb</DisplayName></Grantee>" +
        "<Permission>READ</Permission></Grant></AccessControlList>" +
        "</AccessControlPolicy>";

    requestBody = new TreeMap<>();
    requestBody.put("Auth-ACL", BinaryUtil.encodeToBase64String(acl));
    requestBody.put("Policy", policy);
    ServerResponse actualServerResponse = null;
    requestBody.put("ClientQueryParams", "acl");
    requestBody.put("Method", "GET");
    requestBody.put("ClientAbsoluteUri", "/try1/a.txt");
    requestBody.put("S3Action", "getObjectacl");
    PowerMockito.whenNew(AccountImpl.class).withNoArguments().thenReturn(
        mockAccountImpl);
    Mockito.when(mockAccountImpl.findByCanonicalID("qWwZGnGYTga8gbpcuY79SA"))
        .thenReturn(mockAccount);
    Mockito.when(mockAccount.getName()).thenReturn("testAccount");
    actualServerResponse = authorizer.authorize(requestor2, requestBody);
    assertEquals(HttpResponseStatus.FORBIDDEN,
                 actualServerResponse.getResponseStatus());
  }

  /**
   * Below will test- PutObjectAcl operation with Conflicting permissions in
   * policy and Allow in ACL
   *
   * @throws Exception
   */
  @Test public void putObjectAcl_conflicting_inPolicy_present_inAcl()
      throws Exception {
    Requestor requestor2 = new Requestor();
    requestor2.setId("456");
    requestor2.setName("tester2");
    requestor2.setAccount(secondaryAccount);
    User user3 = new User();
    user3.setId("456");
    user3.setName("root");
    user3.setPolicyIds(Collections.EMPTY_LIST);
    user3.setAccountName("testAccount2");
    requestor2.setUser(user3);
    String policy =
        "{\r\n" + "  \"Id\": \"Policy1571741920713\",\r\n" +
        "  \"Version\": \"2012-10-17\",\r\n" + "\r\n" +
        "  \"Statement\": [\r\n" + "        {\r\n" +
        "      \"Sid\": \"Stmt1571741573370\",\r\n" +
        "      \"Resource\": \"arn:aws:s3:::try1/a.txt\",\r\n" +
        "      \"Action\": \"s3:putobjectacl\",\r\n" +
        "      \"Effect\": \"Allow\",\r\n" + "     \"Principal\": {\r\n" +
        "        \"AWS\": [\r\n" + "          \"123456\"\r\n" +
        "        ]\r\n" + "      }\r\n" + "    },\r\n" + "    {\r\n" +
        "      \"Sid\": \"Stmt1571741573371\",\r\n" +
        "      \"Resource\": \"arn:aws:s3:::try1/a.txt\",\r\n" +
        "      \"Action\": \"s3:putobjectacl\",\r\n" +
        "      \"Effect\": \"Deny\",\r\n" + "     \"Principal\": {\r\n" +
        "        \"AWS\": [\r\n" + "          \"123456\"\r\n" +
        "        ]\r\n" + "      }\r\n" + "    }\r\n" + "    ]\r\n" + "}\r\n" +
        "\r\n" + "\r\n" + "";
    String acl =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
        "<AccessControlPolicy" +
        " xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">" + "<Owner><ID>" +
        "qWwZGnGYTga8gbpcuY79SA" + "</ID>" +
        "<DisplayName>kirungeb</DisplayName></Owner><AccessControlList>" +
        "<Grant>" + "<Grantee " +
        "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" " +
        "xsi:type=\"CanonicalUser\"><ID>" + "ACDDDFfeffTga8gbpcuY79SA" +
        "</ID>" + "<DisplayName>kirungeb</DisplayName></Grantee>" +
        "<Permission>WRITE_ACP</Permission></Grant></AccessControlList>" +
        "</AccessControlPolicy>";

    requestBody = new TreeMap<>();
    requestBody.put("Auth-ACL", BinaryUtil.encodeToBase64String(acl));
    requestBody.put("Policy", policy);
    ServerResponse actualServerResponse = null;
    requestBody.put("ClientQueryParams", "acl");
    requestBody.put("Method", "PUT");
    requestBody.put("ClientAbsoluteUri", "/try1/a.txt");
    requestBody.put("S3Action", "PutObjectAcl");
    PowerMockito.whenNew(AccountImpl.class).withNoArguments().thenReturn(
        mockAccountImpl);
    Mockito.when(mockAccountImpl.findByCanonicalID("qWwZGnGYTga8gbpcuY79SA"))
        .thenReturn(mockAccount);
    Mockito.when(mockAccount.getName()).thenReturn("testAccount");
    actualServerResponse = authorizer.authorize(requestor2, requestBody);
    assertEquals(HttpResponseStatus.FORBIDDEN,
                 actualServerResponse.getResponseStatus());
  }

  /**
       * Below will test- GetObjectAcl operation when no permission in policy
    *but
       * allow in acl
       *
       * @throws Exception
  */
  @Test public void putObjectAcl_nopermission_inPolicy_present_inAcl()
      throws Exception {
    Requestor requestor2 = new Requestor();
    requestor2.setId("456");
    requestor2.setName("tester2");
    requestor2.setAccount(secondaryAccount);
    User user3 = new User();
    user3.setId("456");
    user3.setName("root");
    user3.setPolicyIds(Collections.EMPTY_LIST);
    user3.setAccountName("testAccount2");
    requestor2.setUser(user3);
    String policy = "{\r\n" + "  \"Id\": \"Policy1571741920713\",\r\n" +
                    "  \"Version\": \"2012-10-17\",\r\n" + "\r\n" +
                    "  \"Statement\": [\r\n" + "        {\r\n" +
                    "      \"Sid\": \"Stmt1571741573370\",\r\n" +
                    "      \"Resource\": \"arn:aws:s3:::try1/a.txt\",\r\n" +
                    "      \"Action\": \"s3:PutObject\",\r\n" +
                    "      \"Effect\": \"Allow\",\r\n" +
                    "     \"Principal\": {\r\n" + "        \"AWS\": [\r\n" +
                    "          \"123456\"\r\n" + "        ]\r\n" +
                    "      }\r\n" + "    }\r\n" + "    ]\r\n" + "}\r\n" +
                    "\r\n" + "\r\n" + "";

    String acl =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
        "<AccessControlPolicy" +
        " xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">" + "<Owner><ID>" +
        "qWwZGnGYTga8gbpcuY79SA" + "</ID>" +
        "<DisplayName>kirungeb</DisplayName></Owner><AccessControlList>" +
        "<Grant>" + "<Grantee " +
        "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" " +
        "xsi:type=\"CanonicalUser\"><ID>" + "ACDDDFfeffTga8gbpcuY79SA" +
        "</ID>" + "<DisplayName>kirungeb</DisplayName></Grantee>" +
        "<Permission>READ_ACP</Permission></Grant></AccessControlList>" +
        "</AccessControlPolicy>";

    requestBody = new TreeMap<>();
    requestBody.put("Auth-ACL", BinaryUtil.encodeToBase64String(acl));
    requestBody.put("Policy", policy);
    ServerResponse actualServerResponse = null;
    requestBody.put("ClientQueryParams", "acl");
    requestBody.put("Method", "GET");
    requestBody.put("ClientAbsoluteUri", "/try1/a.txt");
    requestBody.put("S3Action", "GetObjectAcl");
    PowerMockito.whenNew(AccountImpl.class).withNoArguments().thenReturn(
        mockAccountImpl);
    Mockito.when(mockAccountImpl.findByCanonicalID("qWwZGnGYTga8gbpcuY79SA"))
        .thenReturn(mockAccount);
    Mockito.when(mockAccount.getName()).thenReturn("testAccount");
    actualServerResponse = authorizer.authorize(requestor2, requestBody);
    assertEquals(HttpResponseStatus.OK,
                 actualServerResponse.getResponseStatus());
  }

  /**
   *    * Below will test- PutObject operation Allow with canonical id in policy
   * but Deny in ACL
   *       *
   *          * @throws Exception
   *             */
  @Test public void putObject_allow_inPolicy_with_canonicalid_deny_inAcl()
      throws Exception {
    Requestor requestor2 = new Requestor();
    requestor2.setId("456");
    requestor2.setName("tester2");
    requestor2.setAccount(secondaryAccount);
    User user3 = new User();
    user3.setId("456");
    user3.setName("root");
    user3.setPolicyIds(Collections.EMPTY_LIST);
    user3.setAccountName("testAccount2");
    requestor2.setUser(user3);
    String policy = "{\r\n" + "  \"Id\": \"Policy1571741920713\",\r\n" +
                    "  \"Version\": \"2012-10-17\",\r\n" + "\r\n" +
                    "  \"Statement\": [\r\n" + "        {\r\n" +
                    "      \"Sid\": \"Stmt1571741573370\",\r\n" +
                    "      \"Resource\": \"arn:aws:s3:::try1/a.txt\",\r\n" +
                    "      \"Action\": \"s3:PutObject\",\r\n" +
                    "      \"Effect\": \"Allow\",\r\n" +
                    "     \"Principal\": {\r\n" +
                    "        \"CanonicalUser\": [\r\n" +
                    "          \"ACDDDFfeffTga8gbpcuY79SA\"\r\n" +
                    "        ]\r\n" + "      }\r\n" + "    }\r\n" +
                    "    ]\r\n" + "}\r\n" + "\r\n" + "\r\n" + "";

    String acl =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
        "<AccessControlPolicy" +
        " xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">" + "<Owner><ID>" +
        "qWwZGnGYTga8gbpcuY79SA" + "</ID>" +
        "<DisplayName>kirungeb</DisplayName></Owner><AccessControlList>" +
        "<Grant>" + "<Grantee " +
        "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" " +
        "xsi:type=\"CanonicalUser\"><ID>" + "ACDDDFfeffTga8gbpcuY79SA" +
        "</ID>" + "<DisplayName>kirungeb</DisplayName></Grantee>" +
        "<Permission>READ</Permission></Grant></AccessControlList>" +
        "</AccessControlPolicy>";

    requestBody = new TreeMap<>();
    requestBody.put("Auth-ACL", BinaryUtil.encodeToBase64String(acl));
    requestBody.put("Policy", policy);
    ServerResponse actualServerResponse = null;
    requestBody.put("ClientQueryParams", "");
    requestBody.put("Method", "PUT");
    requestBody.put("ClientAbsoluteUri", "/try1/a.txt");
    requestBody.put("S3Action", "PutObject");
    PowerMockito.whenNew(AccountImpl.class).withNoArguments().thenReturn(
        mockAccountImpl);
    Mockito.when(mockAccountImpl.findByCanonicalID("qWwZGnGYTga8gbpcuY79SA"))
        .thenReturn(mockAccount);
    Mockito.when(mockAccount.getName()).thenReturn("testAccount");
    actualServerResponse = authorizer.authorize(requestor2, requestBody);
    assertEquals(HttpResponseStatus.OK,
                 actualServerResponse.getResponseStatus());
  }

  /**
 *    * Below will test- PutObject operation Deny with canonical id in policy
 * but Allow in ACL
 *       *
 *          * @throws Exception
 *             */
  @Test public void putObject_deny_inPolicy_with_canonicalid_allow_inAcl()
      throws Exception {
    Requestor requestor2 = new Requestor();
    requestor2.setId("456");
    requestor2.setName("tester2");
    requestor2.setAccount(secondaryAccount);
    User user3 = new User();
    user3.setId("456");
    user3.setName("root");
    user3.setPolicyIds(Collections.EMPTY_LIST);
    user3.setAccountName("testAccount2");
    requestor2.setUser(user3);
    String policy = "{\r\n" + "  \"Id\": \"Policy1571741920713\",\r\n" +
                    "  \"Version\": \"2012-10-17\",\r\n" + "\r\n" +
                    "  \"Statement\": [\r\n" + "        {\r\n" +
                    "      \"Sid\": \"Stmt1571741573370\",\r\n" +
                    "      \"Resource\": \"arn:aws:s3:::try1/a.txt\",\r\n" +
                    "      \"Action\": \"s3:PutObject\",\r\n" +
                    "      \"Effect\": \"Deny\",\r\n" +
                    "     \"Principal\": {\r\n" +
                    "        \"CanonicalUser\": [\r\n" +
                    "          \"ACDDDFfeffTga8gbpcuY79SA\"\r\n" +
                    "        ]\r\n" + "      }\r\n" + "    }\r\n" +
                    "    ]\r\n" + "}\r\n" + "\r\n" + "\r\n" + "";

    String acl =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
        "<AccessControlPolicy" +
        " xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">" + "<Owner><ID>" +
        "qWwZGnGYTga8gbpcuY79SA" + "</ID>" +
        "<DisplayName>kirungeb</DisplayName></Owner><AccessControlList>" +
        "<Grant>" + "<Grantee " +
        "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" " +
        "xsi:type=\"CanonicalUser\"><ID>" + "ACDDDFfeffTga8gbpcuY79SA" +
        "</ID>" + "<DisplayName>kirungeb</DisplayName></Grantee>" +
        "<Permission>FULL_CONTROL</Permission></Grant></AccessControlList>" +
        "</AccessControlPolicy>";

    requestBody = new TreeMap<>();
    requestBody.put("Auth-ACL", BinaryUtil.encodeToBase64String(acl));
    requestBody.put("Policy", policy);
    ServerResponse actualServerResponse = null;
    requestBody.put("ClientQueryParams", "");
    requestBody.put("Method", "PUT");
    requestBody.put("ClientAbsoluteUri", "/try1/a.txt");
    requestBody.put("S3Action", "PutObject");
    PowerMockito.whenNew(AccountImpl.class).withNoArguments().thenReturn(
        mockAccountImpl);
    Mockito.when(mockAccountImpl.findByCanonicalID("qWwZGnGYTga8gbpcuY79SA"))
        .thenReturn(mockAccount);
    Mockito.when(mockAccount.getName()).thenReturn("testAccount");
    actualServerResponse = authorizer.authorize(requestor2, requestBody);
    assertEquals(HttpResponseStatus.FORBIDDEN,
                 actualServerResponse.getResponseStatus());
  }

  /**
     * Below will test iam policy authorization for PutObject on bucket
     */
  @Test public void testIamPolicyAuthorizationSuccess() {
    String iamPolicy = "{\r\n" + "  \"Version\": \"2012-10-17\",\r\n" +
                       "  \"Statement\": [\r\n" + "    {\r\n" +
                       "      \"Sid\": \"Stmt1638781464110\",\r\n" +
                       "      \"Action\": [\r\n" +
                       "        \"s3:PutObject\"\r\n" + "      ],\r\n" +
                       "      \"Effect\": \"Allow\",\r\n" +
                       "      \"Resource\": \"arn:aws:s3:::testbucket/*\"\r\n" +
                       "    }\r\n" + "  ]\r\n" + "}";

    requestBody = new TreeMap<>();
    String acl =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
        "<AccessControlPolicy" +
        " xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">" + "<Owner><ID>" +
        "qWwZGnGYTga8gbpcuY79SA" + "</ID>" +
        "<DisplayName>kirungeb</DisplayName></Owner><AccessControlList>" +
        "<Grant>" + "<Grantee " +
        "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" " +
        "xsi:type=\"CanonicalUser\"><ID>" + "qWwZGnGYTga8gbpcuY79SA" + "</ID>" +
        "<DisplayName>kirungeb</DisplayName></Grantee>" +
        "<Permission>FULL_CONTROL</Permission></Grant></AccessControlList>" +
        "</AccessControlPolicy>";
    requestBody.put("Auth-ACL", BinaryUtil.encodeToBase64String(acl));
    ServerResponse actualServerResponse = null;
    requestBody.put("ClientQueryParams", "");
    requestBody.put("Method", "PUT");
    requestBody.put("ClientAbsoluteUri", "/testbucket/a.txt");
    requestBody.put("S3Action", "PutObject");
    List<String> policys = new ArrayList<>();
    policys.add(iamPolicy);
    requestor.setPolicyDocuments(policys);
    actualServerResponse = authorizer.authorize(requestor, requestBody);
    assertEquals(HttpResponseStatus.OK,
                 actualServerResponse.getResponseStatus());
  }

  /**
   * Below will test successful authorization
   * if no match in IAM policy
   * But requesting user and input action's user is same
   */
  @Test public void testIamPolicyNotMatchingButAllowed() {
    String iamPolicy = "{\r\n" + "  \"Version\": \"2012-10-17\",\r\n" +
                       "  \"Statement\": [\r\n" + "    {\r\n" +
                       "      \"Sid\": \"Stmt1638781464110\",\r\n" +
                       "      \"Action\": [\r\n" +
                       "        \"s3:PutObject\"\r\n" + "      ],\r\n" +
                       "      \"Effect\": \"Allow\",\r\n" +
                       "      \"Resource\": \"arn:aws:s3:::testbucket/*\"\r\n" +
                       "    }\r\n" + "  ]\r\n" + "}";

    requestBody = new TreeMap<>();
    ServerResponse actualServerResponse = null;
    requestBody.put("Method", "POST");
    requestBody.put("Action", "ListAccessKeys");
    requestBody.put("UserName", "tester");
    List<String> policys = new ArrayList<>();
    policys.add(iamPolicy);
    requestor.setPolicyDocuments(policys);
    actualServerResponse = authorizer.authorize(requestor, requestBody);
    assertEquals(HttpResponseStatus.OK,
                 actualServerResponse.getResponseStatus());
  }

  @Test public void testIamPolicyForAccessDenied() {
    String iamPolicy = "{\r\n" + "  \"Version\": \"2012-10-17\",\r\n" +
                       "  \"Statement\": [\r\n" + "    {\r\n" +
                       "      \"Sid\": \"Stmt1638781464110\",\r\n" +
                       "      \"Action\": [\r\n" +
                       "        \"iam:GetPolicy\"\r\n" + "      ],\r\n" +
                       "      \"Effect\": \"Deny\",\r\n" +
                       "      \"Resource\": " +
                       "\"arn:aws:iam::690398721731:policy/policy32\"\r\n" +
                       "    }\r\n" + "  ]\r\n" + "}";

    requestBody = new TreeMap<>();
    ServerResponse actualServerResponse = null;
    requestBody.put("Method", "GET");
    requestBody.put("Action", "GetPolicy");
    requestBody.put("PolicyARN", "arn:aws:iam::690398721731:policy/policy32");
    List<String> policys = new ArrayList<>();
    policys.add(iamPolicy);
    requestor.setPolicyDocuments(policys);
    actualServerResponse = authorizer.authorize(requestor, requestBody);
    assertEquals(HttpResponseStatus.FORBIDDEN,
                 actualServerResponse.getResponseStatus());
  }

  @Test public void testIamPolicyAllowBucketPolicyDeny() {
    String iamPolicy = "{\r\n" + "  \"Version\": \"2012-10-17\",\r\n" +
                       "  \"Statement\": [\r\n" + "    {\r\n" +
                       "      \"Sid\": \"Stmt1638781464110\",\r\n" +
                       "      \"Action\": [\r\n" +
                       "        \"s3:PutObject\"\r\n" + "      ],\r\n" +
                       "      \"Effect\": \"Allow\",\r\n" +
                       "      \"Resource\": \"arn:aws:s3:::testbucket/*\"\r\n" +
                       "    }\r\n" + "  ]\r\n" + "}";
    String bucketPolicy =
        "{\r\n" + "  \"Version\": \"2012-10-17\",\r\n" +
        "  \"Statement\": [\r\n" + "    {\r\n" +
        "      \"Sid\": \"Stmt1638781464110\",\r\n" +
        "      \"Action\": [\r\n" + "        \"s3:PutObject\"\r\n" +
        "      ],\r\n" + "      \"Effect\": \"Deny\",\r\n" +
        "	  \"Resource\": \"arn:aws:s3:::testbucket/*\",\r\n" +
        "	  \"Principal\": \"*\"\r\n" + "    }\r\n" + "  ]\r\n" + "}";
    requestBody = new TreeMap<>();
    String acl =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
        "<AccessControlPolicy" +
        " xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">" + "<Owner><ID>" +
        "qWwZGnGYTga8gbpcuY79SA" + "</ID>" +
        "<DisplayName>kirungeb</DisplayName></Owner><AccessControlList>" +
        "<Grant>" + "<Grantee " +
        "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" " +
        "xsi:type=\"CanonicalUser\"><ID>" + "qWwZGnGYTga8gbpcuY79SA" + "</ID>" +
        "<DisplayName>kirungeb</DisplayName></Grantee>" +
        "<Permission>FULL_CONTROL</Permission></Grant></AccessControlList>" +
        "</AccessControlPolicy>";
    requestBody.put("Auth-ACL", BinaryUtil.encodeToBase64String(acl));
    ServerResponse actualServerResponse = null;
    requestBody.put("ClientQueryParams", "");
    requestBody.put("Method", "PUT");
    requestBody.put("ClientAbsoluteUri", "/testbucket/a.txt");
    requestBody.put("S3Action", "PutObject");
    requestBody.put("Policy", bucketPolicy);
    List<String> policys = new ArrayList<>();
    policys.add(iamPolicy);
    requestor.setPolicyDocuments(policys);
    actualServerResponse = authorizer.authorize(requestor, requestBody);
    assertEquals(HttpResponseStatus.FORBIDDEN,
                 actualServerResponse.getResponseStatus());
  }

  @Test public void testIamPolicyAllowBucketPolicyAllow() {

    String iamPolicy =
        "{\r\n" + "  \"Version\": \"2012-10-17\",\r\n" +
        "  \"Statement\": [\r\n" + "    {\r\n" +
        "      \"Sid\": \"Stmt1638781464110\",\r\n" +
        "      \"Action\": [\r\n" + "        \"s3:DeleteBucket\"\r\n" +
        "      ],\r\n" + "      \"Effect\": \"Allow\",\r\n" +
        "	  \"Resource\": \"*\"\r\n" + "    }\r\n" + "  ]\r\n" + "}";
    String bucketPolicy =
        "{\r\n" + "  \"Version\": \"2012-10-17\",\r\n" +
        "  \"Statement\": [\r\n" + "    {\r\n" +
        "      \"Sid\": \"Stmt1638781464110\",\r\n" +
        "      \"Action\": [\r\n" + "        \"s3:DeleteBucket\"\r\n" +
        "      ],\r\n" + "      \"Effect\": \"Allow\",\r\n" +
        "	  \"Resource\": \"arn:aws:s3:::testbucket\",\r\n" +
        "	  \"Principal\": \"*\"\r\n" + "    }\r\n" + "  ]\r\n" + "}";
    requestBody = new TreeMap<>();
    String acl =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
        "<AccessControlPolicy" +
        " xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">" + "<Owner><ID>" +
        "qWwZGnGYTga8gbpcuY79SA" + "</ID>" +
        "<DisplayName>kirungeb</DisplayName></Owner><AccessControlList>" +
        "<Grant>" + "<Grantee " +
        "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" " +
        "xsi:type=\"CanonicalUser\"><ID>" + "qWwZGnGYTga8gbpcuY79SA" + "</ID>" +
        "<DisplayName>kirungeb</DisplayName></Grantee>" +
        "<Permission>FULL_CONTROL</Permission></Grant></AccessControlList>" +
        "</AccessControlPolicy>";
    requestBody.put("Auth-ACL", BinaryUtil.encodeToBase64String(acl));
    ServerResponse actualServerResponse = null;
    requestBody.put("ClientQueryParams", "");
    requestBody.put("Method", "DELETE");
    requestBody.put("ClientAbsoluteUri", "/testbucket");
    requestBody.put("S3Action", "DeleteBucket");
    requestBody.put("Policy", bucketPolicy);
    List<String> policys = new ArrayList<>();
    policys.add(iamPolicy);
    requestor.setPolicyDocuments(policys);
    actualServerResponse = authorizer.authorize(requestor, requestBody);
    assertEquals(HttpResponseStatus.OK,
                 actualServerResponse.getResponseStatus());
  }

  @Test public void testIamPolicyDenyBucketPolicyAllow() {
    String iamPolicy =
        "{\r\n" + "  \"Version\": \"2012-10-17\",\r\n" +
        "  \"Statement\": [\r\n" + "    {\r\n" +
        "      \"Sid\": \"Stmt1638781464110\",\r\n" +
        "      \"Action\": [\r\n" + "        \"s3:GetBucketAcl\"\r\n" +
        "      ],\r\n" + "      \"Effect\": \"Deny\",\r\n" +
        "	  \"Resource\": \"*\"\r\n" + "    }\r\n" + "  ]\r\n" + "}";
    String bucketPolicy =
        "{\r\n" + "  \"Version\": \"2012-10-17\",\r\n" +
        "  \"Statement\": [\r\n" + "    {\r\n" +
        "      \"Sid\": \"Stmt1638781464110\",\r\n" +
        "      \"Action\": [\r\n" + "        \"s3:GetBucketAcl\"\r\n" +
        "      ],\r\n" + "      \"Effect\": \"Allow\",\r\n" +
        "	  \"Resource\": \"arn:aws:s3:::testbucket\",\r\n" +
        "	  \"Principal\": \"*\"\r\n" + "    }\r\n" + "  ]\r\n" + "}";
    requestBody = new TreeMap<>();
    String acl =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
        "<AccessControlPolicy" +
        " xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">" + "<Owner><ID>" +
        "qWwZGnGYTga8gbpcuY79SA" + "</ID>" +
        "<DisplayName>kirungeb</DisplayName></Owner><AccessControlList>" +
        "<Grant>" + "<Grantee " +
        "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" " +
        "xsi:type=\"CanonicalUser\"><ID>" + "qWwZGnGYTga8gbpcuY79SA" + "</ID>" +
        "<DisplayName>kirungeb</DisplayName></Grantee>" +
        "<Permission>FULL_CONTROL</Permission></Grant></AccessControlList>" +
        "</AccessControlPolicy>";
    requestBody.put("Auth-ACL", BinaryUtil.encodeToBase64String(acl));
    ServerResponse actualServerResponse = null;
    requestBody.put("ClientQueryParams", "");
    requestBody.put("Method", "GET");
    requestBody.put("ClientAbsoluteUri", "/testbucket");
    requestBody.put("S3Action", "GetBucketAcl");
    requestBody.put("Policy", bucketPolicy);
    List<String> policys = new ArrayList<>();
    policys.add(iamPolicy);
    requestor.setPolicyDocuments(policys);
    actualServerResponse = authorizer.authorize(requestor, requestBody);
    assertEquals(HttpResponseStatus.FORBIDDEN,
                 actualServerResponse.getResponseStatus());
  }

  @Test public void testIamPolicyNoDecisionBucketPolicyNoDecisionAclAllow() {
    String iamPolicy =
        "{\r\n" + "  \"Version\": \"2012-10-17\",\r\n" +
        "  \"Statement\": [\r\n" + "    {\r\n" +
        "      \"Sid\": \"Stmt1638781464110\",\r\n" +
        "      \"Action\": [\r\n" + "        \"s3:PutBucketAcl\"\r\n" +
        "      ],\r\n" + "      \"Effect\": \"Deny\",\r\n" +
        "	  \"Resource\": \"*\"\r\n" + "    }\r\n" + "  ]\r\n" + "}";
    String bucketPolicy =
        "{\r\n" + "  \"Version\": \"2012-10-17\",\r\n" +
        "  \"Statement\": [\r\n" + "    {\r\n" +
        "      \"Sid\": \"Stmt1638781464110\",\r\n" +
        "      \"Action\": [\r\n" + "        \"s3:PutBucketAcl\"\r\n" +
        "      ],\r\n" + "      \"Effect\": \"Allow\",\r\n" +
        "	  \"Resource\": \"arn:aws:s3:::testbucket\",\r\n" +
        "	  \"Principal\": \"*\"\r\n" + "    }\r\n" + "  ]\r\n" + "}";
    requestBody = new TreeMap<>();
    String acl =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
        "<AccessControlPolicy" +
        " xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">" + "<Owner><ID>" +
        "qWwZGnGYTga8gbpcuY79SA" + "</ID>" +
        "<DisplayName>kirungeb</DisplayName></Owner><AccessControlList>" +
        "<Grant>" + "<Grantee " +
        "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" " +
        "xsi:type=\"CanonicalUser\"><ID>" + "qWwZGnGYTga8gbpcuY79SA" + "</ID>" +
        "<DisplayName>kirungeb</DisplayName></Grantee>" +
        "<Permission>FULL_CONTROL</Permission></Grant></AccessControlList>" +
        "</AccessControlPolicy>";
    requestBody.put("Auth-ACL", BinaryUtil.encodeToBase64String(acl));
    ServerResponse actualServerResponse = null;
    requestBody.put("ClientQueryParams", "");
    requestBody.put("Method", "GET");
    requestBody.put("ClientAbsoluteUri", "/testbucket");
    requestBody.put("S3Action", "GetBucketAcl");
    requestBody.put("Policy", bucketPolicy);
    List<String> policys = new ArrayList<>();
    policys.add(iamPolicy);
    requestor.setPolicyDocuments(policys);
    actualServerResponse = authorizer.authorize(requestor, requestBody);
    assertEquals(HttpResponseStatus.OK,
                 actualServerResponse.getResponseStatus());
  }

  @Test public void testIamPolicyNoDecisionBucketPolicyNoDecisionAclDeny() {
    String iamPolicy =
        "{\r\n" + "  \"Version\": \"2012-10-17\",\r\n" +
        "  \"Statement\": [\r\n" + "    {\r\n" +
        "      \"Sid\": \"Stmt1638781464110\",\r\n" +
        "      \"Action\": [\r\n" + "        \"s3:PutBucketAcl\"\r\n" +
        "      ],\r\n" + "      \"Effect\": \"Deny\",\r\n" +
        "	  \"Resource\": \"*\"\r\n" + "    }\r\n" + "  ]\r\n" + "}";
    String bucketPolicy =
        "{\r\n" + "  \"Version\": \"2012-10-17\",\r\n" +
        "  \"Statement\": [\r\n" + "    {\r\n" +
        "      \"Sid\": \"Stmt1638781464110\",\r\n" +
        "      \"Action\": [\r\n" + "        \"s3:PutBucketAcl\"\r\n" +
        "      ],\r\n" + "      \"Effect\": \"Allow\",\r\n" +
        "	  \"Resource\": \"arn:aws:s3:::testbucket\",\r\n" +
        "	  \"Principal\": \"*\"\r\n" + "    }\r\n" + "  ]\r\n" + "}";
    requestBody = new TreeMap<>();
    String acl =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
        "<AccessControlPolicy" +
        " xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">" + "<Owner><ID>" +
        "qWwZGnGYTga8gbpcuY79SA" + "</ID>" +
        "<DisplayName>kirungeb</DisplayName></Owner><AccessControlList>" +
        "<Grant>" + "<Grantee " +
        "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" " +
        "xsi:type=\"CanonicalUser\"><ID>" + "qWwZGnGYTga8gbpcuY79SA" + "</ID>" +
        "<DisplayName>kirungeb</DisplayName></Grantee>" +
        "<Permission>WRITE</Permission></Grant></AccessControlList>" +
        "</AccessControlPolicy>";
    requestBody.put("Auth-ACL", BinaryUtil.encodeToBase64String(acl));
    ServerResponse actualServerResponse = null;
    requestBody.put("ClientQueryParams", "");
    requestBody.put("Method", "GET");
    requestBody.put("ClientAbsoluteUri", "/testbucket");
    requestBody.put("S3Action", "GetBucketAcl");
    requestBody.put("Policy", bucketPolicy);
    List<String> policys = new ArrayList<>();
    policys.add(iamPolicy);
    requestor.setPolicyDocuments(policys);
    actualServerResponse = authorizer.authorize(requestor, requestBody);
    assertEquals(HttpResponseStatus.FORBIDDEN,
                 actualServerResponse.getResponseStatus());
  }
}
