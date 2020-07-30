package com.seagates3.dao;

import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Account;
import com.seagates3.model.Group;

public interface GroupDAO {

    /**
     * Find Policy.
     *
     * @param account
     * @param name
     * @return
     * @throws DataAccessException
     */
    public Group find(Account account, String groupName)
            throws DataAccessException;

    /**
     * Save the Group.
     *
     * @param group
     * @throws DataAccessException
     */
    public void save(Group group) throws DataAccessException;

     /**
      * Find group by path.
      *
      * @param path
      * @return
      * @throws DataAccessException
      */
    public
     Group findByPath(String path) throws DataAccessException;

     /**
      * Find group by path and account.
      *
      * @param account
      * @param path
      * @return
      * @throws DataAccessException
      */
    public
     Group findByPathAndAccount(Account account,
                                String path) throws DataAccessException;
}
