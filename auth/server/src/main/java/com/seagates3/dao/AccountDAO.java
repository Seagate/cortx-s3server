package com.seagates3.dao;

import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Account;

public interface AccountDAO {

    /**
     * TODO - Replace this method with an overloaded find method taking integer
     * account id as a parameter.
     *
     * @param accountID Account ID.
     * @return Account
     * @throws DataAccessException
     */
    public Account findByID(String accountID) throws DataAccessException;

    /* @param canonicalId of Account.
    *  @return Account
    *  @throws DataAccessException
    */
   public Account findByCanonicalID(String canonicalID) throws DataAccessException;
    /*
     * Get account details from the database.
     */
    public Account find(String name) throws DataAccessException;

    /*
     * Add a new entry for the account in the database.
     */
    public void save(Account account) throws DataAccessException;

    /*
     * Fetch all accounts from the database
     */
    public Account[] findAll() throws DataAccessException;

    /*
     * Delete account
     */
    public void delete(Account account) throws DataAccessException;

    /*
     * Delete ou under account
     */
    public void deleteOu(Account account, String ou) throws DataAccessException;

    public
     Account findByEmailAddress(String emailAddress) throws DataAccessException;
}
