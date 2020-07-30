package com.seagates3.dao;

import com.seagates3.exception.DataAccessException;
import com.seagates3.model.AccessKey;
import com.seagates3.model.Requestor;

public interface RequestorDAO {

    /*
     * Get the requestor details from the database.
     */
    public Requestor find(AccessKey accessKey) throws DataAccessException;
}
