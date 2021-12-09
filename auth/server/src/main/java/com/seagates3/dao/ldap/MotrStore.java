package com.seagates3.dao.ldap;

import java.util.List;
import java.util.Map;

import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Policy;

public
class MotrStore implements AuthStore {

  @Override public void save(Map<String, Object> dataMap,
                             String prefix) throws DataAccessException {
    // TODO Auto-generated method stub
  }

  @Override public Object find(String keyToFind, Object obj, Object obj2,
                               String prefix) throws DataAccessException {
    // TODO Auto-generated method stub
    return null;
  }

  @Override public List<Policy> findAll(
      String keyToFind, Object obj, Map<String, Object> parameters,
      String prefix) throws DataAccessException {
    // TODO Auto-generated method stub
    return null;
  }

  @Override public void delete (String keyToRemove, Object obj,
                                String prefix) throws DataAccessException {
    // TODO Auto-generated method stub
  }

  @Override public void attach(Map<String, Object> dataMap,
                               String prefix) throws DataAccessException {
    // TODO Add motr store implementation for attach policy
  }

  @Override public void detach(Map<String, Object> dataMap,
                               String prefix) throws DataAccessException {
    // TODO Add motr store implementation for attach policy
  }
}
