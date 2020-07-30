package com.seagates3.util;

import org.junit.Test;

import static org.junit.Assert.*;

public class ARNUtilTest {

    @Test
    public void createARNTest() {
        String accountId = "testID";
        String resource = "testResource";
        String expected = "arn:seagate:iam::testID:testResource";

        String result = ARNUtil.createARN(accountId, resource);

        assertNotNull(result);
        assertEquals(expected, result);
    }

    @Test
    public void createARNTest_WithResourceType() {
        String accountId = "testID";
        String resource = "testResource";
        String resourceType = "testType";
        String expected = "arn:seagate:iam::testID:testType/testResource";

        String result = ARNUtil.createARN(accountId, resourceType, resource);

        assertNotNull(result);
        assertEquals(expected, result);
    }
}
