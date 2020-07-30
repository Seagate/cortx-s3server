package com.seagates3.exception;

import com.seagates3.response.ServerResponse;

public class InvalidUserException extends Exception {

    ServerResponse serverResponse;

    public InvalidUserException(ServerResponse serverResponse) {
        super(serverResponse.getResponseBody());
        this.serverResponse = serverResponse;
    }

    @Override
    public String getMessage() {
        return super.getMessage();
    }

    public ServerResponse getServerResponse() {
        return serverResponse;
    }

}
