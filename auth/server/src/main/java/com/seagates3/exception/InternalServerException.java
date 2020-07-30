package com.seagates3.exception;

import com.seagates3.response.ServerResponse;

public class InternalServerException extends Exception {

    ServerResponse serverResponse;

    public InternalServerException(ServerResponse serverResponse) {
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
