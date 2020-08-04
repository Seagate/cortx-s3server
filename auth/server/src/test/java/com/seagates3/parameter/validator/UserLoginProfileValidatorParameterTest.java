/*
 * Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For any questions about this software or licensing,
 * please email opensource@seagate.com or cortx-questions@seagate.com.
 *
 */

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
