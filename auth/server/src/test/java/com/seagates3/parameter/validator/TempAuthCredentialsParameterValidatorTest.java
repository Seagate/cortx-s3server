package com.seagates3.parameter.validator;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import java.util.HashMap;
import java.util.Map;

import org.junit.Before;
import org.junit.Test;

public
class TempAuthCredentialsParameterValidatorTest {

  Map<String, String> requestBody = new HashMap<>();
  TempAuthCredentialsParameterValidator tempAuthCredentialsParameterValidator =
      null;

  @Before public void setUp() throws Exception {
    requestBody.put("AccountName", "abcd");
    requestBody.put("Password", "pwd");
    tempAuthCredentialsParameterValidator =
        new TempAuthCredentialsParameterValidator();
  }

  @Test public void isValidCreateParams_withoutUserName_success_test() {
    assertTrue(
        tempAuthCredentialsParameterValidator.isValidCreateParams(requestBody));
  }

  @Test public void isValidCreateParams_withUserName_success_test() {
    requestBody.put("UserName", "uname");
    assertTrue(
        tempAuthCredentialsParameterValidator.isValidCreateParams(requestBody));
  }
}
