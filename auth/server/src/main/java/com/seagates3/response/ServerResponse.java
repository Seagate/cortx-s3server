package com.seagates3.response;

import io.netty.handler.codec.http.HttpResponseStatus;

public class ServerResponse {

    String responseBody;
    HttpResponseStatus responseStatus;

    public ServerResponse() {}

    public ServerResponse(HttpResponseStatus responseStatus, String responseBody) {
        this.responseBody = responseBody;
        this.responseStatus = responseStatus;
    }

    public String getResponseBody() {
        return responseBody;
    }

    public void setResponseStatus(HttpResponseStatus status) {
        responseStatus = status;
    }

    public void setResponseBody(String body) {
        responseBody = body;
    }

    public HttpResponseStatus getResponseStatus() {
        return responseStatus;
    }
}