package com.seagates3.fi;

import org.junit.Test;

import static org.junit.Assert.*;

public class FaultPointTest {

    private FaultPoint faultPoint;

    @Test
    public void isActiveTest_FAIL_ONCE() {
        faultPoint = new FaultPoint("LDAP_SEARCH", "FAIL_ONCE", 0);

        assertTrue(faultPoint.isActive());
        assertFalse(faultPoint.isActive());
        assertFalse(faultPoint.isActive());
    }

    @Test
    public void isActiveTest_FAIL_N_TIMES() {
        faultPoint = new FaultPoint("LDAP_SEARCH", "FAIL_N_TIMES", 2);

        assertTrue(faultPoint.isActive());
        assertTrue(faultPoint.isActive());
        assertFalse(faultPoint.isActive());
        assertFalse(faultPoint.isActive());
    }

    @Test
    public void isActiveTest_FAIL_0_TIMES() {
        faultPoint = new FaultPoint("LDAP_SEARCH", "FAIL_N_TIMES", 0);

        assertFalse(faultPoint.isActive());
        assertFalse(faultPoint.isActive());
    }

    @Test
    public void isActiveTest_SKIP_FIRST_N_TIMES() {
        faultPoint = new FaultPoint("LDAP_SEARCH", "SKIP_FIRST_N_TIMES", 2);

        assertFalse(faultPoint.isActive());
        assertFalse(faultPoint.isActive());
        assertTrue(faultPoint.isActive());
        assertTrue(faultPoint.isActive());
        assertTrue(faultPoint.isActive());
    }

    @Test
    public void isActiveTest_SKIP_FIRST_0_TIMES() {
        faultPoint = new FaultPoint("LDAP_SEARCH", "SKIP_FIRST_N_TIMES", 0);

        assertTrue(faultPoint.isActive());
        assertTrue(faultPoint.isActive());
        assertTrue(faultPoint.isActive());
    }

    @Test
    public void isActiveTest_FAIL_ALWAYS() {
        faultPoint = new FaultPoint("LDAP_SEARCH", "FAIL_ALWAYS", 0);

        assertTrue(faultPoint.isActive());
        assertTrue(faultPoint.isActive());
        assertTrue(faultPoint.isActive());
    }
}