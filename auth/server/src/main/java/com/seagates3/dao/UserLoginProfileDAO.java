package com.seagates3.dao;

import com.seagates3.exception.DataAccessException;
import com.seagates3.model.User;

public
interface UserLoginProfileDAO {

 public
  void save(User user) throws DataAccessException;
}
