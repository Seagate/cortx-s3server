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
