package com.seagates3.dao.ldap;

import java.util.List;
import java.util.Map;

import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Policy;

public
interface AuthStore {

 public
  void save(Map<String, Object> dataMap) throws DataAccessException;

 public
  Object find(String keyToFind) throws DataAccessException;

 public
  List<Policy> findAll(String keyToFind) throws DataAccessException;

 public
  void delete (String keyToRemove) throws DataAccessException;
}
