package com.seagates3.authorization;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import java.util.Map;
import java.util.TreeMap;

import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;

import com.seagates3.authserver.AuthServerConfig;
import com.seagates3.model.Account;
import com.seagates3.model.Requestor;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.AuthorizationResponseGenerator;

import io.netty.handler.codec.http.HttpResponseStatus;

public
class AuthorizerTest {

 private
  Map<String, String> requestBody = null;
 private
  static Requestor requestor = new Requestor();
 private
  String requestHeaderName = "Request-Object-ACL";
 private
  Authorizer authorizer;
  AuthorizationResponseGenerator responseGenerator =
      new AuthorizationResponseGenerator();

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
    authorizer.authorize(requestor, requestBody);
    actualServerResponse = authorizer.authorize(requestor, requestBody);
    assertEquals(HttpResponseStatus.OK,
                 actualServerResponse.getResponseStatus());
    assertTrue(actualServerResponse.getResponseBody().matches(
        "[\\s\\S]*\\<ACL\\>[\\s\\S]*\\<\\/ACL\\>[\\s\\S]*"));
  }
}
