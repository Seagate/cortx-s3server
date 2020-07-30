package com.seagates3.dao;

public enum DAOProvider {
    LDAP("Ldap");

    private final String className;

    private DAOProvider(final String className) {
        this.className = className;
    }

    @Override
    public String toString() {
        return className;
    }
}
