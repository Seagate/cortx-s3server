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
import com.seagates3.model.SAMLProvider;
import java.security.cert.Certificate;

public interface SAMLProviderDAO {

    /**
     * Get the SAML provider from issuer name.
     *
     * @param issuer Issuer name.
     * @return SAMLProvider
     * @throws DataAccessException
     */
    public SAMLProvider find(String issuer) throws DataAccessException;

    /*
     * Get user details from the database.
     */
    public SAMLProvider find(Account account, String name) throws DataAccessException;

    /*
     * Get the list of all the saml providers.
     */
    public SAMLProvider[] findAll(Account account) throws DataAccessException;

    /*
     * Return true if the key exists for the idp.
     */
    public Boolean keyExists(String accountId, String name, Certificate cert)
            throws DataAccessException;

    /*
     * Create a new entry for the saml provider in the data base.
     */
    public void save(SAMLProvider samlProvider) throws DataAccessException;

    /*
     * Delete the saml provider.
     */
    public void delete(SAMLProvider samlProvider) throws DataAccessException;

    /*
     * Modify saml provider metadata.
     */
    public void update(SAMLProvider samlProvider, String newSamlMetadata)
            throws DataAccessException;
}
