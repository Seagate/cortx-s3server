package com.seagates3.dao.ldap;

import java.util.List;

import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Account;
import com.seagates3.model.Policy;

public
class MotrStore implements AuthStore {

  @Override public void save(Policy policy) throws DataAccessException {
    // TODO Auto-generated method stub
  }

  @Override public Policy find(String policyarn) throws DataAccessException {
    // TODO Auto-generated method stub
    return null;
  }

  @Override public List<Policy> findAll(Account account)
      throws DataAccessException {
    // TODO Auto-generated method stub
    return null;
  }

  @Override public void delete (Policy policy) throws DataAccessException {
    // TODO Auto-generated method stub
  }

  @Override public Policy find(Account account,
                               String name) throws DataAccessException {
    // TODO Auto-generated method stub
    return null;
  }
}
