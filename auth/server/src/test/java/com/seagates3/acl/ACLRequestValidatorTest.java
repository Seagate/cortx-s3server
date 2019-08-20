package com.seagates3.acl;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
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
        ACLRequestValidator.class) public class ACLRequestValidatorTest {
 private
  Map<String, String> requestBody;
 private
  ACLRequestValidator spyValidator;
 private
  AuthorizationResponseGenerator responseGenerator;
 private
  AccountImpl mockAccountImpl;
 private
  Account mockAccount;
 private
  Map<String, List<Account>> accountPermissionMap;

  @Before public void setup() {
    requestBody = new TreeMap<>(String.CASE_INSENSITIVE_ORDER);
    responseGenerator = new AuthorizationResponseGenerator();
    spyValidator = Mockito.spy(new ACLRequestValidator());
    mockAccountImpl = Mockito.mock(AccountImpl.class);
    mockAccount = Mockito.mock(Account.class);
    accountPermissionMap = new HashMap<>();
  }

  /**
   * Below will succeed if valid canned acl requested in input
   */
  @Test public void testValidateAclRequest_canncedAclInRequest_Success() {
    requestBody.put("x-amz-acl", "public-read");
    ServerResponse response =
        spyValidator.validateAclRequest(requestBody, accountPermissionMap);
    Assert.assertNull(response.getResponseStatus());
  }

  /**
   * Below test will succeed if valid acl present in request body
   */
  @Test public void testValidateAclRequest_aclInRequestBody_Success() {
    requestBody.put("acp", "dummy_xml");
    ServerResponse response =
        spyValidator.validateAclRequest(requestBody, accountPermissionMap);
    Assert.assertNull(response.getResponseStatus());
  }

  /**
   * Below test will succeed if valid permission header present in request
   */
  @Test public void testValidateAclRequest_permissionHeadersInRequest_Success()
      throws Exception {
    ACLRequestValidator mockValidator = Mockito.mock(ACLRequestValidator.class);
    WhiteboxImpl.setInternalState(mockValidator, "responseGenerator",
                                  responseGenerator);
    Mockito.when(mockValidator.validateAclRequest(
                     requestBody, accountPermissionMap)).thenCallRealMethod();
    Mockito.when(mockValidator.isValidPermissionHeader(requestBody, null,
                                                       accountPermissionMap))
        .thenCallRealMethod();
    requestBody.put(
        "x-amz-grant-read",
        "emailaddress=abc@seagate.com,emailaddress=xyz@seagate.com");
    Mockito.when(mockValidator.isGranteeValid(
                     "emailaddress=abc@seagate.com", null, accountPermissionMap,
                     "x-amz-grant-read")).thenReturn(true);
    Mockito.when(mockValidator.isGranteeValid(
                     "emailaddress=xyz@seagate.com", null, accountPermissionMap,
                     "x-amz-grant-read")).thenReturn(true);
    ServerResponse response =
        mockValidator.validateAclRequest(requestBody, accountPermissionMap);
    Assert.assertNull(response.getResponseStatus());
  }

  /**
   * Below test will fail when more than one acl headers/requestbody present in
   * request
   */
  @Test public void testValidateAclRequest_multipleacl_InRequest_Check() {
    requestBody.put("x-amz-acl", "public-read");
    requestBody.put(
        "x-amz-grant-read",
        "emailaddress=abc@seagate.com,emailaddress=xyz@seagate.com");
    ServerResponse response =
        spyValidator.validateAclRequest(requestBody, accountPermissionMap);
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
        spyValidator.isGranteeValid("emailaddress=xyz@seagate.com", null,
                                    accountPermissionMap, "x-amz-grant-read"));
  }

  /**
   * Below will test repetitive email address scenario
   *
   * @throws Exception
   */
  @Test public void testIsGranteeValid_repetitive_emailAddress_check()
      throws Exception {
    PowerMockito.whenNew(AccountImpl.class).withNoArguments().thenReturn(
        mockAccountImpl);
    Mockito.when(mockAccountImpl.findByEmailAddress("xyz@seagate.com"))
        .thenReturn(mockAccount);
    Mockito.when(mockAccount.exists()).thenReturn(true);
    Mockito.when(mockAccount.getCanonicalId()).thenReturn("id1");
    List<Account> accounts = new ArrayList<>();
    accounts.add(mockAccount);
    accountPermissionMap.put("x-amz-grant-read", accounts);
    Assert.assertTrue(
        spyValidator.isGranteeValid("emailaddress=xyz@seagate.com", null,
                                    accountPermissionMap, "x-amz-grant-read"));
    Assert.assertEquals(1, accountPermissionMap.size());
  }

  /**
   * Below will test multiple permissions for same account scenario
   *
   * @throws Exception
   */
  @Test public void testIsGranteeValid_multiple_permissions_same_account_check()
      throws Exception {
    PowerMockito.whenNew(AccountImpl.class).withNoArguments().thenReturn(
        mockAccountImpl);
    Mockito.when(mockAccountImpl.findByEmailAddress("xyz@seagate.com"))
        .thenReturn(mockAccount);
    Mockito.when(mockAccount.exists()).thenReturn(true);
    Mockito.when(mockAccount.getCanonicalId()).thenReturn("id1");
    List<Account> accounts = new ArrayList<>();
    accounts.add(mockAccount);
    accountPermissionMap.put("x-amz-grant-write", accounts);
    Assert.assertTrue(
        spyValidator.isGranteeValid("emailaddress=xyz@seagate.com", null,
                                    accountPermissionMap, "x-amz-grant-read"));
    Assert.assertEquals(2, accountPermissionMap.size());
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
    Assert.assertFalse(spyValidator.isGranteeValid(
        "emailaddress=xyz@seagate.com", new ServerResponse(),
        accountPermissionMap, "x-amz-grant-read"));
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
    Assert.assertTrue(spyValidator.isGranteeValid(
        "id=C123453", null, accountPermissionMap, "x-amz-grant-read"));
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
    Assert.assertFalse(
        spyValidator.isGranteeValid("id=C123453", new ServerResponse(),
                                    accountPermissionMap, "x-amz-grant-read"));
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
    Assert.assertFalse(spyValidator.isGranteeValid(
        "id=C123453", null, accountPermissionMap, "x-amz-grant-read"));
  }

  /**
   * Below will test empty cannonical id scenario
   *
   * @throws Exception
   */
  @Test public void testIsGranteeValid_empty_cannonicalId_check_negativeTest()
      throws Exception {
    PowerMockito.whenNew(AccountImpl.class).withNoArguments().thenReturn(
        mockAccountImpl);
    Mockito.when(mockAccountImpl.findByCanonicalID("")).thenReturn(mockAccount);
    Mockito.when(mockAccount.exists()).thenReturn(false);
    Assert.assertFalse(spyValidator.isGranteeValid(
        "id=", new ServerResponse(), accountPermissionMap, "x-amz-grant-read"));
  }

  /**
   * Below will test invalid argument scenario
   *
   * @throws Exception
   */
  @Test public void testIsGranteeValid_invalid_argument_check()
      throws Exception {
    PowerMockito.whenNew(AccountImpl.class).withNoArguments().thenReturn(
        mockAccountImpl);
    Mockito.when(mockAccountImpl.findByCanonicalID("C123453"))
        .thenReturn(mockAccount);
    Mockito.when(mockAccount.exists()).thenReturn(true);
    Assert.assertFalse(
        spyValidator.isGranteeValid("cid=C123453", new ServerResponse(),
                                    accountPermissionMap, "x-amz-grant-read"));
  }
}
