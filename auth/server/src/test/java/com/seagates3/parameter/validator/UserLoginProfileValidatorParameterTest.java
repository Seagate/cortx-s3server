package com.seagates3.parameter.validator;

import com.seagates3.parameter.validator.UserLoginProfileParameterValidator;
import java.util.Map;
import java.util.TreeMap;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import org.junit.Before;
import org.junit.Test;

public
class UserLoginProfileValidatorParameterTest {
  UserLoginProfileParameterValidator userLoginProfileValidator;
  Map requestBody;

 public
  UserLoginProfileValidatorParameterTest() {
    userLoginProfileValidator = new UserLoginProfileParameterValidator();
  }

  @Before public void setUp() {
    requestBody = new TreeMap();
  }


  @Test public void Create_Password_True() {
    requestBody.put("UserName", "abcd");
    requestBody.put("Password", "abvdef");
    assertTrue(userLoginProfileValidator.isValidCreateParams(requestBody));
  }
}
