package com.seagates3.dao;

import com.seagates3.exception.DataAccessException;
import com.seagates3.model.UserPolicy;

public
interface UserPolicyDAO {

 public
  void attach(UserPolicy userPolicy) throws DataAccessException;

 public
  void detach(UserPolicy userPolicy) throws DataAccessException;
}
