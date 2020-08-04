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

import java.util.Map;

public
class UserLoginProfileParameterValidator extends AbstractParameterValidator {

 public
  Boolean isValidCreateParams(Map<String, String> requestBody) {
    if (("true".equals(requestBody.get("PasswordResetRequired"))) &&
        ("false".equals(requestBody.get("PasswordResetRequired")))) {
      return false;
    }

    return S3ParameterValidatorUtil.isValidName(requestBody.get("UserName"));
  }

  @Override public Boolean isValidListParams(Map<String, String> requestBody) {
    return S3ParameterValidatorUtil.isValidName(requestBody.get("UserName"));
  }

  @Override public Boolean isValidUpdateParams(
      Map<String, String> requestBody) {

    if (("true".equals(requestBody.get("PasswordResetRequired"))) &&
        ("false".equals(requestBody.get("PasswordResetRequired")))) {
      return false;
    }
    return S3ParameterValidatorUtil.isValidName(requestBody.get("UserName"));
  }

  /**
    * Validate IAM user changePassword parameters i.e. OldPassword and
   * NewPassowrd
    */

  @Override public Boolean isValidChangepasswordParams(
      Map<String, String> requestBody) {

    if (requestBody.get("OldPassword") != null &&
        !(S3ParameterValidatorUtil.isValidPassword(
             requestBody.get("OldPassword")))) {
      return false;
    }
    if (requestBody.get("NewPassword") != null &&
        !(S3ParameterValidatorUtil.isValidPassword(
             requestBody.get("NewPassword")))) {
      return false;
    }
    return true;
  }
}

