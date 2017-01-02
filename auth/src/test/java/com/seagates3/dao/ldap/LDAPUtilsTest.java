/*
 * COPYRIGHT 2016 SEAGATE LLC
 *
 * THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
 * HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE TECHNOLOGY
 * LIMITED, ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
 * THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
 * BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
 * USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
 * EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.
 *
 * YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
 * THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
 * http://www.seagate.com/contact
 *
 * Original author: Sushant Mane <sushant.mane@seagate.com>
 * Original creation date: 31-Dec-2016
 */
package com.seagates3.dao.ldap;

import com.novell.ldap.LDAPConnection;
import com.novell.ldap.LDAPEntry;
import com.novell.ldap.LDAPException;
import com.novell.ldap.LDAPModification;
import com.seagates3.fi.FaultPoints;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.powermock.api.mockito.PowerMockito;
import org.powermock.core.classloader.annotations.PrepareForTest;
import org.powermock.modules.junit4.PowerMockRunner;

import java.util.ArrayList;

import static junit.framework.TestCase.assertEquals;
import static junit.framework.TestCase.fail;
import static org.mockito.Matchers.any;
import static org.mockito.Matchers.anyString;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;
import static org.powermock.api.mockito.PowerMockito.doReturn;
import static org.powermock.api.mockito.PowerMockito.verifyStatic;

@RunWith(PowerMockRunner.class)
@PrepareForTest({LdapConnectionManager.class, FaultPoints.class})
public class LDAPUtilsTest {

    private LDAPConnection ldapConnection;

    @Before
    public void setUp() throws Exception {
        ldapConnection = mock(LDAPConnection.class);
        when(ldapConnection.isConnected()).thenReturn(Boolean.TRUE);
        PowerMockito.mockStatic(LdapConnectionManager.class);
        doReturn(ldapConnection).when(LdapConnectionManager.class, "getConnection");
    }

    @Test
    public void searchTest() throws LDAPException {
        // Arrange
        String baseDn = "dc=s3,dc=seagate,dc=com";
        String filter = "(cn=s3testuser)";
        String[] attrs = {"s3userid", "path", "rolename", "objectclass", "createtimestamp"};

        // Act
        LDAPUtils.search(baseDn, 2, filter, attrs);

        // Verify
        verify(ldapConnection).search(baseDn, 2, filter, attrs, false);

        verifyStatic();
        LdapConnectionManager.releaseConnection(ldapConnection);
    }

    @Test(expected = LDAPException.class)
    public void searchTest_ShouldThrowLDAPException() throws LDAPException {
        // Arrange
        String baseDn = "dc=s3,dc=seagate,dc=com";
        String filter = "(cn=s3testuser)";
        String[] attrs = {"s3userid", "path", "rolename", "objectclass", "createtimestamp"};
        doThrow(new LDAPException()).when(ldapConnection).search(baseDn, 2, filter, attrs, false);

        // Act
        LDAPUtils.search(baseDn, 2, filter, attrs);
    }

    @Test
    public void searchTest_ShouldReleaseConnectionIfExceptionOccurs() throws LDAPException {
        String baseDn = "dc=s3,dc=seagate,dc=com";
        String filter = "(cn=s3testuser)";
        String[] attrs = {"s3userid", "path", "rolename", "objectclass", "createtimestamp"};
        doThrow(new LDAPException(null, 82, null)).when(ldapConnection).search(baseDn, 2, filter, attrs, false);

        try {
            LDAPUtils.search(baseDn, 2, filter, attrs);
            fail("Expected LDAPException");
        } catch (LDAPException e) {
            assertEquals(LDAPException.LOCAL_ERROR, e.getResultCode());
        }

        verifyStatic();
        LdapConnectionManager.releaseConnection(ldapConnection);
    }

    @Test(expected = LDAPException.class)
    public void searchTest_ShouldThrowLDAPExceptionIfFiEnabled() throws Exception {
        // Arrange
        String baseDn = "dc=s3,dc=seagate,dc=com";
        String filter = "(cn=s3testuser)";
        String[] attrs = {"s3userid", "path", "rolename", "objectclass", "createtimestamp"};
        enableFaultInjection();

        // Act
        LDAPUtils.search(baseDn, 2, filter, attrs);
    }

    // Interaction Verification
    @Test
    public void addTest() throws LDAPException {
        // Arrange
        LDAPEntry ldapEntry = mock(LDAPEntry.class);

        // Act
        LDAPUtils.add(ldapEntry);

        // Verify
        verify(ldapConnection).add(ldapEntry);

        verifyStatic();
        LdapConnectionManager.releaseConnection(ldapConnection);
    }

    @Test(expected = LDAPException.class)
    public void addTest_ShouldThrowLDAPException() throws LDAPException {
        // Arrange
        LDAPEntry ldapEntry = mock(LDAPEntry.class);
        doThrow(new LDAPException()).when(ldapConnection).add(ldapEntry);

        // Act
        LDAPUtils.add(ldapEntry);
    }

    @Test
    public void addTest_ShouldReleaseConnectionIfExceptionOccurs() throws LDAPException {
        LDAPEntry ldapEntry = mock(LDAPEntry.class);
        doThrow(new LDAPException(null, 82, null)).when(ldapConnection).add(ldapEntry);

        try {
            LDAPUtils.add(ldapEntry);
            fail("Expected LDAPException");
        } catch (LDAPException e) {
            assertEquals(LDAPException.LOCAL_ERROR, e.getResultCode());
        }

        verifyStatic();
        LdapConnectionManager.releaseConnection(ldapConnection);
    }

    @Test(expected = LDAPException.class)
    public void addTest_ShouldThrowLDAPExceptionIfFiEnabled() throws Exception {
        // Arrange
        LDAPEntry ldapEntry = mock(LDAPEntry.class);
        enableFaultInjection();

        // Act
        LDAPUtils.add(ldapEntry);
    }

    // Interaction Verification
    @Test
    public void deleteTest() throws LDAPException {
        // Arrange
        String dn = "ak=AKIATEST,ou=accesskeys,dc=s3,dc=seagate,dc=com";

        // Act
        LDAPUtils.delete(dn);

        // Verify
        verify(ldapConnection).delete(dn);

        verifyStatic();
        LdapConnectionManager.releaseConnection(ldapConnection);
    }

    @Test(expected = LDAPException.class)
    public void deleteTest_ShouldThrowLDAPException() throws LDAPException {
        // Arrange
        String dn = "ak=AKIATEST,ou=accesskeys,dc=s3,dc=seagate,dc=com";
        doThrow(new LDAPException()).when(ldapConnection).delete(dn);

        // Act
        LDAPUtils.delete(dn);
    }

    @Test
    public void deleteTest_ShouldReleaseConnectionIfExceptionOccurs() throws LDAPException {
        String dn = "ak=AKIATEST,ou=accesskeys,dc=s3,dc=seagate,dc=com";
        doThrow(new LDAPException(null, 82, null)).when(ldapConnection).delete(dn);

        try {
            LDAPUtils.delete(dn);
            fail("Expected LDAPException");
        } catch (LDAPException e) {
            assertEquals(LDAPException.LOCAL_ERROR, e.getResultCode());
        }

        verifyStatic();
        LdapConnectionManager.releaseConnection(ldapConnection);
    }

    @Test(expected = LDAPException.class)
    public void deleteTest_ShouldThrowLDAPExceptionIfFiEnabled() throws Exception {
        // Arrange
        String dn = "ak=AKIATEST,ou=accesskeys,dc=s3,dc=seagate,dc=com";
        enableFaultInjection();

        // Act
        LDAPUtils.delete(dn);
    }

    // Interaction Verification
    @Test
    public void modifyTest() throws LDAPException {
        // Arrange
        String dn = "ak=AKIATEST,ou=accesskeys,dc=s3,dc=seagate,dc=com";
        LDAPModification ldapModification = mock(LDAPModification.class);

        // Act
        LDAPUtils.modify(dn, ldapModification);

        // Verify
        verify(ldapConnection).modify(dn, ldapModification);

        verifyStatic();
        LdapConnectionManager.releaseConnection(ldapConnection);
    }

    @Test(expected = LDAPException.class)
    public void modifyTest_ShouldThrowLDAPException() throws LDAPException {
        // Arrange
        String dn = "ak=AKIATEST,ou=accesskeys,dc=s3,dc=seagate,dc=com";
        LDAPModification ldapModification = mock(LDAPModification.class);
        doThrow(new LDAPException()).when(ldapConnection).modify(dn, ldapModification);

        // Act
        LDAPUtils.modify(dn, ldapModification);
    }

    @Test
    public void modifyTest_ShouldReleaseConnectionIfExceptionOccurs() throws LDAPException {
        String dn = "ak=AKIATEST,ou=accesskeys,dc=s3,dc=seagate,dc=com";
        LDAPModification ldapModification = mock(LDAPModification.class);
        doThrow(new LDAPException(null, 82, null)).when(ldapConnection).modify(dn, ldapModification);

        try {
            LDAPUtils.modify(dn, ldapModification);
            fail("Expected LDAPException");
        } catch (LDAPException e) {
            assertEquals(LDAPException.LOCAL_ERROR, e.getResultCode());
        }

        verifyStatic();
        LdapConnectionManager.releaseConnection(ldapConnection);
    }

    @Test(expected = LDAPException.class)
    public void modifyTest_ShouldThrowLDAPExceptionIfFiEnabled() throws Exception {
        // Arrange
        String dn = "ak=AKIATEST,ou=accesskeys,dc=s3,dc=seagate,dc=com";
        LDAPModification ldapModification = mock(LDAPModification.class);
        enableFaultInjection();

        // Act
        LDAPUtils.modify(dn, ldapModification);
    }

    // Interaction Verification
    @Test
    public void modifyTest_WithModList() throws LDAPException {
        // Arrange
        String dn = "ak=AKIATEST,ou=accesskeys,dc=s3,dc=seagate,dc=com";
        ArrayList modList = new ArrayList();

        // Act
        LDAPUtils.modify(dn, modList);

        // Verify
        verify(ldapConnection).modify(anyString(), any(LDAPModification[].class));

        verifyStatic();
        LdapConnectionManager.releaseConnection(ldapConnection);
    }

    @Test(expected = LDAPException.class)
    public void modifyTest_WithModList_ShouldThrowLDAPException() throws LDAPException {
        // Arrange
        String dn = "ak=AKIATEST,ou=accesskeys,dc=s3,dc=seagate,dc=com";
        ArrayList modList = new ArrayList();
        doThrow(new LDAPException()).when(ldapConnection).modify(anyString(), any(LDAPModification[].class));

        // Act
        LDAPUtils.modify(dn, modList);
    }

    @Test
    public void modifyTest_WithModList_ShouldReleaseConnectionIfExceptionOccurs() throws LDAPException {
        String dn = "ak=AKIATEST,ou=accesskeys,dc=s3,dc=seagate,dc=com";
        ArrayList modList = new ArrayList();
        doThrow(new LDAPException(null, 82, null)).when(ldapConnection).modify(anyString(),
                any(LDAPModification[].class));

        try {
            LDAPUtils.modify(dn, modList);
            fail("Expected LDAPException");
        } catch (LDAPException e) {
            assertEquals(LDAPException.LOCAL_ERROR, e.getResultCode());
        }

        verifyStatic();
        LdapConnectionManager.releaseConnection(ldapConnection);
    }

    @Test(expected = LDAPException.class)
    public void modifyTest_WithModList_ShouldThrowLDAPExceptionIfFiEnabled() throws Exception {
        // Arrange
        String dn = "ak=AKIATEST,ou=accesskeys,dc=s3,dc=seagate,dc=com";
        ArrayList modList = new ArrayList();
        enableFaultInjection();

        // Act
        LDAPUtils.modify(dn, modList);
    }

    private void enableFaultInjection() throws Exception {
        PowerMockito.mockStatic(FaultPoints.class);
        doReturn(Boolean.TRUE).when(FaultPoints.class, "fiEnabled");
        FaultPoints faultPoints = mock(FaultPoints.class);
        doReturn(faultPoints).when(FaultPoints.class, "getInstance");
        doReturn(Boolean.TRUE).when(faultPoints).isFaultPointActive(anyString());
    }
}
