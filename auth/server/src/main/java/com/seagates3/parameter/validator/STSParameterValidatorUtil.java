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

import com.seagates3.util.BinaryUtil;

/**
 * Util class containing common validation rules for STS APIs.
 */
public class STSParameterValidatorUtil {

    final static int MAX_ASSUME_ROLE_DURATION = 3600;
    final static int MAX_DURATION = 129600;
    final static int MAX_NAME_LENGTH = 32;
    final static int MAX_POLICY_LENGTH = 2048;
    final static int MAX_SAML_ASSERTION_LENGTH = 50000;
    final static int MIN_DURATION = 900;
    final static int MIN_NAME_LENGTH = 2;
    final static int MIN_SAML_ASSERTION_LENGTH = 4;

    final static String NAME_PATTERN = "[\\w+=,.@-]*";
    final static String POLICY_PATTERN = "[\\u0009\\u000A\\u000D\\u0020-\\u00FF]+";

    /**
     * Validate the name.
     * Length of the name should be between 2 and 32 characters.
     * It should match the patten "[\\w+=,.@-]*".
     *
     * @param name user name to be validated.
     * @return true if name is valid.
     */
    public static Boolean isValidName(String name) {
        if(name == null) {
            return false;
        }

        if(!name.matches(NAME_PATTERN)) {
            return false;
        }

        return !(name.length() < MIN_NAME_LENGTH ||
                name.length() > MAX_NAME_LENGTH);
    }

    /**
     * Validate the duration seconds.
     * Range - 900 to 129600
     *
     * @param duration user name to be validated.
     * @return true if name is valid.
     */
    public static Boolean isValidDurationSeconds(String duration) {
        int durationSeconds;
        try {
            durationSeconds = Integer.parseInt(duration);
        } catch(NumberFormatException | AssertionError ex) {
            return false;
        }
        return !(durationSeconds > MAX_DURATION || durationSeconds < MIN_DURATION);
    }

    /**
     * Validate the user policy.
     * Length of the name should be between 1 and 2048 characters.
     * It should match the patten "[\\u0009\\u000A\\u000D\\u0020-\\u00FF]+".
     *
     * @param policy policy to be validated.
     * @return true if name is valid.
     */
    public static Boolean isValidPolicy(String policy) {
        if(policy == null) {
            return false;
        }

        if(!policy.matches(POLICY_PATTERN)) {
            return false;
        }

        return !(policy.length() < 1 || policy.length() > MAX_POLICY_LENGTH);
    }

    /**
     * Validate the duration for assume role with SAML.
     * Range - 900 to 3600
     *
     * @param duration duration to be validated.
     * @return true if name is valid.
     */
    public static Boolean isValidAssumeRoleDuration(String duration) {
        int durationSeconds;
        try {
            durationSeconds = Integer.parseInt(duration);
        } catch(NumberFormatException | AssertionError ex) {
            return false;
        }

        return !(durationSeconds < MIN_DURATION ||
                durationSeconds > MAX_ASSUME_ROLE_DURATION);
    }

    /**
     * Validate the SAML assertion (Assume role with SAML API).
     * Assertion should be base 64 encoded.
     * Length of the assertion should be between 4 and 50000 characters.
     *
     * @param assertion SAML assertion.
     * @return true if name is valid.
     */
    public static Boolean isValidSAMLAssertion(String assertion) {
        if(assertion == null) {
            return false;
        }

        if(!BinaryUtil.isBase64Encoded(assertion)) {
            return false;
        }

        return !(assertion.length() < MIN_SAML_ASSERTION_LENGTH ||
                assertion.length() > MAX_SAML_ASSERTION_LENGTH);
    }
}
