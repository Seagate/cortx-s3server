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
