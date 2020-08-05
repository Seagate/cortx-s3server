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

public interface AccountDAO {

    /**
     * TODO - Replace this method with an overloaded find method taking integer
     * account id as a parameter.
     *
     * @param accountID Account ID.
     * @return Account
     * @throws DataAccessException
     */
    public Account findByID(String accountID) throws DataAccessException;

    /* @param canonicalId of Account.
    *  @return Account
    *  @throws DataAccessException
    */
   public Account findByCanonicalID(String canonicalID) throws DataAccessException;
    /*
     * Get account details from the database.
     */
    public Account find(String name) throws DataAccessException;

    /*
     * Add a new entry for the account in the database.
     */
    public void save(Account account) throws DataAccessException;

    /*
     * Fetch all accounts from the database
     */
    public Account[] findAll() throws DataAccessException;

    /*
     * Delete account
     */
    public void delete(Account account) throws DataAccessException;

    /*
     * Delete ou under account
     */
    public void deleteOu(Account account, String ou) throws DataAccessException;

    public
     Account findByEmailAddress(String emailAddress) throws DataAccessException;
}
