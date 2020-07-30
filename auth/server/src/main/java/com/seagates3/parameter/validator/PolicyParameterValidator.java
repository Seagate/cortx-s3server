package com.seagates3.parameter.validator;

import java.util.Map;

/**
 * Validate the input for managed policy APIs - Create, Delete, List and Update.
 */
public class PolicyParameterValidator extends AbstractParameterValidator {

    /**
     * Validate the input parameters for isValidCreateParams policy request. Policy name is
     * required. Policy document is required. Path and description are optional.
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

        if (requestBody.containsKey("Description")) {
            if (!S3ParameterValidatorUtil.isValidDescription(
                    requestBody.get("Description"))) {
                return false;
            }
        }

        if (!S3ParameterValidatorUtil.isValidPolicyName(
                requestBody.get("PolicyName"))) {
            return false;
        }

        return S3ParameterValidatorUtil.isValidPolicyDocument(
                requestBody.get("PolicyDocument"));
    }
}
