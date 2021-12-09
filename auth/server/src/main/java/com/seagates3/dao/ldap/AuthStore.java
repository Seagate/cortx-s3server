package com.seagates3.dao.ldap;

import java.util.List;
import java.util.Map;

import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Policy;

public
interface AuthStore {

 public
  void save(Map<String, Object> dataMap,
            String prefix) throws DataAccessException;

 public
  Object find(String keyToFind, Object key, Object rootKey,
              String prefix) throws DataAccessException;

 public
  List<Policy> findAll(String keyToFind, Object key,
                       Map<String, Object> parameters, String prefix) throws DataAccessException;

 public
  void delete (String keyToRemove, Object key,
               String prefix) throws DataAccessException;

 public
  void attach(Map<String, Object> dataMap,
              String prefix) throws DataAccessException;

 public
  void detach(Map<String, Object> dataMap,
              String prefix) throws DataAccessException;

}
