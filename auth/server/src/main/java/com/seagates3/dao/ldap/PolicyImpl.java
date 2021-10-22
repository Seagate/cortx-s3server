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

import java.util.List;

import com.seagates3.dao.PolicyDAO;
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Account;
import com.seagates3.model.Policy;

public class PolicyImpl implements PolicyDAO {

    /**
     * Find the policy.
     *
     * @param account
     * @param policyName
     * @return
     * @throws DataAccessException
     */
    @Override
    public Policy find(Account account, String policyName)
            throws DataAccessException {
      AuthStoreFactory factory = new AuthStoreFactory();
      AuthStore storeInstance = factory.createAuthStore();
      return storeInstance.find(account, policyName);
    }

    /**
     * Save the policy.
     *
     * @param policy
     * @throws DataAccessException
     */
    @Override
    public void save(Policy policy) throws DataAccessException {
      AuthStoreFactory factory = new AuthStoreFactory();
      AuthStore storeInstance = factory.createAuthStore();
      storeInstance.save(policy);
    }
    @Override public List<Policy> findAll(Account account)
        throws DataAccessException {
      AuthStoreFactory factory = new AuthStoreFactory();
      AuthStore storeInstance = factory.createAuthStore();
      return storeInstance.findAll(account);
    }
    @Override public void delete (Policy policy) throws DataAccessException {
      AuthStoreFactory factory = new AuthStoreFactory();
      AuthStore storeInstance = factory.createAuthStore();
      storeInstance.delete (policy);
    }
    @Override public Policy find(String arn) throws DataAccessException {
      AuthStoreFactory factory = new AuthStoreFactory();
      AuthStore storeInstance = factory.createAuthStore();
      return storeInstance.find(arn);
    }
}
