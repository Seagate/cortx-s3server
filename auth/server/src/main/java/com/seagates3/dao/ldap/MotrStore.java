package com.seagates3.dao.ldap;

import java.util.List;
import java.util.Map;

import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Account;
import com.seagates3.model.Policy;

public
class MotrStore implements AuthStore {

  @Override public void save(Map<String, Object> dataMap)
      throws DataAccessException {
    // TODO Auto-generated method stub
  }

  @Override public Object find(String keyToFind) throws DataAccessException {
    // TODO Auto-generated method stub
    return null;
  }

  @Override public List<Policy> findAll(String keyToFind)
      throws DataAccessException {
    // TODO Auto-generated method stub
    return null;
  }

  @Override public void delete (String keyToRemove) throws DataAccessException {
    // TODO Auto-generated method stub
  }
}
