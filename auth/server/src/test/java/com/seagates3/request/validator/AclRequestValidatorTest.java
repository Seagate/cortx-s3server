package com.seagates3.request.validator;

import java.util.Map;
import java.util.TreeMap;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mockito;
import org.powermock.api.mockito.PowerMockito;
import org.powermock.core.classloader.annotations.PowerMockIgnore;
import org.powermock.core.classloader.annotations.PrepareForTest;
import org.powermock.modules.junit4.PowerMockRunner;
import org.powermock.reflect.internal.WhiteboxImpl;

import com.seagates3.dao.ldap.AccountImpl;
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Account;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.AuthorizationResponseGenerator;

import io.netty.handler.codec.http.HttpResponseStatus;

@RunWith(PowerMockRunner.class) @PowerMockIgnore({"javax.management.*"})
    @PrepareForTest(
        AclRequestValidator.class) public class AclRequestValidatorTest {
 private
  Map<String, String> requestBody;
 private
  AclRequestValidator spyValidator;
 private
  AuthorizationResponseGenerator responseGenerator;
 private
  AccountImpl mockAccountImpl;
 private
  Account mockAccount;

  @Before public void setup() {
    requestBody = new TreeMap<>(String.CASE_INSENSITIVE_ORDER);
    responseGenerator = new AuthorizationResponseGenerator();
    spyValidator = Mockito.spy(new AclRequestValidator());
    mockAccountImpl = Mockito.mock(AccountImpl.class);
    mockAccount = Mockito.mock(Account.class);
  }

  /**
   * Below will succeed if valid canned acl requested in input
   */
  @Test public void testValidateAclRequest_canncedAclInRequest_Success() {
    requestBody.put("x-amz-acl", "public-read");
    ServerResponse response = spyValidator.validateAclRequest(requestBody);
    Assert.assertNull(response);
  }

  /**
   * Below test will succeed if valid acl present in request body
   */
  @Test public void testValidateAclRequest_aclInRequestBody_Success() {
    requestBody.put("acp", "dummy_xml");
    ServerResponse response = spyValidator.validateAclRequest(requestBody);
    Assert.assertNull(response);
  }

  /**
   * Below test will succeed if valid permission header present in request
   */
  @Test public void testValidateAclRequest_permissionHeadersInRequest_Success()
      throws Exception {
    AclRequestValidator mockValidator = Mockito.mock(AclRequestValidator.class);
    WhiteboxImpl.setInternalState(mockValidator, "responseGenerator",
                                  responseGenerator);
    Mockito.when(mockValidator.validateAclRequest(requestBody))
        .thenCallRealMethod();
    Mockito.when(mockValidator.isValidPermissionHeader(requestBody, null))
        .thenCallRealMethod();
    requestBody.put(
        "x-amz-grant-read",
        "emailaddress=abc@seagate.com,emailaddress=xyz@seagate.com");
    Mockito.when(mockValidator.isGranteeValid("emailaddress=abc@seagate.com",
                                              null)).thenReturn(true);
    Mockito.when(mockValidator.isGranteeValid("emailaddress=xyz@seagate.com",
                                              null)).thenReturn(true);
    ServerResponse response = mockValidator.validateAclRequest(requestBody);
    Assert.assertNull(response);
  }

  /**
   * Below test will fail when more than one acl headers/requestbody present
   * in request
   */
  @Test public void testValidateAclRequest_multipleacl_InRequest_Check() {
    requestBody.put("x-amz-acl", "public-read");
    requestBody.put(
        "x-amz-grant-read",
        "emailaddress=abc@seagate.com,emailaddress=xyz@seagate.com");
    ServerResponse response = spyValidator.validateAclRequest(requestBody);
    Assert.assertEquals(HttpResponseStatus.BAD_REQUEST,
                        response.getResponseStatus());
  }

  /**
   * Below will test valid email address scenario
   *
   * @throws Exception
   */
  @Test public void testIsGranteeValid_emailAddress_check_positiveTest()
      throws Exception {
    PowerMockito.whenNew(AccountImpl.class).withNoArguments().thenReturn(
        mockAccountImpl);
    Mockito.when(mockAccountImpl.findByEmailAddress("xyz@seagate.com"))
        .thenReturn(mockAccount);
    Mockito.when(mockAccount.exists()).thenReturn(true);
    Assert.assertTrue(
        spyValidator.isGranteeValid("emailaddress=xyz@seagate.com", null));
  }

  /**
   * Below will test Invalid email address scenario
   *
   * @throws Exception
   */
  @Test public void testIsGranteeValid_emailAddress_check_negativeTest()
      throws Exception {
    PowerMockito.whenNew(AccountImpl.class).withNoArguments().thenReturn(
        mockAccountImpl);
    Mockito.when(mockAccountImpl.findByEmailAddress("xyz@seagate.com"))
        .thenReturn(mockAccount);
    Mockito.when(mockAccount.exists()).thenReturn(false);
    Assert.assertFalse(
        spyValidator.isGranteeValid("emailaddress=xyz@seagate.com", null));
  }

  /**
   * Below will test valid cannonical id scenario
   *
   * @throws Exception
   */
  @Test public void testIsGranteeValid_cannonicalId_check_positiveTest()
      throws Exception {
    PowerMockito.whenNew(AccountImpl.class).withNoArguments().thenReturn(
        mockAccountImpl);
    Mockito.when(mockAccountImpl.findByCanonicalID("C123453"))
        .thenReturn(mockAccount);
    Mockito.when(mockAccount.exists()).thenReturn(true);
    Assert.assertTrue(spyValidator.isGranteeValid("id=C123453", null));
  }

  /**
   * Below will test Invalid cannonical id scenario
   *
   * @throws Exception
   */
  @Test public void testIsGranteeValid_cannonicalId_check_negativeTest()
      throws Exception {
    PowerMockito.whenNew(AccountImpl.class).withNoArguments().thenReturn(
        mockAccountImpl);
    Mockito.when(mockAccountImpl.findByCanonicalID("C123453"))
        .thenReturn(mockAccount);
    Mockito.when(mockAccount.exists()).thenReturn(false);
    Assert.assertFalse(spyValidator.isGranteeValid("id=C123453", null));
  }

  /**
   * Below will test Exception scenario
   *
   * @throws Exception
   */
  @Test public void testIsGranteeValid_exception_scenario_Test()
      throws Exception {
    PowerMockito.whenNew(AccountImpl.class).withNoArguments().thenReturn(
        mockAccountImpl);
    Mockito.when(mockAccountImpl.findByCanonicalID("C123453")).thenThrow(
        new DataAccessException("failed to search given cannocal id"));
    Assert.assertFalse(spyValidator.isGranteeValid("id=C123453", null));
  }
}
