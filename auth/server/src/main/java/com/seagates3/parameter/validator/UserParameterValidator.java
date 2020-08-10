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
