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

