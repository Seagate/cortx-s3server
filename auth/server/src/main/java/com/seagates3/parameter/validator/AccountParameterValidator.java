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
 * Validate the input for Account API - Create.
 */
public class AccountParameterValidator extends AbstractParameterValidator {
    /*
     * TODO
     * Find out maximum length of account name.
     */

    /**
     * Validate the input parameters for isValidCreateParams account request. Account name is
     * required.
     *
     * @param requestBody TreeMap of input parameters.
     * @return true if input is valid.
     */
    @Override
    public Boolean isValidCreateParams(Map<String, String> requestBody) {
        Boolean isValidParameters = S3ParameterValidatorUtil.isValidEmail(
                                                    requestBody.get("Email"));

        if (isValidParameters) {
            isValidParameters = S3ParameterValidatorUtil.isValidName(
                                                requestBody.get("AccountName"));
        }

        if (isValidParameters) {
        	isValidParameters = isAccessKeySecretKeyCombinationValid(requestBody.get("AccessKey"), requestBody.get("SecretKey"));
        }

        return isValidParameters;
    }
    
    /**
     * Validate access key and secret key combination. Either both valid values
     * of access key and secret key should be provided or not provided at all.
     *
     * @param accessKey accessKey provided.
     * @param secretKey secretKey provided.
     * @return true if both valid access key and secret key are provided
     * or not provided at all.
     */
    private Boolean isAccessKeySecretKeyCombinationValid(String accessKey, String secretKey) {
    	Boolean isValid = true;
        if (accessKey != null || secretKey != null) {
        	isValid = (
               S3ParameterValidatorUtil.isValidCustomAccessKey(accessKey) &&
               S3ParameterValidatorUtil.isValidCustomSecretKey(secretKey)
            );
        }
        
        return isValid;
    }

    /**
     * Validate the input parameters for isValidDeleteParams account request. Account name is
     * required.
     *
     * @param requestBody TreeMap of input parameters.
     * @return true if input is valid.
     */
    @Override
    public Boolean isValidDeleteParams(Map<String, String> requestBody) {
        return S3ParameterValidatorUtil.isValidName(
                requestBody.get("AccountName"));
    }

    /**
     * Validate the input parameters for isValidResetAccountAccessKeyParams
     *  account request. Account name is required.
     *
     * @param requestBody TreeMap of input parameters.
     * @return true if input is valid.
     */

    public Boolean isValidResetAccountAccessKeyParams(Map<String,
                                      String> requestBody) {
        return S3ParameterValidatorUtil.isValidName(
                requestBody.get("AccountName"));
    }

}
