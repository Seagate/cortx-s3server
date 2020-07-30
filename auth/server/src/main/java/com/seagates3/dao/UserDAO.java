package com.seagates3.dao;

import com.seagates3.exception.DataAccessException;
import com.seagates3.model.User;

public interface UserDAO {

    /*
     * Get user details from the database using user name.
     */
    public User find(String accountName, String userName) throws DataAccessException;

    /*
     * Get user details from the database using user id.
     */
    public User findByUserId(String accountName, String userId) throws DataAccessException;

    /*
     * Get the details of all the users with the given path prefix from an account.
     */
    public User[] findAll(String accountName, String pathPrefix) throws DataAccessException;

    /*
     * Delete the user.
     */
    public void delete(User user) throws DataAccessException;

    /*
     * Create a new entry for the user in the data base.
     */
    public void save(User user) throws DataAccessException;

    /*
     * Modify user details.
     */
    public void update(User user, String newUserName, String newPath) throws DataAccessException;
    public
     User findByUserId(String userId) throws DataAccessException;
    public
     User findByArn(String arn) throws DataAccessException;
}
