package com.seagates3.parameter.validator;

import java.util.Map;

/**
 * Validate the input for User APIs - Create, Delete, List and isValidUpdateParams.
 */
public class UserParameterValidator extends AbstractParameterValidator {

    /**
     * Validate the input parameters for isValidCreateParams user request.
     * User name is required.
     * Path is optional.
     *
     * @param requestBody TreeMap of input parameters.
     * @return true if input is valid.
     */
    @Override
    public Boolean isValidCreateParams(Map<String, String> requestBody) {
        if (requestBody.containsKey("Path")) {
            return S3ParameterValidatorUtil.isValidPath(requestBody.get("Path"));
        }

        return S3ParameterValidatorUtil.isValidName(requestBody.get("UserName"));
    }

    /**
     * Validate the input parameters for isValidDeleteParams user request.
     * User name is required.
     *
     * @param requestBody TreeMap of input parameters.
     * @return true if input is valid.
     */
    @Override
    public Boolean isValidDeleteParams(Map<String, String> requestBody) {
        return S3ParameterValidatorUtil.isValidName(requestBody.get("UserName"));
    }

    /**
     * Validate the input parameters for isValidUpdateParams user request.
     * User name is required.
     * New User name is optional.
     * New Path is optional.
     *
     * @param requestBody TreeMap of input parameters.
     * @return true if input is valid.
     */
    @Override
    public Boolean isValidUpdateParams(Map<String, String> requestBody) {
        if (requestBody.containsKey("NewUserName")) {
            if (!S3ParameterValidatorUtil.isValidName(requestBody.get("NewUserName"))) {
                return false;
            }
        }

        if (requestBody.containsKey("NewPath")) {
            if (!S3ParameterValidatorUtil.isValidPath(requestBody.get("NewPath"))) {
                return false;
            }
        }

        return S3ParameterValidatorUtil.isValidName(requestBody.get("UserName"));
    }
}
