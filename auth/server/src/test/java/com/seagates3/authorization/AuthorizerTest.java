package com.seagates3.authorization;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import java.util.Map;
import java.util.TreeMap;

import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mockito;
import org.powermock.api.mockito.PowerMockito;
import org.powermock.core.classloader.annotations.PowerMockIgnore;
import org.powermock.core.classloader.annotations.PrepareForTest;
import org.powermock.modules.junit4.PowerMockRunner;

import com.seagates3.acl.ACLValidation;
import com.seagates3.authserver.AuthServerConfig;
import com.seagates3.dao.ldap.AccountImpl;
import com.seagates3.dao.ldap.LDAPUtils;
import com.seagates3.dao.ldap.UserImpl;
import com.seagates3.model.Account;
import com.seagates3.model.Requestor;
import com.seagates3.model.User;
import com.seagates3.policy.BucketPolicyAuthorizer;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.AuthorizationResponseGenerator;
import com.seagates3.util.BinaryUtil;

import io.netty.handler.codec.http.HttpResponseStatus;

@RunWith(PowerMockRunner.class)
    @PrepareForTest({LDAPUtils.class, ACLValidation.class,
                     BucketPolicyAuthorizer.class})
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
  UserImpl mockUserImpl;
 private
  Authorizer authorizer;
  AuthorizationResponseGenerator responseGenerator =
      new AuthorizationResponseGenerator();
 private
  Account mockAccount;
 private
  User user;

 private
  String serverResponseStringWithoutAcl =
      "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>" +
      "<AuthorizeUserResponse " +
      "xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">" +
      "<AuthorizeUserResult>" + "<UserId>456</UserId>" +
      "<UserName>tester</UserName>" + "<AccountId>12345</AccountId>" +
      "<AccountName>testAccount</AccountName>" + "</AuthorizeUserResult>" +
      "<ResponseMetadata>" + "<RequestId>0000</RequestId>" +
      "</ResponseMetadata>" + "</AuthorizeUserResponse>";

  @BeforeClass public static void setUpBeforeClass() {
    Account account = new Account();
    account.setCanonicalId("qWwZGnGYTga8gbpcuY79SA");
    account.setId("12345");
    account.setName("testAccount");

    requestor = new Requestor();
    requestor.setId("456");
    requestor.setName("tester");
    requestor.setAccount(account);
    AuthServerConfig.authResourceDir = "../resources";
  }

  @Before public void setup() {
    authorizer = new Authorizer();
    PowerMockito.mockStatic(LDAPUtils.class);
    PowerMockito.mockStatic(ACLValidation.class);
    mockAccountImpl = Mockito.mock(AccountImpl.class);
    mockAccount = Mockito.mock(Account.class);
    mockUserImpl = Mockito.mock(UserImpl.class);
    user = new User();
    user.setName("root");
  }

  @Test public void validateServerResponseWithRequestHeaderAsTrue() {

    ServerResponse actualServerResponse = null;
    requestBody = new TreeMap<>();
    requestBody.put(requestHeaderName, "true");
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
    PowerMockito.whenNew(UserImpl.class).withNoArguments().thenReturn(
        mockUserImpl);
    Mockito.when(mockUserImpl.findByUserId("456")).thenReturn(user);
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
    PowerMockito.whenNew(UserImpl.class).withNoArguments().thenReturn(
        mockUserImpl);
    Mockito.when(mockUserImpl.findByUserId("456")).thenReturn(user);
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
    PowerMockito.whenNew(AccountImpl.class).withNoArguments().thenReturn(
        mockAccountImpl);
    Mockito.when(mockAccountImpl.findByCanonicalID("qWwZGnGYTga8gbpcuY79SA"))
        .thenReturn(mockAccount);
    Mockito.when(mockAccount.getName()).thenReturn("abcd");
    actualServerResponse = authorizer.authorize(requestor, requestBody);
    assertEquals(HttpResponseStatus.METHOD_NOT_ALLOWED,
                 actualServerResponse.getResponseStatus());
  }
}
