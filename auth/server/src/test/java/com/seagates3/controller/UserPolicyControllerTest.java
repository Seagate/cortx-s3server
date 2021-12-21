package com.seagates3.controller;

import static org.mockito.Matchers.any;

import java.util.ArrayList;
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

import com.seagates3.authserver.AuthServerConfig;
import com.seagates3.dao.DAODispatcher;
import com.seagates3.dao.DAOResource;
import com.seagates3.dao.PolicyDAO;
import com.seagates3.dao.UserDAO;
import com.seagates3.dao.UserPolicyDAO;
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Account;
import com.seagates3.model.Policy;
import com.seagates3.model.Requestor;
import com.seagates3.model.User;
import com.seagates3.model.UserPolicy;
import com.seagates3.response.ServerResponse;

import io.netty.handler.codec.http.HttpResponseStatus;

@PowerMockIgnore({"javax.management.*"}) @RunWith(PowerMockRunner.class)
    @PrepareForTest(
        {DAODispatcher.class,
         AuthServerConfig.class}) public class UserPolicyControllerTest {

 private
  static final String ACCOUNT_NAME = "s3test";
 private
  static final String ACCOUNT_ID = "AIDA5KZQJXPTROAIAKCKO";
 private
  static final String USER_ID = "AAABBBCCCDDD";
 private
  static final String USER_NAME = "iamuser1";
 private
  static final String POLICY_ID = "EEEFFFGGGHHH";
 private
  static final String POLICY_ARN = "AIDA5KZQJXPTROAIAKCKO";
 private
  static final String POLICY_NAME = "IAMPOLICY";
 private
  static final String PATH_PREFIX = "/testpath/";

 private
  Requestor requestor;
 private
  Map<String, String> requestBody;

 private
  UserDAO userDAO;
 private
  PolicyDAO policyDAO;
 private
  UserPolicyDAO userPolicyDAO;

 private
  Account s3Account;
 private
  User user;
 private
  User userWithPolicy;
 private
  Policy policy;
 private
  List<Policy> userAttachedPolicies;

  @Before public void setUp() throws Exception {
    PowerMockito.mockStatic(DAODispatcher.class);
    PowerMockito.mockStatic(AuthServerConfig.class);

    s3Account = new Account();
    s3Account.setId(ACCOUNT_ID);
    s3Account.setName(ACCOUNT_NAME);

    requestor = new Requestor();
    requestor.setAccount(s3Account);

    user = buildUser();
    User nullUser = buildUser();
    nullUser.setId(null);
    userWithPolicy = buildUserWithPolicy();
    policy = buildPolicy();

    requestor.setUser(user);

    userAttachedPolicies = new ArrayList<Policy>();
    userAttachedPolicies.add(policy);

    requestBody = new TreeMap<>(String.CASE_INSENSITIVE_ORDER);
    requestBody.put("UserName", USER_NAME);
    requestBody.put("PolicyArn", POLICY_ARN);

    userDAO = Mockito.mock(UserDAO.class);
    PowerMockito.doReturn(userDAO)
        .when(DAODispatcher.class, "getResourceDAO", DAOResource.USER);

    policyDAO = Mockito.mock(PolicyDAO.class);
    PowerMockito.doReturn(policyDAO)
        .when(DAODispatcher.class, "getResourceDAO", DAOResource.POLICY);

    userPolicyDAO = Mockito.mock(UserPolicyDAO.class);
    PowerMockito.doReturn(userPolicyDAO)
        .when(DAODispatcher.class, "getResourceDAO", DAOResource.USER_POLICY);

    PowerMockito.doReturn("0000").when(AuthServerConfig.class, "getReqId");
  }

  @Test public void attachSuccess() throws Exception {
    Mockito.doReturn(user).when(userDAO).find(ACCOUNT_NAME, USER_NAME);
    Mockito.doReturn(policy).when(policyDAO).findByArn(POLICY_ARN, s3Account);
    Mockito.doNothing().when(userPolicyDAO).attach(any(UserPolicy.class));

    String expectedResponseBody =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" " +
        "standalone=\"no\"?><AttachUserPolicyResponse " +
        "xmlns=\"https://iam.seagate.com/doc/2010-05-08/" +
        "\"><ResponseMetadata><RequestId>0000</RequestId></" +
        "ResponseMetadata></AttachUserPolicyResponse>";

    UserPolicyController userPolicyController =
        new UserPolicyController(requestor, requestBody);
    ServerResponse response = userPolicyController.attach();

    Assert.assertEquals(expectedResponseBody, response.getResponseBody());
    Assert.assertEquals(HttpResponseStatus.OK, response.getResponseStatus());
  }

  @Test public void attachFailUserDoesNotExists() throws Exception {
    ServerResponse serverResponse = Mockito.mock(ServerResponse.class);
    Mockito.doReturn(HttpResponseStatus.NOT_FOUND)
        .when(serverResponse)
        .getResponseStatus();

    Mockito.doReturn(null).when(userDAO).find(ACCOUNT_NAME, USER_NAME);

    UserPolicyController userPolicyController =
        new UserPolicyController(requestor, requestBody);
    ServerResponse response = userPolicyController.attach();

    Assert.assertEquals(HttpResponseStatus.NOT_FOUND,
                        response.getResponseStatus());
  }

  @Test public void attachFailPolicyDoesNotExists() throws Exception {
    Mockito.doReturn(user).when(userDAO).find(ACCOUNT_NAME, USER_NAME);
    Mockito.doReturn(null).when(policyDAO).findByArn(POLICY_ARN, s3Account);

    ServerResponse serverResponse = Mockito.mock(ServerResponse.class);
    Mockito.doReturn(HttpResponseStatus.NOT_FOUND)
        .when(serverResponse)
        .getResponseStatus();

    UserPolicyController userPolicyController =
        new UserPolicyController(requestor, requestBody);
    ServerResponse response = userPolicyController.attach();

    Assert.assertEquals(HttpResponseStatus.NOT_FOUND,
                        response.getResponseStatus());
  }

  @Test public void attachFailPolicyAlreadyAttached() throws Exception {
    Mockito.doReturn(userWithPolicy).when(userDAO).find(ACCOUNT_NAME,
                                                        USER_NAME);
    Mockito.doReturn(policy).when(policyDAO).findByArn(POLICY_ARN, s3Account);

    ServerResponse serverResponse = Mockito.mock(ServerResponse.class);
    Mockito.doReturn(HttpResponseStatus.CONFLICT)
        .when(serverResponse)
        .getResponseStatus();

    UserPolicyController userPolicyController =
        new UserPolicyController(requestor, requestBody);
    ServerResponse response = userPolicyController.attach();

    Assert.assertEquals(HttpResponseStatus.CONFLICT,
                        response.getResponseStatus());
  }

  @Test public void attachFailNullPolicyId() throws Exception {
    Policy policy = buildPolicy();
    policy.setPolicyId(null);

    Mockito.doReturn(user).when(userDAO).find(ACCOUNT_NAME, USER_NAME);
    Mockito.doReturn(policy).when(policyDAO).findByArn(POLICY_ARN, s3Account);

    UserPolicyController userPolicyController =
        new UserPolicyController(requestor, requestBody);
    ServerResponse response = userPolicyController.attach();

    Assert.assertEquals(HttpResponseStatus.NOT_FOUND,
                        response.getResponseStatus());
  }

  @Test public void attachFailPolicyIsNotAttachable() throws Exception {
    Policy policy = buildPolicy();
    policy.setIsPolicyAttachable("false");

    Mockito.doReturn(user).when(userDAO).find(ACCOUNT_NAME, USER_NAME);
    Mockito.doReturn(policy).when(policyDAO).findByArn(POLICY_ARN, s3Account);

    ServerResponse serverResponse = Mockito.mock(ServerResponse.class);
    Mockito.doReturn(HttpResponseStatus.BAD_REQUEST)
        .when(serverResponse)
        .getResponseStatus();

    UserPolicyController userPolicyController =
        new UserPolicyController(requestor, requestBody);
    ServerResponse response = userPolicyController.attach();

    Assert.assertEquals(HttpResponseStatus.BAD_REQUEST,
                        response.getResponseStatus());
  }

  @Test public void attachFailPolicyQuotaExceeded() throws Exception {
    List<String> attachedPolicies = new ArrayList<String>();
    for (int i = 0; i < AuthServerConfig.USER_POLICY_ATTACH_QUOTA; i++) {
      attachedPolicies.add(POLICY_ID + i);
    }
    User user = new User();
    user.setId(USER_ID);
    user.setPolicyIds(attachedPolicies);

    Mockito.doReturn(user).when(userDAO).find(ACCOUNT_NAME, USER_NAME);
    Mockito.doReturn(policy).when(policyDAO).findByArn(POLICY_ARN, s3Account);

    ServerResponse serverResponse = Mockito.mock(ServerResponse.class);
    Mockito.doReturn(HttpResponseStatus.BAD_REQUEST)
        .when(serverResponse)
        .getResponseStatus();

    UserPolicyController userPolicyController =
        new UserPolicyController(requestor, requestBody);
    ServerResponse response = userPolicyController.attach();

    Assert.assertEquals(HttpResponseStatus.BAD_REQUEST,
                        response.getResponseStatus());
  }

  @Test public void attachFailDataAccessException() throws Exception {
    Mockito.doThrow(DataAccessException.class).when(userDAO).find(ACCOUNT_NAME,
                                                                  USER_NAME);

    ServerResponse serverResponse = Mockito.mock(ServerResponse.class);
    Mockito.doReturn(HttpResponseStatus.INTERNAL_SERVER_ERROR)
        .when(serverResponse)
        .getResponseStatus();

    UserPolicyController userPolicyController =
        new UserPolicyController(requestor, requestBody);
    ServerResponse response = userPolicyController.attach();

    Assert.assertEquals(HttpResponseStatus.INTERNAL_SERVER_ERROR,
                        response.getResponseStatus());
  }

  @Test public void detachSuccess() throws Exception {
    Mockito.doReturn(userWithPolicy).when(userDAO).find(ACCOUNT_NAME,
                                                        USER_NAME);
    Mockito.doReturn(policy).when(policyDAO).findByArn(POLICY_ARN, s3Account);
    Mockito.doNothing().when(userPolicyDAO).detach(any(UserPolicy.class));

    String expectedResponseBody =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" " +
        "standalone=\"no\"?><DetachUserPolicyResponse " +
        "xmlns=\"https://iam.seagate.com/doc/2010-05-08/" +
        "\"><ResponseMetadata><RequestId>0000</RequestId></" +
        "ResponseMetadata></DetachUserPolicyResponse>";

    UserPolicyController userPolicyController =
        new UserPolicyController(requestor, requestBody);
    ServerResponse response = userPolicyController.detach();

    Assert.assertEquals(expectedResponseBody, response.getResponseBody());
    Assert.assertEquals(HttpResponseStatus.OK, response.getResponseStatus());
  }

  @Test public void detachFailUserDoesNotExists() throws Exception {
    ServerResponse serverResponse = Mockito.mock(ServerResponse.class);
    Mockito.doReturn(HttpResponseStatus.NOT_FOUND)
        .when(serverResponse)
        .getResponseStatus();

    Mockito.doReturn(null).when(userDAO).find(ACCOUNT_NAME, USER_NAME);

    UserPolicyController userPolicyController =
        new UserPolicyController(requestor, requestBody);
    ServerResponse response = userPolicyController.detach();

    Assert.assertEquals(HttpResponseStatus.NOT_FOUND,
                        response.getResponseStatus());
  }

  @Test public void detachFailPolicyDoesNotExists() throws Exception {
    Mockito.doReturn(user).when(userDAO).find(ACCOUNT_NAME, USER_NAME);
    Mockito.doReturn(null).when(policyDAO).findByArn(POLICY_ARN, s3Account);

    ServerResponse serverResponse = Mockito.mock(ServerResponse.class);
    Mockito.doReturn(HttpResponseStatus.NOT_FOUND)
        .when(serverResponse)
        .getResponseStatus();

    UserPolicyController userPolicyController =
        new UserPolicyController(requestor, requestBody);
    ServerResponse response = userPolicyController.detach();

    Assert.assertEquals(HttpResponseStatus.NOT_FOUND,
                        response.getResponseStatus());
  }

  @Test public void detachFailPolicyNotAttached() throws Exception {
    Mockito.doReturn(user).when(userDAO).find(ACCOUNT_NAME, USER_NAME);
    Mockito.doReturn(policy).when(policyDAO).findByArn(POLICY_ARN, s3Account);

    ServerResponse serverResponse = Mockito.mock(ServerResponse.class);
    Mockito.doReturn(HttpResponseStatus.NOT_FOUND)
        .when(serverResponse)
        .getResponseStatus();

    UserPolicyController userPolicyController =
        new UserPolicyController(requestor, requestBody);
    ServerResponse response = userPolicyController.detach();

    Assert.assertEquals(HttpResponseStatus.NOT_FOUND,
                        response.getResponseStatus());
  }

  @Test public void detachFailDataAccessException() throws Exception {
    Mockito.doThrow(DataAccessException.class).when(userDAO).find(ACCOUNT_NAME,
                                                                  USER_NAME);

    ServerResponse serverResponse = Mockito.mock(ServerResponse.class);
    Mockito.doReturn(HttpResponseStatus.INTERNAL_SERVER_ERROR)
        .when(serverResponse)
        .getResponseStatus();

    UserPolicyController userPolicyController =
        new UserPolicyController(requestor, requestBody);
    ServerResponse response = userPolicyController.detach();

    Assert.assertEquals(HttpResponseStatus.INTERNAL_SERVER_ERROR,
                        response.getResponseStatus());
  }

  @Test public void listAttachedUserPoliciesSucess() throws Exception {
    Mockito.doReturn(userAttachedPolicies).when(policyDAO).findByIds(
        Mockito.anyMapOf(String.class, Object.class));
    UserPolicyController userPolicyController =
        new UserPolicyController(requestor, requestBody);
    ServerResponse response = userPolicyController.list();
    String expectedResponseBody =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" " +
        "standalone=\"no\"?><ListAttachedUserPoliciesResponse " +
        "xmlns=\"https://iam.seagate.com/doc/2010-05-08/" +
        "\"><ListAttachedUserPoliciesResult><AttachedPolicies><member><" +
        "PolicyName>IAMPOLICY</PolicyName><PolicyArn>AIDA5KZQJXPTROAIAKCKO</" +
        "PolicyArn></member></AttachedPolicies><IsTruncated>false</" +
        "IsTruncated></" +
        "ListAttachedUserPoliciesResult><ResponseMetadata><RequestId>0000</" +
        "RequestId></ResponseMetadata></ListAttachedUserPoliciesResponse>";
    Assert.assertEquals(expectedResponseBody, response.getResponseBody());
    Assert.assertEquals(HttpResponseStatus.OK, response.getResponseStatus());
  }

  @Test public void listAttachedUserPoliciesWithPathSucess() throws Exception {
    requestBody.put("PathPrefix", PATH_PREFIX);
    Mockito.doReturn(userAttachedPolicies).when(policyDAO).findByIds(
        Mockito.anyMapOf(String.class, Object.class));
    UserPolicyController userPolicyController =
        new UserPolicyController(requestor, requestBody);
    ServerResponse response = userPolicyController.list();
    String expectedResponseBody =
        "<?xml version=\"1.0\" encoding=\"UTF-8\" " +
        "standalone=\"no\"?><ListAttachedUserPoliciesResponse " +
        "xmlns=\"https://iam.seagate.com/doc/2010-05-08/" +
        "\"><ListAttachedUserPoliciesResult><AttachedPolicies><member><" +
        "PolicyName>IAMPOLICY</PolicyName><PolicyArn>AIDA5KZQJXPTROAIAKCKO</" +
        "PolicyArn></member></AttachedPolicies><IsTruncated>false</" +
        "IsTruncated></" +
        "ListAttachedUserPoliciesResult><ResponseMetadata><RequestId>0000</" +
        "RequestId></ResponseMetadata></ListAttachedUserPoliciesResponse>";
    Assert.assertEquals(expectedResponseBody, response.getResponseBody());
    Assert.assertEquals(HttpResponseStatus.OK, response.getResponseStatus());
  }

 private
  User buildUser() {
    User user = new User();
    user.setId(USER_ID);

    return user;
  }

 private
  User buildUserWithPolicy() {
    List<String> attachedPolicies = new ArrayList<String>();
    attachedPolicies.add(POLICY_ID);
    User user = buildUser();
    user.setPolicyIds(attachedPolicies);

    return user;
  }

 private
  Policy buildPolicy() {
    Policy policy = new Policy();
    policy.setPolicyId(POLICY_ID);
    policy.setIsPolicyAttachable("true");
    policy.setARN(POLICY_ARN);
    policy.setName(POLICY_NAME);

    return policy;
  }
}
