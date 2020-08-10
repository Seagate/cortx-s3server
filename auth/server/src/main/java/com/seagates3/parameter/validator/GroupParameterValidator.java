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
 * Validate the input for group APIs.
 */
public class GroupParameterValidator extends AbstractParameterValidator {

    /**
     * Validate the input for isValidCreateParams group request. Group name is required. Path
     * is optional.
     *
     * @param requestBody
     * @return
     */
    @Override
    public Boolean isValidCreateParams(Map<String, String> requestBody) {
        if (requestBody.containsKey("Path")) {
            if (!S3ParameterValidatorUtil.isValidPath(
                    requestBody.get("Path"))) {
                return false;
            }
        }

        return S3ParameterValidatorUtil.isValidGroupName(
                requestBody.get("GroupName"));
    }
}
