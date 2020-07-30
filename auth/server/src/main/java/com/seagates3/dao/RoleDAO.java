package com.seagates3.dao;

import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Account;
import com.seagates3.model.Role;

public interface RoleDAO {

    /**
     * Get role from the database.
     *
     * @param account
     * @param roleName
     * @return
     * @throws com.seagates3.exception.DataAccessException
     */
    public Role find(Account account, String roleName)
            throws DataAccessException;

    /**
     * Get the details of all the roles with the given path prefix from an
     * account.
     *
     * @param account
     * @param pathPrefix
     * @return
     * @throws com.seagates3.exception.DataAccessException
     */
    public Role[] findAll(Account account, String pathPrefix)
            throws DataAccessException;

    /*
     * Delete the role.
     */
    public void delete(Role role) throws DataAccessException;

    /*
     * Create a new role.
     */
    public void save(Role role) throws DataAccessException;

}
