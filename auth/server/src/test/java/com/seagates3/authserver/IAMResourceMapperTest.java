package com.seagates3.authserver;

import com.seagates3.exception.AuthResourceNotFoundException;
import org.junit.Before;
import org.junit.Test;

import java.io.UnsupportedEncodingException;

import static org.junit.Assert.*;

public class IAMResourceMapperTest {

    @Before
    public void setUp() throws Exception {
        IAMResourceMapper.init();
    }

    @Test
    public void getResourceMapTest() throws UnsupportedEncodingException,
            AuthResourceNotFoundException {
        ResourceMap resourceMap = IAMResourceMapper.getResourceMap("CreateAccount");

        assertEquals("com.seagates3.controller.AccountController",
                resourceMap.getControllerClass());
        assertEquals("com.seagates3.parameter.validator.AccountParameterValidator",
                resourceMap.getParamValidatorClass());
        assertEquals("create", resourceMap.getControllerAction());
        assertEquals("isValidCreateParams", resourceMap.getParamValidatorMethod());
    }

    @Test(expected = AuthResourceNotFoundException.class)
    public void getResourceMapTest_NullAction() throws AuthResourceNotFoundException {
        IAMResourceMapper.getResourceMap(null);
    }

    @Test(expected = AuthResourceNotFoundException.class)
    public void getResourceMapTest_InvalidAction() throws AuthResourceNotFoundException {
        IAMResourceMapper.getResourceMap("RandomAction");
    }
}