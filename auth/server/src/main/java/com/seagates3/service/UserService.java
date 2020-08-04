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
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Account;
import com.seagates3.model.User;
import com.seagates3.util.KeyGenUtil;

public class UserService {

    /**
     * Search for the role user. If the user doesn't exist, create a new user
     *
     * @param account User Account.
     * @param roleName Role name.
     * @param roleSessionName Role session name.
     * @return User
     * @throws com.seagates3.exception.DataAccessException
     */
    public static User createRoleUser(Account account, String roleName,
            String roleSessionName) throws DataAccessException {
        String userRoleName = String.format("%s/%s", roleName, roleSessionName);

        UserDAO userDAO = (UserDAO) DAODispatcher.getResourceDAO(DAOResource.USER);
        User user = userDAO.find(account.getName(), userRoleName);

        if (!user.exists()) {
            user.setId(KeyGenUtil.createUserId());
            user.setRoleName(userRoleName);
            user.setUserType(User.UserType.ROLE_USER);
            userDAO.save(user);
        }

        return user;
    }

    /**
     * Search for the federation user. If the user doesn't exist, create a new
     * user
     *
     * TODO - Create a new class for federation user. Federation user should
     * contain a reference to the details of the permanent user whose
     * credentials were used at the time of creation. Federation user will
     * derive the policy settings of the permanent user.
     *
     * @param account User Name.
     * @param userName User name
     * @return User.
     * @throws com.seagates3.exception.DataAccessException
     */
    public static User createFederationUser(Account account, String userName)
            throws DataAccessException {
        UserDAO userDAO = (UserDAO) DAODispatcher.getResourceDAO(
                DAOResource.USER);
        User user = userDAO.find(account.getName(), userName);

        if (!user.exists()) {
            user.setId(KeyGenUtil.createUserId());
            user.setName(userName);
            user.setUserType(User.UserType.IAM_FED_USER);
            userDAO.save(user);

        }
        return user;
    }
}
