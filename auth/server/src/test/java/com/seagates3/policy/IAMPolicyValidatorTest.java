package com.seagates3.policy;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.powermock.api.mockito.PowerMockito;
import org.powermock.core.classloader.annotations.PowerMockIgnore;
import org.powermock.core.classloader.annotations.PrepareForTest;
import org.powermock.modules.junit4.PowerMockRunner;

import com.seagates3.authserver.AuthServerConfig;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.generator.PolicyResponseGenerator;

import io.netty.handler.codec.http.HttpResponseStatus;

@RunWith(PowerMockRunner.class)
    @PrepareForTest({BucketPolicyValidator.class, AuthServerConfig.class})
    @PowerMockIgnore(
        {"javax.management.*"}) public class IAMPolicyValidatorTest {
  PolicyValidator validator = null;
  PolicyResponseGenerator responseGenerator = null;
  String syntxErrorResponse = null;

  @Before public void setUp() throws Exception {
    validator = new IAMPolicyValidator();
    responseGenerator = new PolicyResponseGenerator();
    syntxErrorResponse =
        responseGenerator.malformedPolicy("Syntax errors in policy.")
            .getResponseBody();
    PowerMockito.mockStatic(AuthServerConfig.class);
    PowerMockito.doReturn("2012-10-17")
        .when(AuthServerConfig.class, "getPolicyVersion");
    PowerMockito.doReturn("0000").when(AuthServerConfig.class, "getReqId");
  }

  @Test public void testValidatePolicyPositive() {
    String positiveJsonInput =
        "{\"Version\":\"2012-10-17\",\"Statement\":[{\"Sid\":" +
        "\"VisualEditor0\"," +
        "\"Effect\":\"Allow\",\"Action\":[\"iam:CreatePolicy\",\"iam:" +
        "ListAccessKeys\"],\"Resource\":[\"arn:aws:iam::352620587691:user/" +
        "abc\",\"arn:aws:iam::352620587691:policy/testpolicy\"]}]}";
    ServerResponse response = validator.validatePolicy(null, positiveJsonInput);
    Assert.assertNull(response);
  }

  @Test public void testValidatePolicyVersionMissing() {
    String negativeJsonInput =
        "{\"Statement\":[{\"Sid\":\"VisualEditor0\",\"Effect\":\"Allow\"," +
        "\"Action\":[\"iam:CreatePolicy\",\"iam:ListAccessKeys\"]," +
        "\"Resource\":" + "[\"arn:aws:iam::352620587691:user/" +
        "abc\",\"arn:aws:iam::352620587691:policy/testpolicy\"]}]}";
    ServerResponse response = validator.validatePolicy(null, negativeJsonInput);
    Assert.assertNotNull(response);
    Assert.assertEquals(syntxErrorResponse, response.getResponseBody());
    Assert.assertEquals(HttpResponseStatus.BAD_REQUEST,
                        response.getResponseStatus());
  }

  @Test public void testValidatePolicyStatementMissing() {
    String negativeJsonInput = "{\"Version\":\"2012-10-17\"}";
    ServerResponse response = validator.validatePolicy(null, negativeJsonInput);
    Assert.assertNotNull(response);
    Assert.assertEquals(syntxErrorResponse, response.getResponseBody());
    Assert.assertEquals(HttpResponseStatus.BAD_REQUEST,
                        response.getResponseStatus());
  }

  @Test public void testValidatePolicyVersionWrong() {
    String negativeJsonInput =
        "{\"Version\":\"2012-10-18\",\"Statement\":[{\"Sid\":" +
        "\"VisualEditor0\"," +
        "\"Effect\":\"Allow\",\"Action\":[\"iam:CreatePolicy\",\"iam:" +
        "ListAccessKeys\"],\"Resource\":[\"arn:aws:iam::352620587691:user/" +
        "abc\",\"arn:aws:iam::352620587691:policy/testpolicy\"]}]}";
    ServerResponse response = validator.validatePolicy(null, negativeJsonInput);
    Assert.assertNotNull(response);
    Assert.assertEquals(
        new PolicyResponseGenerator()
            .malformedPolicy(
                 "Policy document must have version 2012-10-17 or greater.")
            .getResponseBody(),
        response.getResponseBody());
    Assert.assertEquals(HttpResponseStatus.BAD_REQUEST,
                        response.getResponseStatus());
  }

  @Test public void testValidatePolicyUnknownElement() {
    String negativeJsonInput =
        "{\"unknown\":\"unknown\",\"Version\":\"2012-10-17\",\"Statement\":[{" +
        "\"Sid\":\"VisualEditor0\",\"Effect\":\"Allow\",\"Action\":[\"iam:" +
        "CreatePolicy\",\"iam:ListAccessKeys\"],\"Resource\":[\"arn:aws:iam::" +
        "352620587691:user/abc\",\"arn:aws:iam::352620587691:policy/" +
        "testpolicy\"]}]}";
    ServerResponse response = validator.validatePolicy(null, negativeJsonInput);
    Assert.assertNotNull(response);
    Assert.assertEquals(syntxErrorResponse, response.getResponseBody());
    Assert.assertEquals(HttpResponseStatus.BAD_REQUEST,
                        response.getResponseStatus());
  }

  @Test public void testValidatePolicyActionMissing() {
    String negativeJsonInput =
        "{\"Version\":\"2012-10-17\",\"Statement\":[{\"Sid\":" +
        "\"VisualEditor0\"," +
        "\"Effect\":\"Allow\",\"Resource\":[\"arn:aws:iam::352620587691:user/" +
        "abc\",\"arn:aws:iam::352620587691:policy/testpolicy\"]}]}";
    ServerResponse response = validator.validatePolicy(null, negativeJsonInput);
    Assert.assertNotNull(response);
    Assert.assertEquals(syntxErrorResponse, response.getResponseBody());
    Assert.assertEquals(HttpResponseStatus.BAD_REQUEST,
                        response.getResponseStatus());
  }

  @Test public void testValidatePolicyWrongActionSyntax1() {
    String negativeJsonInput =
        "{\"Version\":\"2012-10-17\",\"Statement\":[{\"Sid\":" +
        "\"VisualEditor0\"," +
        "\"Effect\":\"Allow\",\"Action\":[],\"Resource\":[\"arn:aws:iam::" +
        "352620587691:user/abc\",\"arn:aws:iam::352620587691:policy/" +
        "testpolicy\"]}]}";
    ServerResponse response = validator.validatePolicy(null, negativeJsonInput);
    Assert.assertNotNull(response);
    Assert.assertEquals(
        new PolicyResponseGenerator()
            .malformedPolicy("Policy statement must contain actions.")
            .getResponseBody(),
        response.getResponseBody());
    Assert.assertEquals(HttpResponseStatus.BAD_REQUEST,
                        response.getResponseStatus());
  }

  @Test public void testValidatePolicyWrongActionSyntax2() {
    String negativeJsonInput =
        "{\"Version\":\"2012-10-17\",\"Statement\":[{\"Sid\":" +
        "\"VisualEditor0\"," +
        "\"Effect\":\"Allow\",\"Action\":{},\"Resource\":[\"arn:aws:iam::" +
        "352620587691:user/abc\",\"arn:aws:iam::352620587691:policy/" +
        "testpolicy\"]}]}";
    ServerResponse response = validator.validatePolicy(null, negativeJsonInput);
    Assert.assertNotNull(response);
    Assert.assertEquals(syntxErrorResponse, response.getResponseBody());
    Assert.assertEquals(HttpResponseStatus.BAD_REQUEST,
                        response.getResponseStatus());
  }

  @Test public void testValidatePolicyWrongActionSyntax3() {
    String negativeJsonInput =
        "{\"Version\":\"2012-10-17\",\"Statement\":[{\"Sid\":" +
        "\"VisualEditor0\"," +
        "\"Effect\":\"Allow\",\"Action\":[{},{}],\"Resource\":[\"arn:aws:iam:" +
        ":" + "352620587691:user/abc\",\"arn:aws:iam::352620587691:policy/" +
        "testpolicy\"]}]}";
    ServerResponse response = validator.validatePolicy(null, negativeJsonInput);
    Assert.assertNotNull(response);
    Assert.assertEquals(syntxErrorResponse, response.getResponseBody());
    Assert.assertEquals(HttpResponseStatus.BAD_REQUEST,
                        response.getResponseStatus());
  }

  @Test public void testValidatePolicyResourceMissing() {
    String negativeJsonInput =
        "{\"Version\":\"2012-10-17\",\"Statement\":[{\"Sid\":" +
        "\"VisualEditor0\"," +
        "\"Effect\":\"Allow\",\"Action\":[\"iam:CreatePolicy\",\"iam:" +
        "ListAccessKeys\"]}]}";
    ServerResponse response = validator.validatePolicy(null, negativeJsonInput);
    Assert.assertNotNull(response);
    Assert.assertEquals(syntxErrorResponse, response.getResponseBody());
    Assert.assertEquals(HttpResponseStatus.BAD_REQUEST,
                        response.getResponseStatus());
  }

  @Test public void testValidatePolicyWrongResourceSyntax1() {
    String negativeJsonInput =
        "{\"Version\":\"2012-10-17\",\"Statement\":[{\"Sid\":" +
        "\"VisualEditor0\"," +
        "\"Effect\":\"Allow\",\"Action\":[\"iam:CreatePolicy\",\"iam:" +
        "ListAccessKeys\"],\"Resource\":{}}]}";
    ServerResponse response = validator.validatePolicy(null, negativeJsonInput);
    Assert.assertNotNull(response);
    Assert.assertEquals(syntxErrorResponse, response.getResponseBody());
    Assert.assertEquals(HttpResponseStatus.BAD_REQUEST,
                        response.getResponseStatus());
  }

  @Test public void testValidatePolicyWrongResourceSyntax2() {
    String negativeJsonInput =
        "{\"Version\":\"2012-10-17\",\"Statement\":[{\"Sid\":" +
        "\"VisualEditor0\"," +
        "\"Effect\":\"Allow\",\"Action\":[\"iam:CreatePolicy\",\"iam:" +
        "ListAccessKeys\"],\"Resource\":[]}]}";
    ServerResponse response = validator.validatePolicy(null, negativeJsonInput);
    Assert.assertNotNull(response);
    Assert.assertEquals(
        new PolicyResponseGenerator()
            .malformedPolicy("Policy statement must contain resources.")
            .getResponseBody(),
        response.getResponseBody());
    Assert.assertEquals(HttpResponseStatus.BAD_REQUEST,
                        response.getResponseStatus());
  }

  @Test public void testValidatePolicyWrongResourceSyntax3() {
    String negativeJsonInput =
        "{\"Version\":\"2012-10-17\",\"Statement\":[{\"Sid\":" +
        "\"VisualEditor0\"," +
        "\"Effect\":\"Allow\",\"Action\":[\"iam:CreatePolicy\",\"iam:" +
        "ListAccessKeys\"],\"Resource\":[{},{}]}]}";
    ServerResponse response = validator.validatePolicy(null, negativeJsonInput);
    Assert.assertNotNull(response);
    Assert.assertEquals(syntxErrorResponse, response.getResponseBody());
    Assert.assertEquals(HttpResponseStatus.BAD_REQUEST,
                        response.getResponseStatus());
  }

  @Test public void testValidatePolicyInvalidResourceArn() {
    String negativeJsonInput =
        "{\"Version\":\"2012-10-17\",\"Statement\":[{\"Sid\":" +
        "\"VisualEditor0\"," +
        "\"Effect\":\"Allow\",\"Action\":[\"iam:CreatePolicy\",\"iam:" +
        "ListAccessKeys\"],\"Resource\":[\"arn:awss:iam::352620587691:user/" +
        "abc\",\"arn:aws:ia::352620587691:policy/testpolicy\"]}]}";
    ServerResponse response = validator.validatePolicy(null, negativeJsonInput);
    Assert.assertNotNull(response);
    Assert.assertEquals(
        new PolicyResponseGenerator()
            .malformedPolicy("Resource arn:awss:iam::352620587691:user" +
                             "/abc must be in ARN format or \"*\".")
            .getResponseBody(),
        response.getResponseBody());
    Assert.assertEquals(HttpResponseStatus.BAD_REQUEST,
                        response.getResponseStatus());
  }

  @Test public void testValidatePolicyEffectMissing() {
    String negativeJsonInput =
        "{\"Version\":\"2012-10-17\",\"Statement\":[{\"Sid\":" +
        "\"VisualEditor0\"," +
        "\"Action\":[\"iam:CreatePolicy\",\"iam:ListAccessKeys\"]," +
        "\"Resource\":" + "[\"arn:aws:iam::352620587691:user/" +
        "abc\",\"arn:aws:iam::352620587691:policy/testpolicy\"]}]}";
    ServerResponse response = validator.validatePolicy(null, negativeJsonInput);
    Assert.assertNotNull(response);
    Assert.assertEquals(syntxErrorResponse, response.getResponseBody());
    Assert.assertEquals(HttpResponseStatus.BAD_REQUEST,
                        response.getResponseStatus());
  }

  @Test public void testValidatePolicyInvalidEffect() {
    String negativeJsonInput =
        "{\"Version\":\"2012-10-17\",\"Statement\":[{\"Sid\":" +
        "\"VisualEditor0\"," +
        "\"Effect\":\"Allo\",\"Action\":[\"iam:CreatePolicy\",\"iam:" +
        "ListAccessKeys\"],\"Resource\":[\"arn:aws:iam::352620587691:user/" +
        "abc\",\"arn:aws:iam::352620587691:policy/testpolicy\"]}]}";
    ServerResponse response = validator.validatePolicy(null, negativeJsonInput);
    Assert.assertNotNull(response);
    Assert.assertEquals(syntxErrorResponse, response.getResponseBody());
    Assert.assertEquals(HttpResponseStatus.BAD_REQUEST,
                        response.getResponseStatus());
  }

  @Test public void testValidatePolicyDuplicateSids() {
    String negativeJsonInput =
        "{\"Version\":\"2012-10-17\",\"Statement\":" +
        "[{\"Sid\":\"VisualEditor0\",\"Effect\":\"Allow\"," +
        "\"Action\":[\"iam:CreatePolicy\",\"iam:ListAccessKeys\"]," +
        "\"Resource\":[\"arn:aws:iam::352620587691:user/abc\"," +
        "\"arn:aws:iam::352620587691:policy/testpolicy\"]}," +
        "{\"Sid\":\"VisualEditor0\",\"Effect\":\"Allow\"," +
        "\"Action\":[\"iam:CreatePolicy\",\"iam:ListAccessKeys\"]," +
        "\"Resource\":[\"arn:aws:iam::352620587691:user/abc\"," +
        "\"arn:aws:iam::352620587691:policy/testpolicy\"]}]}";
    ServerResponse response = validator.validatePolicy(null, negativeJsonInput);
    Assert.assertNotNull(response);
    Assert.assertEquals(
        new PolicyResponseGenerator()
            .malformedPolicy(
                 "Statement IDs (SID) in a single policy must be unique.")
            .getResponseBody(),
        response.getResponseBody());
    Assert.assertEquals(HttpResponseStatus.BAD_REQUEST,
                        response.getResponseStatus());
  }
}