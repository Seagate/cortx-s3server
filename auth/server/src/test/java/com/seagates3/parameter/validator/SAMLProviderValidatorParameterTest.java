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

import com.seagates3.parameter.validator.SAMLProviderParameterValidator;
import java.util.Map;
import java.util.TreeMap;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import org.junit.Before;
import org.junit.Test;

public class SAMLProviderValidatorParameterTest {

    SAMLProviderParameterValidator validator;
    Map requestBody;

    public SAMLProviderValidatorParameterTest() {
        validator = new SAMLProviderParameterValidator();
    }

    @Before
    public void setUp() {
        requestBody = new TreeMap();
    }

    /**
     * Test SAMLProvider#isValidCreateParams. Case - Provider name is null (also tests
     * invalid name).
     */
    @Test
    public void Create_NameNull_False() {
        assertFalse(validator.isValidCreateParams(requestBody));
    }

    /**
     * Test SAMLProvider#isValidCreateParams. Case - SAML Metadata is null (also tests for
     * invalid metadata).
     */
    @Test
    public void Create_SAMLMetadataNull_False() {
        requestBody.put("Name", "Test");
        assertFalse(validator.isValidCreateParams(requestBody));
    }

    /**
     * Test SAMLProvider#isValidCreateParams. Case - Valid inputs.
     */
    @Test
    public void Create_ValidInputs_True() {
        requestBody.put("Name", "Test");
        String metadataDoc = new String(new char[1010]).replace('\0', 'a');
        requestBody.put("SAMLMetadataDocument", metadataDoc);

        assertTrue(validator.isValidCreateParams(requestBody));
    }

    /**
     * Test SAMLProvider#isValidDeleteParams. Case - SAML Provider ARN is not provided (also
     * tests for invalid ARN).
     */
    @Test
    public void Delete_SAMLProviderARNNull_False() {
        assertFalse(validator.isValidDeleteParams(requestBody));
    }

    /**
     * Test SAMLProvider#isValidDeleteParams. Case - Valid inputs
     */
    @Test
    public void Delete_ValidInputs_True() {
        requestBody.put("SAMLProviderArn", "arn:seagate:iam:::saml-provider/Test");
        assertTrue(validator.isValidDeleteParams(requestBody));
    }

    /**
     * Test SAMLProvider#isValidListParams. Case - Empty input.
     */
    @Test
    public void List_NoInput_True() {
        assertTrue(validator.isValidListParams(requestBody));
    }

    /**
     * Test SAMLProvider#isValidUpdateParams. Case - SAML Provider ARN is null (also tests
     * invalid name).
     */
    @Test
    public void Update_SAMLProviderARNNull_False() {
        assertFalse(validator.isValidUpdateParams(requestBody));
    }

    /**
     * Test SAMLProvider#isValidUpdateParams. Case - SAML Metadata is null (also tests for
     * invalid metadata).
     */
    @Test
    public void Update_SAMLMetadataNull_False() {
        requestBody.put("SAMLProviderArn", "arn:seagate:iam:::saml-provider/Test");
        assertFalse(validator.isValidUpdateParams(requestBody));
    }

    /**
     * Test SAMLProvider#isValidUpdateParams. Case - Valid inputs.
     */
    @Test
    public void Update_ValidInputs_True() {
        requestBody.put("SAMLProviderArn", "arn:seagate:iam:::saml-provider/Test");
        String metadataDoc = new String(new char[1010]).replace('\0', 'a');
        requestBody.put("SAMLMetadataDocument", metadataDoc);

        assertTrue(validator.isValidUpdateParams(requestBody));
    }
}
