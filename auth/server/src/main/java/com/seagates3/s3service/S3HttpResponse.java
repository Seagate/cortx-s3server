package com.seagates3.s3service;

public class S3HttpResponse {

    int httpCode;
    String httpStatusMessage;

    public void setHttpCode(int code) {
        httpCode = code;
    }

    public void setHttpStatusMessage(String message) {
        httpStatusMessage = message;
    }

    public int getHttpCode() {
        return httpCode;
    }

    public String gettHttpStatusMessage() {
        return httpStatusMessage;
    }

}
