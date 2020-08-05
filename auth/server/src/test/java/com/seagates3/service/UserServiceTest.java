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

package com.seagates3.service;

import com.seagates3.dao.DAODispatcher;
import com.seagates3.dao.DAOResource;
import com.seagates3.dao.UserDAO;
import com.seagates3.model.Account;
import com.seagates3.model.User;
import com.seagates3.util.KeyGenUtil;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.powermock.api.mockito.mockpolicies.Slf4jMockPolicy;
import org.powermock.core.classloader.annotations.MockPolicy;
import org.powermock.core.classloader.annotations.PowerMockIgnore;
import org.powermock.core.classloader.annotations.PrepareForTest;
import org.powermock.modules.junit4.PowerMockRunner;

import static org.junit.Assert.*;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;
import static org.powermock.api.mockito.PowerMockito.mockStatic;

@RunWith(PowerMockRunner.class)
@PrepareForTest({RequestorService.class, DAODispatcher.class, KeyGenUtil.class})
@PowerMockIgnore("javax.management.*")
@MockPolicy(Slf4jMockPolicy.class)
public class UserServiceTest {

    private UserDAO userDAO;
    private Account account;
    private User user;

    private final String accountName = "sampleUser";
    private final String userName = "sampleUserName";
    private final String roleName = "sampleUserRoleName";
    private final String roleSessionName = "sampleRoleSessionName";
    private final String userID = "SampleUserId";

    @Before
    public void setUp() throws Exception {
        user = new User();

        userDAO = mock(UserDAO.class);
        account = mock(Account.class);
        mockStatic(DAODispatcher.class);
        mockStatic(KeyGenUtil.class);

        when(DAODispatcher.getResourceDAO(DAOResource.USER)).thenReturn(userDAO);
        when(account.getName()).thenReturn(accountName);
        when(KeyGenUtil.createUserId()).thenReturn(userID);
    }

    @Test
    public void createRoleUserTest() throws Exception {
        String userRoleName = String.format("%s/%s", roleName, roleSessionName);
        when(userDAO.find(accountName, userRoleName)).thenReturn(user);

        User result = UserService.createRoleUser(account, roleName, roleSessionName);

        assertEquals(userID, result.getId());
        assertEquals(userRoleName, result.getRoleName());
        assertEquals(User.UserType.ROLE_USER, result.getUserType());
    }

    @Test
    public void createFederationUserTest() throws Exception {
        when(userDAO.find(accountName, userName)).thenReturn(user);

        User result = UserService.createFederationUser(account, userName);

        assertEquals(userID, result.getId());
        assertEquals(userName, result.getName());
        assertEquals(User.UserType.IAM_FED_USER, result.getUserType());
    }
}