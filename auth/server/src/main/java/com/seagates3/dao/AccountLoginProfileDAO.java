package com.seagates3.dao;

import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Account;

public
interface AccountLoginProfileDAO {
 public
  void save(Account account) throws DataAccessException;
}
