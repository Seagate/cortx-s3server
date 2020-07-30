package com.seagates3.parameter.validator;

import com.seagates3.parameter.validator.UserParameterValidator;
import java.util.Map;
import java.util.TreeMap;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import org.junit.Before;
import org.junit.Test;

public class UserValidatorParameterTest {
    UserParameterValidator userValidator;
    Map requestBody;

    public UserValidatorParameterTest() {
        userValidator = new UserParameterValidator();
    }

    @Before
    public void setUp() {
        requestBody = new TreeMap();
    }

    /**
     * Test User#isValidCreateParams.
     * Case - User name is not provided.
     */
    @Test
    public void Create_UserNameNull_False() {
        assertFalse(userValidator.isValidCreateParams(requestBody));
    }

    /**
     * Test User#isValidCreateParams.
     * Case - User name is valid , Path is not provided.
     */
    @Test
    public void Create_ValidUserNameAndPathEmpty_True() {
        requestBody.put("UserName", "root");
        assertTrue(userValidator.isValidCreateParams(requestBody));
    }

    /**
     * Test User#isValidCreateParams.
     * Case - User name and path are valid.
     */
    @Test
    public void Create_ValidUserNameAndPath_True() {
        requestBody.put("UserName", "root");
        requestBody.put("Path", "/seagate/test/");
        assertTrue(userValidator.isValidCreateParams(requestBody));
    }

    /**
     * Test User#isValidCreateParams.
     * Case - User name is invalid.
     */
    @Test
    public void Create_InValidUserName_False() {
        requestBody.put("UserName", "root$^");
        assertFalse(userValidator.isValidCreateParams(requestBody));
    }

    /**
     * Test User#isValidCreateParams.
     * Case - User name is valid, path is invalid.
     */
    @Test
    public void Create_ValidUserNameAndInvalidPath_False() {
        requestBody.put("UserName", "root");
        requestBody.put("Path", "/seagate/test");
        assertFalse(userValidator.isValidCreateParams(requestBody));
    }

    /**
     * Test User#isValidDeleteParams.
     * Case - User name is not provided.
     */
    @Test
    public void Delete_UserNameNull_False() {
        assertFalse(userValidator.isValidDeleteParams(requestBody));
    }

    /**
     * Test User#isValidDeleteParams.
     * Case - User name is valid.
     */
    @Test
    public void Delete_ValidUserName_True() {
        requestBody.put("UserName", "root");
        assertTrue(userValidator.isValidDeleteParams(requestBody));
    }

    /**
     * Test User#isValidDeleteParams.
     * Case - User name is invalid.
     */
    @Test
    public void Delete_InValidUserName_False() {
        requestBody.put("UserName", "root$^");
        assertFalse(userValidator.isValidDeleteParams(requestBody));
    }

    /**
     * Test User#isValidUpdateParams.
     * Case - User name is invalid.
     */
    @Test
    public void Update_InvalidUserName_False() {
        requestBody.put("UserName", "root$^");
        assertFalse(userValidator.isValidUpdateParams(requestBody));
    }

    /**
     * Test User#isValidUpdateParams.
     * Case - New user name is invalid.
     */
    @Test
    public void Update_InvalidNewUserName_False() {
        requestBody.put("UserName", "root");
        requestBody.put("NewUserName", "root$^");
        assertFalse(userValidator.isValidUpdateParams(requestBody));
    }

    /**
     * Test User#isValidUpdateParams.
     * Case - User name is valid, new user name is valid and new path is invalid.
     */
    @Test
    public void Update_InvalidNewPath_False() {
        requestBody.put("UserName", "root");
        requestBody.put("NewUserName", "root123");
        requestBody.put("NewPath", "seagate/test");
        assertFalse(userValidator.isValidUpdateParams(requestBody));
    }

    /**
     * Test User#isValidUpdateParams.
     * Case - valid inputs.
     */
    @Test
    public void Update_ValidInputs_True() {
        requestBody.put("UserName", "root");
        requestBody.put("NewUserName", "root123");
        requestBody.put("NewPath", "/seagate/test/");
        assertTrue(userValidator.isValidUpdateParams(requestBody));
    }
}
