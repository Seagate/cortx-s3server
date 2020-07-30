package com.seagates3.parameter.validator;

import java.util.Map;

/**
 * Validate the input for Get Federation Token API.
 */
public class FederationTokenParameterValidator extends AbstractParameterValidator {

    /**
     * Validate the input parameters for get federation token request.
     * Name is required.
     * Duration seconds is optional.
     * Policy is optional.
     *
     * @param requestBody TreeMap of input parameters.
     * @return true if input is valid.
     */
    @Override
    public Boolean isValidCreateParams(Map<String, String> requestBody) {
        if (requestBody.containsKey("DurationSeconds")) {
            if (!STSParameterValidatorUtil.isValidDurationSeconds(requestBody.get("DurationSeconds"))) {
                return false;
            }
        }

        if (requestBody.containsKey("Policy")) {
            if (!STSParameterValidatorUtil.isValidPolicy(requestBody.get("Policy"))) {
                return false;
            }
        }

        return STSParameterValidatorUtil.isValidName(requestBody.get("Name"));
    }
}
