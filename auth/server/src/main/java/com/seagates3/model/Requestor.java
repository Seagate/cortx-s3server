package com.seagates3.model;

public class Requestor {

    private String id, name;
    private Account account;
    private AccessKey accessKey;

    /*
     * Return the requestor id.
     */
    public String getId() {
        return id;
    }

    /*
     * Name of the requestor.
     */
    public String getName() {
        return name;
    }

    public AccessKey getAccesskey() {
        return accessKey;
    }

    public Account getAccount() {
        return account;
    }

    public void setAccessKey(AccessKey accessKey) {
        this.accessKey = accessKey;
    }

    public void setAccount(Account account) {
        this.account = account;
    }

    public void setId(String id) {
        this.id = id;
    }

    public void setName(String name) {
        this.name = name;
    }

    /*
     * Return if the requestor exists.
     */
    public Boolean exists() {
        return id != null;
    }

    /*
     * Return true if the user is a federated user.
     */
    public Boolean isFederatedUser() {
        return accessKey.getToken() != null;
    }
}
