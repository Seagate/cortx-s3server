package com.seagates3.parameter.validator;

import java.util.Map;

/**
 * Validate the input for SAML provider APIs - Create, Delete and isValidUpdateParams.
 */
public class SAMLProviderParameterValidator extends AbstractParameterValidator {

    /**
     * Validate the input parameters for isValidCreateParams SAML Provider request. SAML
     * provider name is required. SAML metadata is required.
     *
     * @param requestBody TreeMap of input parameters.
     * @return true if input is valid.
     */
    @Override
    public Boolean isValidCreateParams(Map<String, String> requestBody) {
        if (!S3ParameterValidatorUtil.isValidSamlProviderName(requestBody.get("Name"))) {
            return false;
        }

        return S3ParameterValidatorUtil.isValidSAMLMetadata(
                requestBody.get("SAMLMetadataDocument"));
    }

    /**
     * Validate the input parameters for isValidDeleteParams user request. SAML Provider ARN
     * is required.
     *
     * @param requestBody TreeMap of input parameters.
     * @return true if input is valid.
     */
    @Override
    public Boolean isValidDeleteParams(Map<String, String> requestBody) {
        return S3ParameterValidatorUtil.isValidARN(requestBody.get("SAMLProviderArn"));
    }

    /**
     * Validate the input parameters for isValidListParams SAML providers request. This API
     * doesn't require an input.
     *
     * @param requestBody TreeMap of input parameters.
     * @return true.
     */
    @Override
    public Boolean isValidListParams(Map<String, String> requestBody) {
        return true;
    }

    /**
     * Validate the input parameters for isValidUpdateParams SAML provider request. SAML
     * provider ARN is required. SAML metadata is required.
     *
     * @param requestBody TreeMap of input parameters.
     * @return true.
     */
    @Override
    public Boolean isValidUpdateParams(Map<String, String> requestBody) {
        if (!S3ParameterValidatorUtil.isValidARN(requestBody.get("SAMLProviderArn"))) {
            return false;
        }

        return S3ParameterValidatorUtil.isValidSAMLMetadata(requestBody.get("SAMLMetadataDocument"));
    }
}
