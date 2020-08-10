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

import com.seagates3.parameter.validator.AccessKeyParameterValidator;
import java.util.Map;
import java.util.TreeMap;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import org.junit.Before;
import org.junit.Test;

public class AccessKeyParameterValidatorTest {
    AccessKeyParameterValidator accessKeyValidator;
    Map requestBody;

    public AccessKeyParameterValidatorTest() {
        accessKeyValidator = new AccessKeyParameterValidator();
    }

    @Before
    public void setUp() {
        requestBody = new TreeMap();
    }

    /**
     * Test AccessKey#isValidCreateParams.
     * Case - User name is not provided.
     */
    @Test
    public void Create_UserNameNull_True() {
        assertTrue(accessKeyValidator.isValidCreateParams(requestBody));
    }

    /**
     * Test AccessKey#isValidCreateParams.
     * Case - User name is valid.
     */
    @Test
    public void Create_ValidUserName_True() {
        requestBody.put("UserName", "root");
        assertTrue(accessKeyValidator.isValidCreateParams(requestBody));
    }

    /**
     * Test AccessKey#isValidDeleteParams.
     * Case - Access key id is not provided.
     */
    @Test
    public void Delete_AccessKeyIdNull_False() {
        assertFalse(accessKeyValidator.isValidDeleteParams(requestBody));
    }

    /**
     * Test AccessKey#isValidDeleteParams.
     * Case - Access key id is valid.
     *   User name is empty.
     */
    @Test
    public void Delete_ValidAccessKeyIdEmptyUserName_True() {
        requestBody.put("AccessKeyId", "ABCDEFGHIJKLMN123456");
        assertTrue(accessKeyValidator.isValidDeleteParams(requestBody));
    }

    /**
     * Test AccessKey#isValidDeleteParams.
     * Case - Access key id is valid.
     *   User name is valid.
     */
    @Test
    public void Delete_ValidAccessKeyIdAndUserName_True() {
        requestBody.put("AccessKeyId", "ABCDEFGHIJKLMN123456");
        requestBody.put("UserName", "root");
        assertTrue(accessKeyValidator.isValidDeleteParams(requestBody));
    }

    /**
     * Test AccessKey#isValidDeleteParams.
     * Case - Access key id is invalid.
     */
    @Test
    public void Delete_InValidAccessKeyId_False() {
        requestBody.put("AccessKeyId", "ABCDEFGHIJKLMN 123456");
        assertFalse(accessKeyValidator.isValidDeleteParams(requestBody));
    }

    /**
     * Test AccessKey#isValidDeleteParams.
     * Case - User name id is invalid.
     */
    @Test
    public void Delete_InValidAccessKeyIdAndUserName_False() {
        requestBody.put("AccessKeyId", "ABCDEFGHIJKLMN 123456");
        requestBody.put("UserName", "root*^");
        assertFalse(accessKeyValidator.isValidDeleteParams(requestBody));
    }

    /**
     * Test AccessKey#isValidListParams.
     * Case - Empty input.
     */
    @Test
    public void List_EmptyInput_True() {
        assertTrue(accessKeyValidator.isValidListParams(requestBody));
    }

    /**
     * Test AccessKey#isValidListParams.
     * Case - Invalid user name.
     */
    @Test
    public void List_InvalidPathPrefix_False() {
        requestBody.put("UserName", "root$^");
        assertFalse(accessKeyValidator.isValidListParams(requestBody));
    }

    /**
     * Test AccessKey#isValidListParams.
     * Case - Invalid Max Items.
     */
    @Test
    public void List_InvalidMaxItems_False() {
        requestBody.put("UserName", "root");
        requestBody.put("MaxItems", "0");
        assertFalse(accessKeyValidator.isValidListParams(requestBody));
    }

    /**
     * Test AccessKey#isValidListParams.
     * Case - Invalid Marker.
     */
    @Test
    public void List_InvalidMarker_False() {
        requestBody.put("UserName", "root");
        requestBody.put("MaxItems", "100");

        char c = "\u0100".toCharArray()[0];
        String marker = String.valueOf(c);

        requestBody.put("Marker", marker);
        assertFalse(accessKeyValidator.isValidListParams(requestBody));
    }

    /**
     * Test AccessKey#isValidListParams.
     * Case - Valid inputs.
     */
    @Test
    public void List_ValidInputs_True() {
        requestBody.put("UserName", "root");
        requestBody.put("MaxItems", "100");
        requestBody.put("Marker", "abc");
        assertTrue(accessKeyValidator.isValidListParams(requestBody));
    }

    /**
     * Test AccessKey#isValidUpdateParams.
     * Case - status is invalid.
     */
    @Test
    public void Update_InvalidAccessKeyStatus_False() {
        requestBody.put("Status", "active");
        assertFalse(accessKeyValidator.isValidUpdateParams(requestBody));
    }

    /**
     * Test AccessKey#isValidUpdateParams.
     * Case - User name is invalid.
     */
    @Test
    public void Update_InvalidUserName_False() {
        requestBody.put("Status", "Active");
        requestBody.put("UserName", "root$^");
        assertFalse(accessKeyValidator.isValidUpdateParams(requestBody));
    }

    /**
     * Test AccessKey#isValidUpdateParams.
     * Case - Access key id is invalid.
     */
    @Test
    public void Update_InvalidAccessKeyId_False() {
        requestBody.put("Status", "Active");
        requestBody.put("UserName", "root");
        requestBody.put("AccessKeyId", "ABCDE");
        assertFalse(accessKeyValidator.isValidUpdateParams(requestBody));
    }

    /**
     * Test AccessKey#isValidUpdateParams.
     * Case - Valid inputs.
     */
    @Test
    public void Update_ValidInputs_True() {
        requestBody.put("Status", "Active");
        requestBody.put("UserName", "root");
        requestBody.put("AccessKeyId", "ABCDEFGHIJKLMN123456");
        assertTrue(accessKeyValidator.isValidUpdateParams(requestBody));
    }
}
