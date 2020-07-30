package com.seagates3.dao;

import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Account;
import com.seagates3.model.Policy;

public interface PolicyDAO {

    /**
     * Find Policy.
     *
     * @param account
     * @param name
     * @return
     * @throws DataAccessException
     */
    public Policy find(Account account, String name)
            throws DataAccessException;

    /**
     * Save the policy.
     *
     * @param policy
     * @throws DataAccessException
     */
    public void save(Policy policy) throws DataAccessException;
}
