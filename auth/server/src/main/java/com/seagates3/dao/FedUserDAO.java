package com.seagates3.dao;

import com.seagates3.exception.DataAccessException;
import com.seagates3.model.User;

public interface FedUserDAO {

    /*
     * Get user details from the database.
     */
    public User find(String accountName, String name) throws DataAccessException;

    /*
     * Create a new entry for the user in the data base.
     */
    public void save(User user) throws DataAccessException;
}
