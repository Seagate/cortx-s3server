package com.seagates3.dao.ldap;

import java.util.List;

import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Account;
import com.seagates3.model.Policy;

public
interface AuthStore {

 public
  void save(Policy policy) throws DataAccessException;

 public
  Policy find(String policyarn) throws DataAccessException;

 public
  Policy find(Account account, String name) throws DataAccessException;

 public
  List<Policy> findAll(Account account) throws DataAccessException;

 public
  void delete (Policy policy) throws DataAccessException;
}
