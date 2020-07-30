package com.seagates3.parameter.validator;

import java.util.Map;
import java.util.TreeMap;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import org.junit.Before;
import org.junit.Test;

public class AccountParameterValidatorTest {

    AccountParameterValidator accountValidator;
    Map requestBody;

    public AccountParameterValidatorTest() {
        accountValidator = new AccountParameterValidator();
    }

    @Before
    public void setUp() {
        requestBody = new TreeMap();
    }

    /**
     * Test Account#isValidCreateParams. Case - Account name is not provided.
     */
    @Test
    public void Create_AccountNameNull_False() {
        assertFalse(accountValidator.isValidCreateParams(requestBody));
    }

    /**
     * Test Account#isValidCreateParams. Case - Email is valid.
     */
    @Test
    public void Create_InvalidEmail_False() {
        requestBody.put("AccountName", "seagate");
        requestBody.put("Email", "testuser");
        assertFalse(accountValidator.isValidCreateParams(requestBody));
    }

    /**
     * Test Account#isValidCreateParams. Case - Email is valid.
     */
    @Test
    public void Create_InvalidAccountName_False() {
        requestBody.put("AccountName", "arj-123");
        requestBody.put("Email", "testuser");
        assertFalse(accountValidator.isValidCreateParams(requestBody));
    }

    /**
     * Test Account#isValidCreateParams. Case - Account name is valid.
     */
    @Test
    public void Create_ValidInputParams_True() {
        requestBody.put("AccountName", "seagate");
        requestBody.put("Email", "testuser@seagate.com");
        assertTrue(accountValidator.isValidCreateParams(requestBody));
    }
}
