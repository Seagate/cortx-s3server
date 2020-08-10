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

package com.seagates3.authserver;

import org.junit.Before;
import org.junit.Test;

import static org.junit.Assert.assertEquals;

public class ResourceMapTest {

    private ResourceMap resourceMap;

    @Before
    public void setUp() throws Exception {
        resourceMap = new ResourceMap("Account", "create");
    }

    @Test
    public void getControllerClassTest() {
        assertEquals("com.seagates3.controller.AccountController",
                resourceMap.getControllerClass());
    }

    @Test
    public void getParamValidatorClassTest() {
        assertEquals("com.seagates3.parameter.validator.AccountParameterValidator",
                resourceMap.getParamValidatorClass());
    }

    @Test
    public void getControllerActionTest() {
        assertEquals("create", resourceMap.getControllerAction());
    }

    @Test
    public void getParamValidatorMethodTest() {
        assertEquals("isValidCreateParams", resourceMap.getParamValidatorMethod());
    }
}