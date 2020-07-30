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