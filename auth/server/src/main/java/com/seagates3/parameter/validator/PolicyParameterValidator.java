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
