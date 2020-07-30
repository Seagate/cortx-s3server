package com.seagates3.controller;

import com.seagates3.exception.DataAccessException;
import com.seagates3.model.Requestor;
import com.seagates3.response.ServerResponse;
import java.util.Map;

public abstract class AbstractController {

    final Requestor requestor;
    final Map<String, String> requestBody;

    public AbstractController(Requestor requestor, Map<String, String> requestBody) {
        this.requestor = requestor;
        this.requestBody = requestBody;
    }

    public ServerResponse create() throws DataAccessException {
        return null;
    }

    public ServerResponse delete() throws DataAccessException {
        return null;
    }

    public ServerResponse list() throws DataAccessException {
        return null;
    }

    public ServerResponse update() throws DataAccessException {
        return null;
    }

    public
     ServerResponse changepassword() throws DataAccessException { return null; }
}
