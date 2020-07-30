package com.seagates3.parameter.validator;

import java.util.Map;

/**
 * Validate the input for Account API - Create.
 */
public class AccountParameterValidator extends AbstractParameterValidator {
    /*
     * TODO
     * Find out maximum length of account name.
     */

    /**
     * Validate the input parameters for isValidCreateParams account request. Account name is
     * required.
     *
     * @param requestBody TreeMap of input parameters.
     * @return true if input is valid.
     */
    @Override
    public Boolean isValidCreateParams(Map<String, String> requestBody) {
        if (!S3ParameterValidatorUtil.isValidEmail(requestBody.get("Email"))) {
            return false;
        }

        return S3ParameterValidatorUtil.isValidName(
                requestBody.get("AccountName"));
    }

    /**
     * Validate the input parameters for isValidDeleteParams account request. Account name is
     * required.
     *
     * @param requestBody TreeMap of input parameters.
     * @return true if input is valid.
     */
    @Override
    public Boolean isValidDeleteParams(Map<String, String> requestBody) {
        return S3ParameterValidatorUtil.isValidName(
                requestBody.get("AccountName"));
    }

    /**
     * Validate the input parameters for isValidResetAccountAccessKeyParams
     *  account request. Account name is required.
     *
     * @param requestBody TreeMap of input parameters.
     * @return true if input is valid.
     */

    public Boolean isValidResetAccountAccessKeyParams(Map<String,
                                      String> requestBody) {
        return S3ParameterValidatorUtil.isValidName(
                requestBody.get("AccountName"));
    }

}
