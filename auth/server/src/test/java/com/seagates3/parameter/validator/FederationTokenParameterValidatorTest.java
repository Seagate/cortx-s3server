package com.seagates3.parameter.validator;

import com.seagates3.parameter.validator.FederationTokenParameterValidator;
import java.util.Map;
import java.util.TreeMap;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import org.junit.Before;
import org.junit.Test;

public class FederationTokenParameterValidatorTest {
    FederationTokenParameterValidator validator;
    Map requestBody;

    public FederationTokenParameterValidatorTest() {
        validator = new FederationTokenParameterValidator();
    }

    @Before
    public void setUp() {
        requestBody = new TreeMap();
    }

    /**
     * Test AssumeRoleWithSAML#isValidCreateParams.
     * Case - Name (also covers invalid Name).
     */
    @Test
    public void Create_PrincipalArnNull_False() {
        assertFalse(validator.isValidCreateParams(requestBody));
    }

    /**
     * Test AssumeRoleWithSAML#isValidCreateParams.
     * Case - Duration Seconds is invalid.
     */
    @Test
    public void Create_InvalidDurationSeconds_False() {
        requestBody.put("DurationSeconds", "800");
        assertFalse(validator.isValidCreateParams(requestBody));
    }

    /**
     * Test AssumeRoleWithSAML#isValidCreateParams.
     * Case - Invalid policy
     */
    @Test
    public void Create_InvalidPolicy_False() {
        requestBody.put("DurationSeconds", "900");
        requestBody.put("Policy", "");
        assertFalse(validator.isValidCreateParams(requestBody));
    }

    /**
     * Test AssumeRoleWithSAML#isValidCreateParams.
     * Case - Valid inputs.
     */
    @Test
    public void Create_ValidInputs_False() {
        requestBody.put("DurationSeconds", "900");
        requestBody.put("Policy", "test");
        requestBody.put("Name", "root");
        assertTrue(validator.isValidCreateParams(requestBody));
    }
}
