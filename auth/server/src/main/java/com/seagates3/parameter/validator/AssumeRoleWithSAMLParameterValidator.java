package com.seagates3.parameter.validator;

import java.util.Map;

/**
 * Validate the input for Assume Role with SAML API.
 */
public class AssumeRoleWithSAMLParameterValidator extends AbstractParameterValidator {

    /**
     * Validate the input parameters for assume role with SAML API. PrincipalArn
     * is required. RoleArn is required. SAMLAssertion is required.
     * DurationSeconds is optional. Policy is optional.
     *
     * @param requestBody TreeMap of input parameters.
     * @return true if input is valid.
     */
    @Override
    public Boolean isValidCreateParams(Map<String, String> requestBody) {
        if (requestBody.containsKey("DurationSeconds")) {
            if (!STSParameterValidatorUtil.isValidAssumeRoleDuration(
                    requestBody.get("DurationSeconds"))) {
                return false;
            }
        }

        if (requestBody.containsKey("Policy")) {
            if (!STSParameterValidatorUtil.isValidPolicy(requestBody.get("Policy"))) {
                return false;
            }
        }

        if (!S3ParameterValidatorUtil.isValidARN(requestBody.get("PrincipalArn"))) {
            return false;
        }

        if (!S3ParameterValidatorUtil.isValidARN(requestBody.get("RoleArn"))) {
            return false;
        }

        return STSParameterValidatorUtil.isValidSAMLAssertion(
                requestBody.get("SAMLAssertion"));
    }
}
