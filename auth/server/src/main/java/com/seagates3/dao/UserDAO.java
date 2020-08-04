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
import com.seagates3.model.User;

public interface UserDAO {

    /*
     * Get user details from the database using user name.
     */
    public User find(String accountName, String userName) throws DataAccessException;

    /*
     * Get user details from the database using user id.
     */
    public User findByUserId(String accountName, String userId) throws DataAccessException;

    /*
     * Get the details of all the users with the given path prefix from an account.
     */
    public User[] findAll(String accountName, String pathPrefix) throws DataAccessException;

    /*
     * Delete the user.
     */
    public void delete(User user) throws DataAccessException;

    /*
     * Create a new entry for the user in the data base.
     */
    public void save(User user) throws DataAccessException;

    /*
     * Modify user details.
     */
    public void update(User user, String newUserName, String newPath) throws DataAccessException;
    public
     User findByUserId(String userId) throws DataAccessException;
    public
     User findByArn(String arn) throws DataAccessException;
}
