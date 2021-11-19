/*
 * Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For any questions about this software or licensing,
 * please email opensource@seagate.com or cortx-questions@seagate.com.
 *
 */

package com.seagates3.dao.ldap;

import com.novell.ldap.LDAPAttribute;
import com.novell.ldap.LDAPConnection;
import com.novell.ldap.LDAPEntry;
import com.novell.ldap.LDAPException;
import com.novell.ldap.LDAPSearchResults;
import com.seagates3.authserver.AuthServerConfig;
import com.seagates3.dao.AccountDAO;
import com.seagates3.dao.DAODispatcher;
import com.seagates3.dao.DAOResource;
import com.seagates3.exception.DataAccessException;
import com.seagates3.fi.FaultPoints;
import com.seagates3.model.AccessKey;
import com.seagates3.model.Account;
import com.seagates3.model.Requestor;
import java.util.Collections;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;
import static org.powermock.api.mockito.PowerMockito.doReturn;
import static org.powermock.api.mockito.PowerMockito.mockStatic;
import com.seagates3.dao.UserDAO;
import com.seagates3.model.User;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.ExpectedException;
import org.junit.runner.RunWith;
import org.mockito.Mockito;
import org.mockito.internal.matchers.apachecommons.ReflectionEquals;
import org.powermock.api.mockito.PowerMockito;
import org.powermock.core.classloader.annotations.PowerMockIgnore;
import org.powermock.core.classloader.annotations.PrepareForTest;
import org.powermock.modules.junit4.PowerMockRunner;

@RunWith(PowerMockRunner.class)
    @PrepareForTest({DAODispatcher.class, LdapConnectionManager.class,
                     FaultPoints.class,   AuthServerConfig.class,
                     LDAPUtils.class})
    @PowerMockIgnore({"javax.management.*"}) public class RequestorImplTest {

    private final String BASE_DN = "ou=accounts,dc=s3,dc=seagate,dc=com";
    private final String[] FIND_ATTRS = {"cn"};

    private final RequestorImpl requestorImpl;
    private final LDAPSearchResults ldapResults;
    private final LDAPEntry entry;
    private final LDAPAttribute commonNameAttr;
    private
     LDAPConnection ldapConnection;
    private
     final String LDAP_HOST = "127.0.0.1";
    private
     final int LDAP_PORT = 389;
    private
     final int socket_timeout = 1000;

    @Rule
    public final ExpectedException exception = ExpectedException.none();

    public RequestorImplTest() {
        requestorImpl = new RequestorImpl();
        ldapResults = Mockito.mock(LDAPSearchResults.class);
        entry = Mockito.mock(LDAPEntry.class);
        commonNameAttr = Mockito.mock(LDAPAttribute.class);
    }

    @Before
    public void setUp() throws Exception {
        PowerMockito.mockStatic(LDAPUtils.class);

        mockStatic(AuthServerConfig.class);
        ldapConnection = mock(LDAPConnection.class);
        PowerMockito.doReturn(LDAP_HOST)
            .when(AuthServerConfig.class, "getLdapHost");
        PowerMockito.doReturn(LDAP_PORT)
            .when(AuthServerConfig.class, "getLdapPort");

        when(ldapConnection.isConnected()).thenReturn(Boolean.TRUE);
        PowerMockito.mockStatic(LdapConnectionManager.class);
        doReturn(ldapConnection)
            .when(LdapConnectionManager.class, "getConnection");
        PowerMockito.whenNew(LDAPConnection.class)
            .withArguments(socket_timeout)
            .thenReturn(ldapConnection);
    }

    @Test
    public void Find_UserDoesNotExist_ReturnEmptyRequestor() throws Exception {
        AccessKey accessKey = new AccessKey();
        accessKey.setId("ak=AKIATEST");

        Requestor expectedRequestor = new Requestor();
        expectedRequestor.setAccessKey(accessKey);

        Requestor requestor = requestorImpl.find(accessKey);
        Assert.assertThat(expectedRequestor,
                new ReflectionEquals(requestor));
    }

    @Test
    public void Find_UserSearchFailed_ThrowException() throws Exception {
        AccessKey accessKey = new AccessKey();
        accessKey.setId("ak=AKIATEST");
        accessKey.setUserId("123");

        String filter = "s3userid=123";
        doThrow(new LDAPException()).when(ldapConnection).search(
            BASE_DN, 2, filter, FIND_ATTRS, false);

        exception.expect(DataAccessException.class);

        requestorImpl.find(accessKey);
    }

    @Test
    public void Find_UserSearchEmpty_ThrowException() throws Exception {
        AccessKey accessKey = new AccessKey();
        accessKey.setId("ak=AKIATEST");
        accessKey.setUserId("123");
        String filter = "s3userid=123";
        doReturn(ldapResults).when(ldapConnection).search(BASE_DN, 2, filter,
                                                          FIND_ATTRS, false);
        Mockito.doReturn(Boolean.FALSE).when(ldapResults).hasMore();
        exception.expect(DataAccessException.class);
        requestorImpl.find(accessKey);
    }

    @Test(expected = DataAccessException.class)
    public void Find_GetEntryShouldThrowException() throws Exception {
        AccessKey accessKey = new AccessKey();
        accessKey.setId("ak=AKIATEST");
        accessKey.setUserId("123");
        String filter = "s3userid=123";
        doReturn(ldapResults).when(ldapConnection).search(BASE_DN, 2, filter,
                                                          FIND_ATTRS, false);
        Mockito.doReturn(Boolean.TRUE).when(ldapResults).hasMore();
        Mockito.when(ldapResults.next()).thenThrow(new LDAPException());
        requestorImpl.find(accessKey);
    }

    @Test
    public void Find_UserFound_ReturnRequestor() throws Exception {
        AccessKey accessKey = new AccessKey();
        accessKey.setId("ak=AKIATEST");
        accessKey.setUserId("123");
        Account account = new Account();
        account.setId("12345");
        account.setName("s3test");
        User user = new User();
        user.setId("123");
        user.setPolicyIds(Collections.EMPTY_LIST);
        Requestor expectedRequestor = new Requestor();
        expectedRequestor.setAccessKey(accessKey);
        expectedRequestor.setId("123");
        expectedRequestor.setAccount(account);
        expectedRequestor.setName("s3testuser");
        String filter = "s3userid=123";
        doReturn(ldapResults).when(ldapConnection).search(BASE_DN, 2, filter,
                                                          FIND_ATTRS, false);
        Mockito.when(ldapResults.hasMore())
                .thenReturn(Boolean.TRUE)
                .thenReturn(Boolean.FALSE);
        Mockito.when(ldapResults.next()).thenReturn(entry);
        String dn = "s3userid=123,ou=users,o=s3test,ou=accounts,dc=s3,"
                + "dc=seagate,dc=com";
        Mockito.when(entry.getDN())
                .thenReturn(dn);
        Mockito.when(entry.getAttribute("cn")).thenReturn(commonNameAttr);
        Mockito.when(commonNameAttr.getStringValue()).thenReturn("s3testuser");
        AccountDAO accountDAO = Mockito.mock(AccountDAO.class);
        PowerMockito.mockStatic(DAODispatcher.class);
        PowerMockito.doReturn(accountDAO).when(DAODispatcher.class,
                "getResourceDAO", DAOResource.ACCOUNT
        );
        UserDAO userDAO = mock(UserDAO.class);
        doReturn(userDAO)
            .when(DAODispatcher.class, "getResourceDAO", DAOResource.USER);
        Mockito.doReturn(account).when(accountDAO).find("s3test");
        Mockito.doReturn(user).when(userDAO).find("s3test", "123");
        Requestor requestor = requestorImpl.find(accessKey);
        Assert.assertThat(expectedRequestor, new ReflectionEquals(requestor));
    }
}

