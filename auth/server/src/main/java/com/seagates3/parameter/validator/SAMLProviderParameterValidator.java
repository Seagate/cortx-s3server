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
