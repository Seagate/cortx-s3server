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
import com.seagates3.model.AccessKey;
import com.seagates3.model.User;

public interface AccessKeyDAO {

    /*
     * Get the access key details from the database.
     */
    public AccessKey find(String accessKeyId) throws DataAccessException;

    public AccessKey findFromToken(String accessKeyId) throws DataAccessException;

    /*
     * Get all the access keys belonging to the user.
     */
    public AccessKey[] findAll(User user) throws DataAccessException;

    /*
     * Return true if the user has an access key.
     */
    public Boolean hasAccessKeys(String userId) throws DataAccessException;

    /*
     * Return the no of access keys which a user has.
     * AWS allows a maximum of 2 access keys per user.
     */
    public int getCount(String userId) throws DataAccessException;

    /*
     * Delete the access key.
     */
    public void delete(AccessKey accessKey) throws DataAccessException;

    /*
     * Create a new entry in the data base for the access key.
     */
    public void save(AccessKey accessKey) throws DataAccessException;

    /*
     * modify the access key details.
     */
    public void update(AccessKey accessKey, String newStatus) throws DataAccessException;

     /*
      * Get all permanent access keys belonging to the user.
      */
    public
     AccessKey[] findAllPermanent(User user) throws DataAccessException;
     /*
      * Deletes all expired and inactive keys
      */
    public
     void deleteExpiredKeys(User user) throws DataAccessException;
}

