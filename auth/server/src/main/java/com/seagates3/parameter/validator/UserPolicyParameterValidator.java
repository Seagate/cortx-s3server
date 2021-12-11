package com.seagates3.parameter.validator;

import java.util.Map;

import com.seagates3.constants.APIRequestParamsConstants;

public
class UserPolicyParameterValidator extends AbstractParameterValidator {

 public
  Boolean isValidAttachParams(Map<String, String> requestBody) {
    return isValidUserPolicyParameters(requestBody);
  }

 public
  Boolean isValidDetachParams(Map<String, String> requestBody) {
    return isValidUserPolicyParameters(requestBody);
  }

 private
  Boolean isValidUserPolicyParameters(Map<String, String> requestBody) {
    if (!S3ParameterValidatorUtil.isValidName(requestBody.get(APIRequestParamsConstants.USER_NAME))) {
      return false;
    }

    return S3ParameterValidatorUtil.isValidARN(requestBody.get(APIRequestParamsConstants.POLICY_ARN));
  }
}
