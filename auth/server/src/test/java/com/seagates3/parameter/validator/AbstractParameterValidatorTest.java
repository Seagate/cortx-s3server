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

import com.seagates3.parameter.validator.RoleParameterValidator;
import com.seagates3.parameter.validator.AbstractParameterValidator;
import java.util.Map;
import java.util.TreeMap;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import org.junit.Before;
import org.junit.Test;

public class AbstractParameterValidatorTest extends AbstractParameterValidator {
    RoleParameterValidator roleValidator;
    Map requestBody;

    public AbstractParameterValidatorTest() {
        roleValidator = new RoleParameterValidator();
    }

    @Before
    public void setUp() {
        requestBody = new TreeMap();
    }

    /**
     * Test AbstractValidator#isValidCreateParams.
     */
    @Test
    public void Create_EmptyInput_True() {
        assertTrue(isValidCreateParams(requestBody));
    }

    /**
     * Test AbstractValidator#isValidDeleteParams.
     */
    @Test
    public void Delete_EmptyInput_True() {
        assertTrue(isValidDeleteParams(requestBody));
    }


    /**
     * Test AbstractValidator#isValidListParams.
     * Case - Empty input.
     */
    @Test
    public void List_EmptyInput_True() {
        assertTrue(isValidListParams(requestBody));
    }

    /**
     * Test AbstractValidator#isValidListParams.
     * Case - Invalid Path Prefix.
     */
    @Test
    public void List_InvalidPathPrefix_False() {
        requestBody.put("PathPrefix", "segate/");
        assertFalse(isValidListParams(requestBody));
    }

    /**
     * Test AbstractValidator#isValidListParams.
     * Case - Invalid Max Items.
     */
    @Test
    public void List_InvalidMaxItems_False() {
        requestBody.put("PathPrefix", "/segate/");
        requestBody.put("MaxItems", "0");
        assertFalse(isValidListParams(requestBody));
    }

    /**
     * Test AbstractValidator#isValidListParams.
     * Case - Invalid Marker.
     */
    @Test
    public void List_InvalidMarker_False() {
        requestBody.put("PathPrefix", "/segate/");
        requestBody.put("MaxItems", "100");

        char c = "\u0100".toCharArray()[0];
        String marker = String.valueOf(c);

        requestBody.put("Marker", marker);
        assertFalse(isValidListParams(requestBody));
    }

    /**
     * Test AbstractValidator#isValidListParams.
     * Case - Valid inputs.
     */
    @Test
    public void List_ValidInputs_True() {
        requestBody.put("PathPrefix", "/segate/");
        requestBody.put("MaxItems", "100");
        requestBody.put("Marker", "abc");
        assertTrue(isValidListParams(requestBody));
    }

    /**
     * Test AbstractValidator#isValidUpdateParams.
     */
    @Test
    public void Update_EmptyInput_True() {
        assertTrue(isValidUpdateParams(requestBody));
    }
}
