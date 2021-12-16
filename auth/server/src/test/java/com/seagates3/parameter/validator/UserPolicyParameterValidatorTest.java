package com.seagates3.parameter.validator;

import static org.junit.Assert.assertTrue;
import static org.junit.Assert.assertFalse;

import java.util.Map;
import java.util.TreeMap;

import org.junit.Before;
import org.junit.Test;

public
class UserPolicyParameterValidatorTest {

 private
  static final String VALID_USER_NAME = "iamuser11";
 private
  static final String VALID_POLICY_ARN =
      "arn:aws:iam::830072722140:policy/Policy1";
 private
  static final String INVALID_VALID_USER_NAME = "Abc&&&123";
 private
  static final String INVALID_VALID_POLICY_ARN =
      "arn:seagate:iam::830072722140:policy/Policy1";

 private
  UserPolicyParameterValidator userPolicyParameterValidator;

  @Before public void setUp() {
    userPolicyParameterValidator = new UserPolicyParameterValidator();
  }

  @Test public void attachValidParams() {
    assertTrue(userPolicyParameterValidator.isValidAttachParams(
        buildValidRequestBody()));
  }

  @Test public void attachInValidParams() {
    assertFalse(userPolicyParameterValidator.isValidAttachParams(
        buildInvalidUserValidRequestBody(null)));
    assertFalse(userPolicyParameterValidator.isValidAttachParams(
        buildInvalidUserValidRequestBody(INVALID_VALID_USER_NAME)));
    assertFalse(userPolicyParameterValidator.isValidAttachParams(
        buildInvalidArnValidRequestBody(null)));
    assertFalse(userPolicyParameterValidator.isValidAttachParams(
        buildInvalidArnValidRequestBody(INVALID_VALID_POLICY_ARN)));
  }

  @Test public void detachValidParams() {
    assertTrue(userPolicyParameterValidator.isValidDetachParams(
        buildValidRequestBody()));
  }

  @Test public void detachInValidParams() {
    assertFalse(userPolicyParameterValidator.isValidDetachParams(
        buildInvalidUserValidRequestBody(null)));
    assertFalse(userPolicyParameterValidator.isValidDetachParams(
        buildInvalidUserValidRequestBody(INVALID_VALID_USER_NAME)));
    assertFalse(userPolicyParameterValidator.isValidDetachParams(
        buildInvalidArnValidRequestBody(null)));
    assertFalse(userPolicyParameterValidator.isValidDetachParams(
        buildInvalidArnValidRequestBody(INVALID_VALID_POLICY_ARN)));
  }

 private
  Map buildValidRequestBody() {
    Map requestBody = new TreeMap();
    requestBody.put("UserName", VALID_USER_NAME);
    requestBody.put("PolicyArn", VALID_POLICY_ARN);

    return requestBody;
  }

 private
  Map buildInvalidUserValidRequestBody(String userName) {
    Map requestBody = new TreeMap();
    requestBody.put("UserName", userName);
    requestBody.put("PolicyArn", VALID_POLICY_ARN);

    return requestBody;
  }

 private
  Map buildInvalidArnValidRequestBody(String arn) {
    Map requestBody = new TreeMap();
    requestBody.put("UserName", VALID_USER_NAME);
    requestBody.put("PolicyArn", arn);

    return requestBody;
  }
}
