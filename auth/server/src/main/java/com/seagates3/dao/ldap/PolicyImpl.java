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

import java.util.HashMap;
import java.util.List;
import java.util.Map;

import com.seagates3.constants.APIRequestParameters;
import com.seagates3.dao.PolicyDAO;
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Account;
import com.seagates3.model.Policy;
import com.seagates3.policy.PolicyUtil;

public
class PolicyImpl implements PolicyDAO {

 private
  static final String POLICY_PREFIX = "Policy";

  /**
   * Find the policy.
   *
   * @param account
   * @param policyName
   * @return
   * @throws DataAccessException
   */
  @Override public Policy findByArn(String arn, Account account)
      throws DataAccessException {
    AuthStoreFactory factory = new AuthStoreFactory();
    AuthStore storeInstance = factory.createAuthStore(POLICY_PREFIX);
    if (arn != null) {
      String key = PolicyUtil.retrieveKeyFromArn(arn);
      return (Policy)storeInstance.find(key, arn, account, POLICY_PREFIX);
    }
    return null;
  }

  /**
   * Save the policy.
   *
   * @param policy
   * @throws DataAccessException
   */
  @Override public void save(Policy policy) throws DataAccessException {

    AuthStoreFactory factory = new AuthStoreFactory();
    AuthStore storeInstance = factory.createAuthStore(POLICY_PREFIX);
    String key =
        PolicyUtil.getKey(policy.getName(), policy.getAccount().getId());
    Map policyDetailsMap = new HashMap<>();
    policyDetailsMap.put(key, policy);
    storeInstance.save(policyDetailsMap, POLICY_PREFIX);
  }

  @Override public List<Policy> findAll(Account account,
                                        Map<String, Object> apiParameters)
      throws DataAccessException {
    AuthStoreFactory factory = new AuthStoreFactory();
    AuthStore storeInstance = factory.createAuthStore(POLICY_PREFIX);
    String key = PolicyUtil.getKey("", account.getId());
    return storeInstance.findAll(key, account, apiParameters, POLICY_PREFIX);
  }

  @Override public void delete (Policy policy) throws DataAccessException {
    AuthStoreFactory factory = new AuthStoreFactory();
    AuthStore storeInstance = factory.createAuthStore(POLICY_PREFIX);
    String keyToBeRemoved =
        PolicyUtil.getKey(policy.getName(), policy.getAccount().getId());
    storeInstance.delete (keyToBeRemoved, policy, POLICY_PREFIX);
  }

  @Override public Policy find(Account account,
                               String name) throws DataAccessException {
    AuthStoreFactory factory = new AuthStoreFactory();
    AuthStore storeInstance = factory.createAuthStore(POLICY_PREFIX);
    String key = PolicyUtil.getKey("", account.getId());
    Map<String, Object> apiParameters = new HashMap<>();
    apiParameters.put(APIRequestParameters.POLICY_NAME, name);
    List<Policy> policyList =
        storeInstance.findAll(key, account, apiParameters, POLICY_PREFIX);
    if (!policyList.isEmpty()) {
      return (Policy)policyList.get(0);
    }
    return null;
  }
}
