package com.seagates3.exception;

public class BadRequestException extends Exception {

    public BadRequestException(String ex) {
        super(ex);
    }

    @Override
    public String getMessage() {
        return super.getMessage();
    }
}
