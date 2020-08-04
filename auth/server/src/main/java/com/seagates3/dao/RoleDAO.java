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

package com.seagates3.dao;

import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Account;
import com.seagates3.model.Role;

public interface RoleDAO {

    /**
     * Get role from the database.
     *
     * @param account
     * @param roleName
     * @return
     * @throws com.seagates3.exception.DataAccessException
     */
    public Role find(Account account, String roleName)
            throws DataAccessException;

    /**
     * Get the details of all the roles with the given path prefix from an
     * account.
     *
     * @param account
     * @param pathPrefix
     * @return
     * @throws com.seagates3.exception.DataAccessException
     */
    public Role[] findAll(Account account, String pathPrefix)
            throws DataAccessException;

    /*
     * Delete the role.
     */
    public void delete(Role role) throws DataAccessException;

    /*
     * Create a new role.
     */
    public void save(Role role) throws DataAccessException;

}
