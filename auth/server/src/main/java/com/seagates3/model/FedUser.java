package com.seagates3.model;

public class FedUser {
    private String name;
    private String accountName;
    private String id;
    private String createDate;

    public String getName() {
        return name;
    }

    public String getAccountName() {
        return accountName;
    }

    public String getId() {
        return id;
    }

    public String getCreateDate() {
        return createDate;
    }

    public void setName(String name) {
        this.name = name;
    }

    public void setAccountName(String accountName) {
        this.accountName = accountName;
    }

    public void setId(String id) {
        this.id = id;
    }

    public void setCreateDate(String createDate) {
        this.createDate = createDate;
    }

    public Boolean exists() {
        return id != null;
    }
}
