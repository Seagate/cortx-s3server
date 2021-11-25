package com.seagates3.controller;

import static org.mockito.Matchers.any;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;
import java.util.List;
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
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Account;
import com.seagates3.model.Policy;
import com.seagates3.model.Requestor;
import com.seagates3.response.ServerResponse;

import io.netty.handler.codec.http.HttpResponseStatus;

@RunWith(PowerMockRunner.class)
    @PrepareForTest({DAODispatcher.class, PolicyController.class,
                     AuthServerConfig.class})
    @PowerMockIgnore({"javax.management.*"}) public class PolicyControllerTest {

 private
  PolicyDAO mockPolicyDao;
 private
  Account account;
 private
  Policy policy;
 private
  Policy policy2;
 private
  Map<String, String> requestBody = new HashMap<>();
 private
  PolicyController policyController;

  @Before public void setUp() throws Exception {

    policy = new Policy();
    policy.setPolicyId("P1234");
    policy.setPath("/");
    policy.setName("policy1");
    policy.setAccount(account);
    policy.setARN("arn:aws:iam::352620587691:policy/policy1");
    policy.setCreateDate("2021/12/12 12:23:34");
    policy.setDefaultVersionId("0");
    policy.setAttachmentCount(0);
    policy.setPermissionsBoundaryUsageCount(0);
    policy.setIsPolicyAttachable("true");
    policy.setUpdateDate("2021/12/12 12:23:34");

    policy2 = new Policy();
    policy2.setPolicyId("P12345");
    policy2.setName("policy2");
    policy2.setAccount(account);
    policy2.setARN("arn:aws:iam::352620587691:policy/policy2");

    account = new Account();
    account.setId("352620587691");
    account.setName("account1");
    mockPolicyDao = Mockito.mock(PolicyDAO.class);
    PowerMockito.mockStatic(DAODispatcher.class);
    PowerMockito.mockStatic(AuthServerConfig.class);
    PowerMockito.doReturn("2012-10-17")
        .when(AuthServerConfig.class, "getPolicyVersion");
    PowerMockito.doReturn("0000").when(AuthServerConfig.class, "getReqId");

    Requestor requestor = new Requestor();
    requestor.setAccount(account);
    requestBody.put("PolicyName", "policy1");
    requestBody.put("PolicyARN", "arn:aws:iam::352620587691:policy/policy1");
    requestBody.put(
        "PolicyDocument",
        "{\r\n" + "\"Version\": \"2012-10-17\",\r\n" +
            "  \"Statement\": [\r\n" + "    {\r\n" +
            "      \"Sid\": \"Stmt1632740110513\",\r\n" +
            "      \"Action\": [\r\n" +
            "        \"s3:PutBucketAcljhghsghsd\"\r\n" + "      ],\r\n" +
            "      \"Effect\": \"Allow\",\r\n" +
            "      \"Resource\": \"arn:aws:iam::352620587691:buck1\"\r\n" +
            "	  \r\n" + "    }\r\n" + "\r\n" + "  ]\r\n" + "}");

    PowerMockito.doReturn(mockPolicyDao)
        .when(DAODispatcher.class, "getResourceDAO", DAOResource.POLICY);
    policyController = new PolicyController(requestor, requestBody);
  }

  @Test public void createPolicySuccess() throws DataAccessException {
    Mockito.doNothing().when(mockPolicyDao).save(any(Policy.class));
    ServerResponse response = policyController.create();
    Assert.assertEquals(HttpResponseStatus.CREATED,
                        response.getResponseStatus());
  }
  @Test public void createPolicyAlreadyExists() throws DataAccessException {
    Mockito.when(mockPolicyDao.find(account, "policy1")).thenReturn(policy);
    Mockito.doNothing().when(mockPolicyDao).save(any(Policy.class));
    ServerResponse response = policyController.create();
    Assert.assertEquals(HttpResponseStatus.CONFLICT,
                        response.getResponseStatus());
  }
  @Test public void createPolicyFailedException() throws DataAccessException {
    Mockito.doThrow(new DataAccessException("failed to add new policy\n"))
        .when(mockPolicyDao)
        .save(any(Policy.class));
    ServerResponse response = policyController.create();
    Assert.assertEquals(HttpResponseStatus.INTERNAL_SERVER_ERROR,
                        response.getResponseStatus());
  }
  @Test public void deletePolicySuccess() throws DataAccessException {
    Mockito.when(mockPolicyDao.findByArn(
                     "arn:aws:iam::352620587691:policy/policy1", account))
        .thenReturn(policy);
    Mockito.doNothing().when(mockPolicyDao).delete (policy);
    ServerResponse response = policyController.delete ();
    Assert.assertEquals(HttpResponseStatus.OK, response.getResponseStatus());
  }
  @Test public void deletePolicyNotExists() throws DataAccessException {
    ServerResponse response = policyController.delete ();
    Assert.assertEquals(HttpResponseStatus.NOT_FOUND,
                        response.getResponseStatus());
  }
  @Test public void deletePolicyFailedException() throws DataAccessException {
    Mockito.when(mockPolicyDao.findByArn(
                     "arn:aws:iam::352620587691:policy/policy1", account))
        .thenReturn(policy);
    Mockito.doThrow(new DataAccessException("failed to delete policy\n"))
        .when(mockPolicyDao)
        .delete (policy);
    ServerResponse response = policyController.delete ();
    Assert.assertEquals(HttpResponseStatus.INTERNAL_SERVER_ERROR,
                        response.getResponseStatus());
  }
  @Test public void listPoliciesSuccess() throws DataAccessException {
    List<Policy> policyList = new ArrayList<>();
    policyList.add(policy);
    Mockito.when(mockPolicyDao.findAll(account)).thenReturn(policyList);
    ServerResponse response = policyController.list();
    Assert.assertEquals(HttpResponseStatus.OK, response.getResponseStatus());
  }

  @Test public void listPoliciesFailsWithNullValues()
      throws DataAccessException {
    List<Policy> policyList = new ArrayList<>();
    policyList.add(policy2);
    Mockito.when(mockPolicyDao.findAll(account)).thenReturn(policyList);
    ServerResponse response = policyController.list();
    Assert.assertEquals(null, response);
  }
  @Test public void getPolicySuccess() throws DataAccessException {
    Mockito.when(mockPolicyDao.findByArn(
                     "arn:aws:iam::352620587691:policy/policy1", account))
        .thenReturn(policy);
    ServerResponse response = policyController.get();
    Assert.assertEquals(HttpResponseStatus.OK, response.getResponseStatus());
  }

  @Test public void getPolicyNotExists() throws DataAccessException {
    ServerResponse response = policyController.get();
    Assert.assertEquals(HttpResponseStatus.NOT_FOUND,
                        response.getResponseStatus());
  }
}
