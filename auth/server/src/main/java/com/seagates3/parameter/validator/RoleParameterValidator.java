package com.seagates3.parameter.validator;

import java.util.Map;

/**
 * Validate the input for role APIs - Create, Delete and List.
 */
public class RoleParameterValidator extends AbstractParameterValidator {

    /**
     * Validate the input parameters for isValidCreateParams role request. Role name is
     * required. AssumeRolePolicyDocument is required. Path is optional.
     *
     * @param requestBody TreeMap of input parameters.
     * @return true if input is valid.
     */
    @Override
    public Boolean isValidCreateParams(Map<String, String> requestBody) {
        if (requestBody.containsKey("Path")) {
            if (!S3ParameterValidatorUtil.isValidPath(
                    requestBody.get("Path"))) {
                return false;
            }
        }

        if (!S3ParameterValidatorUtil.isValidName(requestBody.get("RoleName"))) {
            return false;
        }

        return S3ParameterValidatorUtil.isValidAssumeRolePolicyDocument(
                requestBody.get("AssumeRolePolicyDocument"));
    }

    /**
     * Validate the input parameters for isValidDeleteParams role request. Role name is
     * required.
     *
     * @param requestBody TreeMap of input parameters.
     * @return true if input is valid.
     */
    @Override
    public Boolean isValidDeleteParams(Map<String, String> requestBody) {
        return S3ParameterValidatorUtil.isValidName(requestBody.get("RoleName"));
    }
}
