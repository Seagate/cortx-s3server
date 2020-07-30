package com.seagates3.fi;

import com.seagates3.exception.FaultPointException;
import org.junit.Test;

import static org.junit.Assert.*;

public class FaultPointsTest {

    @Test
    public void faultPointsTest() throws FaultPointException {
        // fiEnabledTest
        assertFalse(FaultPoints.fiEnabled());
        assertNull(FaultPoints.getInstance());
        FaultPoints.init();
        assertTrue(FaultPoints.fiEnabled());

        // getInstanceTest
        FaultPoints faultPoints = FaultPoints.getInstance();
        assertNotNull(faultPoints);

        // setFaultPointTest, isFaultPointSetTest, isFaultPointActiveTest
        assertFalse(faultPoints.isFaultPointSet("LDAP_SEARCH"));
        assertFalse(faultPoints.isFaultPointActive("LDAP_SEARCH"));
        assertFalse(faultPoints.isFaultPointActive("LDAP_SEARCH"));

        // setFaultPointTest_FailLocation_Null
        try {
            faultPoints.setFaultPoint(null, "FAIL_ONCE", 0);
            fail("Should throw IllegalArgumentException.");
        } catch (IllegalArgumentException e) {
            assertEquals("Invalid fault point", e.getMessage());
            assertFalse(faultPoints.isFaultPointSet("LDAP_SEARCH"));
            assertFalse(faultPoints.isFaultPointActive("LDAP_SEARCH"));
        }

        // setFaultPointTest_FailLocation_Empty
        try {
            faultPoints.setFaultPoint("", "FAIL_ONCE", 0);
            fail("Should throw IllegalArgumentException.");
        } catch (IllegalArgumentException e) {
            assertEquals("Invalid fault point", e.getMessage());
            assertFalse(faultPoints.isFaultPointSet("LDAP_SEARCH"));
            assertFalse(faultPoints.isFaultPointActive("LDAP_SEARCH"));
        }

        // setFaultPointTest_Invalid_Mode
        try {
            faultPoints.setFaultPoint("LDAP_SEARCH", "NO_SUCH_MODE", 0);
            fail("Should throw IllegalArgumentException.");
        } catch (IllegalArgumentException e) {
            assertEquals("No enum constant com.seagates3.fi.FaultPoint.Mode.NO_SUCH_MODE",
                    e.getMessage());
            assertFalse(faultPoints.isFaultPointSet("LDAP_SEARCH"));
            assertFalse(faultPoints.isFaultPointActive("LDAP_SEARCH"));
        }

        // setFaultPointTest
        faultPoints.setFaultPoint("LDAP_SEARCH", "FAIL_ONCE", 0);
        assertTrue(faultPoints.isFaultPointSet("LDAP_SEARCH"));
        assertTrue(faultPoints.isFaultPointActive("LDAP_SEARCH"));
        assertFalse(faultPoints.isFaultPointActive("LDAP_SEARCH"));

        // setFaultPointTest_FaultPointAlreadySet
        try {
            faultPoints.setFaultPoint("LDAP_SEARCH", "FAIL_ONCE", 0);
            fail("Should throw IllegalArgumentException.");
        } catch (FaultPointException e) {
            assertEquals("Fault point LDAP_SEARCH already set", e.getMessage());
        }

        // resetFaultPointTest_FailLocation_Null
        try {
            faultPoints.resetFaultPoint(null);
            fail("Should throw IllegalArgumentException.");
        } catch (IllegalArgumentException e) {
            assertEquals("Invalid fault point", e.getMessage());
            assertTrue(faultPoints.isFaultPointSet("LDAP_SEARCH"));
            assertFalse(faultPoints.isFaultPointActive("LDAP_SEARCH"));
        }

        // resetFaultPointTest_FailLocation_Empty
        try {
            faultPoints.resetFaultPoint("");
            fail("Should throw IllegalArgumentException.");
        } catch (IllegalArgumentException e) {
            assertEquals("Invalid fault point", e.getMessage());
            assertTrue(faultPoints.isFaultPointSet("LDAP_SEARCH"));
            assertFalse(faultPoints.isFaultPointActive("LDAP_SEARCH"));
        }

        // resetFaultPointTest
        faultPoints.resetFaultPoint("LDAP_SEARCH");
        assertFalse(faultPoints.isFaultPointSet("LDAP_SEARCH"));
        assertFalse(faultPoints.isFaultPointActive("LDAP_ADD"));

        // resetFaultPointTest_FailLocation_Empty
        try {
            faultPoints.resetFaultPoint("INVALID_FAIL_LOCATION");
            fail("Should throw IllegalArgumentException.");
        } catch (FaultPointException e) {
            assertEquals("Fault point INVALID_FAIL_LOCATION is not set", e.getMessage());
        }
    }
}