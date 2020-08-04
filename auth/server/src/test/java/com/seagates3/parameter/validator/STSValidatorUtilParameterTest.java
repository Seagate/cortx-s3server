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

import com.seagates3.parameter.validator.STSParameterValidatorUtil;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import org.junit.Test;

import com.seagates3.util.BinaryUtil;

public class STSValidatorUtilParameterTest {

    /**
     * Test STSValidatorUtil#isValidName.
     * case - Valid name.
     */
    @Test
    public void IsValidName_ValidName_True() {
        String name = "test.123@seagate.com+=,-";
        assertTrue(STSParameterValidatorUtil.isValidName(name));
    }

    /**
     * Test STSValidatorUtil#isValidName.
     * case - Name equals null
     */
    @Test
    public void IsValidName_NameNull_False() {
        String name = null;
        assertFalse(STSParameterValidatorUtil.isValidName(name));
    }

    /**
     * Test STSValidatorUtil#isValidName.
     * case - Invalid name pattern.
     */
    @Test
    public void IsValidName_InvalidNamePattern_False() {
        String name = "root*^";
        assertFalse(STSParameterValidatorUtil.isValidName(name));
    }

    /**
     * Test STSValidatorUtil#isValidName.
     * case - Length of the name is out of range.
     */
    @Test
    public void IsValidName_NameLengthOutOfRange_False() {
        String name = new String(new char[35]).replace('\0', 'a');
        assertFalse(STSParameterValidatorUtil.isValidName(name));

        name = "";
        assertFalse(STSParameterValidatorUtil.isValidName(name));
    }

    /**
     * Test STSValidatorUtil#isValidDurationSeconds.
     * case - Valid duration.
     */
    @Test
    public void IsValidDurationSeconds_ValidDuration_True() {
        String duration = "900";
        assertTrue(STSParameterValidatorUtil.isValidDurationSeconds(duration));

        duration = "1500";
        assertTrue(STSParameterValidatorUtil.isValidDurationSeconds(duration));

        duration = "129600";
        assertTrue(STSParameterValidatorUtil.isValidDurationSeconds(duration));
    }

    /**
     * Test STSValidatorUtil#isValidDurationSeconds.
     * case - Duration out of range.
     */
    @Test
    public void IsValidDurationSeconds_DurationOutOfRange_False() {
        String duration = "850";
        assertFalse(STSParameterValidatorUtil.isValidDurationSeconds(duration));

        duration = "229600";
        assertFalse(STSParameterValidatorUtil.isValidDurationSeconds(duration));
    }

    /**
     * Test STSValidatorUtil#isValidDurationSeconds.
     * case - Duration seconds is not an integer.
     */
    @Test
    public void IsValidDurationSeconds_InvalidDuration_False() {
        String duration = "abc";
        assertFalse(STSParameterValidatorUtil.isValidDurationSeconds(duration));

        duration = null;
        assertFalse(STSParameterValidatorUtil.isValidAssumeRoleDuration(duration));
    }

    /**
     * Test STSValidatorUtil#isValidPolicy.
     * case - Valid policy.
     */
    @Test
    public void IsValidPolicy_ValidName_True() {
        String policy = "{\n"
                + "  \"Version\":\"2012-10-17\",\n"
                + "  \"Statement\":[\n"
                + "    {\n"
                + "      \"Sid\":\"AddCannedAcl\",\n"
                + "      \"Effect\":\"Allow\",\n"
                + "      \"Principal\": {\"AWS\": [\"arn:aws:iam::111122223333:root\",\"arn:aws:iam::444455556666:root\"]},\n"
                + "      \"Action\":[\"s3:PutObject\",\"s3:PutObjectAcl\"],\n"
                + "      \"Resource\":[\"arn:aws:s3:::examplebucket/*\"],\n"
                + "      \"Condition\":{\"StringEquals\":{\"s3:x-amz-acl\":[\"public-read\"]}}\n"
                + "    }\n"
                + "  ]\n"
                + "}";
        assertTrue(STSParameterValidatorUtil.isValidPolicy(policy));
    }

    /**
     * Test STSValidatorUtil#isValidPolicy.
     * case - Policy is null.
     */
    @Test
    public void IsValidPolicy_NameNull_False() {
        String policy = null;
        assertFalse(STSParameterValidatorUtil.isValidPolicy(policy));
    }

    /**
     * Test STSValidatorUtil#isValidPolicy.
     * case - Invalid characters in policy.
     */
    @Test
    public void IsValidPolicy_InvalidNamePattern_False() {
        char c = "\u0100".toCharArray()[0];
        String policy = String.valueOf(c);
        assertFalse(STSParameterValidatorUtil.isValidPolicy(policy));
    }

    /**
     * Test STSValidatorUtil#isValidPolicy.
     * case - policy length of the name is out of range.
     */
    @Test
    public void IsValidPolicy_NameLengthOutOfRange_False() {
        String policy = new String(new char[2050]).replace('\0', 'a');
        assertFalse(STSParameterValidatorUtil.isValidPolicy(policy));

        policy = "";
        assertFalse(STSParameterValidatorUtil.isValidPolicy(policy));
    }

    /**
     * Test STSValidatorUtil#isValidAssumeRoleDuration.
     * case - Valid duration.
     */
    @Test
    public void IsValidAssumeRoleDuration_ValidDuration_True() {
        String duration = "900";
        assertTrue(STSParameterValidatorUtil.isValidAssumeRoleDuration(duration));

        duration = "1500";
        assertTrue(STSParameterValidatorUtil.isValidAssumeRoleDuration(duration));

        duration = "3600";
        assertTrue(STSParameterValidatorUtil.isValidAssumeRoleDuration(duration));
    }

    /**
     * Test STSValidatorUtil#isValidAssumeRoleDuration.
     * case - Duration out of range.
     */
    @Test
    public void IsValidAssumeRoleDuration_DurationOutOfRange_False() {
        String duration = "850";
        assertFalse(STSParameterValidatorUtil.isValidAssumeRoleDuration(duration));

        duration = "4000";
        assertFalse(STSParameterValidatorUtil.isValidAssumeRoleDuration(duration));
    }

    /**
     * Test STSValidatorUtil#isValidAssumeRoleDuration.
     * case - Duration seconds is not an integer.
     */
    @Test
    public void IsValidAssumeRoleDuration_InvalidDuration_False() {
        String duration = "abc";
        assertFalse(STSParameterValidatorUtil.isValidAssumeRoleDuration(duration));

        duration = null;
        assertFalse(STSParameterValidatorUtil.isValidAssumeRoleDuration(duration));
    }

    /**
     * Test STSValidatorUtil#isValidSAMLAssertion.
     * case - Valid assertion.
     */
    @Test
    public void IsValidSAMLAssertion_ValidAssertion_True() {
        String assertion = "c2VhZ2F0ZSB0ZXN0IGFzc2VydGlvbg==";
        assertTrue(STSParameterValidatorUtil.isValidSAMLAssertion(assertion));
    }

    /**
     * Test STSValidatorUtil#isValidSAMLAssertion.
     * case - SAML Assertion is not encoded.
     */
    @Test
    public void IsValidSAMLAssertion_AssertionNotEncoded_False() {
        String assertion = "Assertion is not encoded.";
        assertFalse(STSParameterValidatorUtil.isValidSAMLAssertion(assertion));
    }

    /**
     * Test STSValidatorUtil#isValidSAMLAssertion.
     * case - SAML Assertion is null.
     */
    @Test
    public void IsValidSAMLAssertion_AssertionNull_False() {
        String assertion = null;
        assertFalse(STSParameterValidatorUtil.isValidSAMLAssertion(assertion));
    }

    /**
     * Test STSValidatorUtil#isValidSAMLAssertion.
     * case - Length of SAML Assertion is our of range.
     */
    @Test
    public void IsValidSAMLAssertion_AssertionLengthOutOfRange_False() {
        String assertion = new String(new char[50000]).replace('\0', 'a');
        String assertionBase64 = BinaryUtil.encodeToBase64String(assertion);
        assertFalse(STSParameterValidatorUtil.isValidSAMLAssertion(assertionBase64));

        assertion = "";
        assertFalse(STSParameterValidatorUtil.isValidSAMLAssertion(assertion));
    }
}
