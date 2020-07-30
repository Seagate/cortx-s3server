package com.seagates3.exception;

public class DataAccessException extends Exception {
    public DataAccessException(String ex) {
        super(ex);
    }

    @Override
    public String getMessage() {
        return super.getMessage();
    }
}
