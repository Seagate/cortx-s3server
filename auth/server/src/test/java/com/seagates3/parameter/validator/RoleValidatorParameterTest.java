package com.seagates3.parameter.validator;

import com.seagates3.parameter.validator.RoleParameterValidator;
import java.util.Map;
import java.util.TreeMap;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import org.junit.Before;
import org.junit.Test;

public class RoleValidatorParameterTest {
    RoleParameterValidator roleValidator;
    Map requestBody;

    final String assumeRolePolicyDoc = "{\n" +
"  \"Version\": \"2012-10-17\",\n" +
"  \"Statement\": {\n" +
"    \"Effect\": \"Allow\",\n" +
"    \"Principal\": {\"Service\": \"test\"},\n" +
"    \"Action\": \"sts:AssumeRole\"\n" +
"  }\n" +
"}";

    public RoleValidatorParameterTest() {
        roleValidator = new RoleParameterValidator();
    }

    @Before
    public void setUp() {
        requestBody = new TreeMap();
    }

    /**
     * Test Role#isValidCreateParams.
     * Case - Role name is not provided.
     */
    @Test
    public void Create_RoleNameNull_False() {
        assertFalse(roleValidator.isValidCreateParams(requestBody));
    }

    /**
     * Test Role#isValidCreateParams.
     * Case - Assume role policy document is not provided.
     */
    @Test
    public void Create_AssumeRolePolicyDocNull_False() {
        requestBody.put("RoleName", "admin");
        assertFalse(roleValidator.isValidCreateParams(requestBody));
    }

    /**
     * Test Role#isValidCreateParams.
     * Case - Role name and Assume role policy doc are valid, Path is not provided.
     */
    @Test
    public void Create_ValidRoleNamePolicyDocAndPathEmpty_True() {
        requestBody.put("RoleName", "admin");
        requestBody.put("AssumeRolePolicyDocument", assumeRolePolicyDoc);
        assertTrue(roleValidator.isValidCreateParams(requestBody));
    }

    /**
     * Test Role#isValidCreateParams.
     * Case - Role name and path are valid.
     */
    @Test
    public void Create_ValidInputs_True() {
        requestBody.put("RoleName", "admin");
        requestBody.put("AssumeRolePolicyDocument", assumeRolePolicyDoc);
        requestBody.put("Path", "/seagate/test/");
        assertTrue(roleValidator.isValidCreateParams(requestBody));
    }

    /**
     * Test Role#isValidCreateParams.
     * Case - Role name is invalid.
     */
    @Test
    public void Create_InvalidRoleName_False() {
        requestBody.put("RoleName", "admin$^");
        assertFalse(roleValidator.isValidCreateParams(requestBody));
    }

    /**
     * Test Role#isValidCreateParams.
     * Case - Role name is invalid.
     */
    @Test
    public void Create_InvalidPolidyDoc_False() {
        char c = "\u0100".toCharArray()[0];
        String policyDoc = String.valueOf(c);

        requestBody.put("RoleName", "admin");
        requestBody.put("AssumeRolePolicyDocument", policyDoc);
        assertFalse(roleValidator.isValidCreateParams(requestBody));
    }

    /**
     * Test Role#isValidCreateParams.
     * Case - Role name and role policy are valid, path is invalid.
     */
    @Test
    public void Create_InvalidPath_False() {
        requestBody.put("RoleName", "admin");
        requestBody.put("AssumeRolePolicyDocument", assumeRolePolicyDoc);
        requestBody.put("Path", "seagate/test/");
        assertFalse(roleValidator.isValidCreateParams(requestBody));
    }

    /**
     * Test Role#isValidDeleteParams.
     * Case - Role name is not provided.
     */
    @Test
    public void Delete_RoleNameNull_False() {
        assertFalse(roleValidator.isValidDeleteParams(requestBody));
    }

    /**
     * Test Role#isValidDeleteParams.
     * Case - Role name is valid.
     */
    @Test
    public void Delete_ValidRoleName_True() {
        requestBody.put("RoleName", "admin");
        assertTrue(roleValidator.isValidDeleteParams(requestBody));
    }

    /**
     * Test Role#isValidDeleteParams.
     * Case - Role name is invalid.
     */
    @Test
    public void Delete_InValidRoleName_False() {
        requestBody.put("RoleName", "admin$^");
        assertFalse(roleValidator.isValidDeleteParams(requestBody));
    }
}
