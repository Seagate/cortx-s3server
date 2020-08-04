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
 * Abstract class for Validator classes.
 */
public abstract class AbstractParameterValidator {

    /**
     * Abstract implementation to check create request parameters.
     *
     * @param requestBody TreeMap of input parameters.
     * @return true
     */
    public Boolean isValidCreateParams(Map<String, String> requestBody) {
        return true;
    }

    /**
     * Abstract implementation to check delete request parameters.
     *
     * @param requestBody TreeMap of input parameters.
     * @return true
     */
    public Boolean isValidDeleteParams(Map<String, String> requestBody) {
        return true;
    }

    /**
     * Abstract implementation to check list request parameters. Validate the
     * input parameters for isValidListParams requests.
     *
     * List operation is used only on IAM resources i.e roles, user etc. Hence
     * default implementation is specific to IAM APIs.
     *
     * Path prefix is optional. Max Items is optional. Marker is optional.
     *
     * @param requestBody TreeMap of input parameters.
     * @return true if input is valid.
     */
    public Boolean isValidListParams(Map<String, String> requestBody) {
        if (requestBody.containsKey("PathPrefix")) {
            if (!S3ParameterValidatorUtil.isValidPathPrefix(
                    requestBody.get("PathPrefix"))) {
                return false;
            }
        }

        if (requestBody.containsKey("MaxItems")) {
            if (!S3ParameterValidatorUtil.isValidMaxItems(
                    requestBody.get("MaxItems"))) {
                return false;
            }
        }

        if (requestBody.containsKey("Marker")) {
            return S3ParameterValidatorUtil.isValidMarker(
                    requestBody.get("Marker"));
        }

        return true;
    }

    /**
     * Abstract implementation to check update request parameters.
     *
     * @param requestBody TreeMap of input parameters.
     * @return true
     */
    public Boolean isValidUpdateParams(Map<String, String> requestBody) {
        return true;
    }

    /**
     * Abstract implementation to check changePassword request parameters.
     *
     * @param requestBody TreeMap of input parameters.
     * @return true
     */
   public
    Boolean isValidChangepasswordParams(Map<String, String> requestBody) {
      return true;
    }
}
