package com.seagates3.dao.ldap;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;

import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Account;
import com.seagates3.model.Policy;

public
class InMemoryStore implements AuthStore {

  Map<String, Policy> policyDetailsMap = new HashMap<>();

  @Override public void save(Policy policy) throws DataAccessException {
    String key = InMemoryStoreUtil.getKey(policy.getPolicyId(),
                                          policy.getAccount().getId());
    policyDetailsMap.put(key, policy);
  }

  @Override public Policy find(String policyarn) throws DataAccessException {
    if (policyarn != null) {
      String key = InMemoryStoreUtil.retrieveKeyFromArn(policyarn);
      return policyDetailsMap.get(key);
    }
    return null;
  }

  @Override public Policy find(Account account,
                               String name) throws DataAccessException {
    String key = InMemoryStoreUtil.getKey("", account.getId());
    for (Entry<String, Policy> entry : policyDetailsMap.entrySet()) {
      if (entry.getKey().contains(key) &&
          name.equals(entry.getValue().getName())) {
        return entry.getValue();
      }
    }
    return null;
  }

  @Override public List<Policy> findAll(Account account)
      throws DataAccessException {
    List<Policy> policyList = new ArrayList<>();
    String key = InMemoryStoreUtil.getKey("", account.getId());
    for (Entry<String, Policy> entry : policyDetailsMap.entrySet()) {
      if (entry.getKey().contains(key)) {
        policyList.add(entry.getValue());
      }
    }
    return policyList;
  }

  @Override public void delete (Policy policy) throws DataAccessException {
    String keyToBeRemoved = InMemoryStoreUtil.getKey(
        policy.getPolicyId(), policy.getAccount().getId());
    policyDetailsMap.remove(keyToBeRemoved);
  }
}
