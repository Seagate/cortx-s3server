package com.seagates3.model;

public class AccessKey {

    public enum AccessKeyStatus {

        ACTIVE("Active"),
        INACTIVE("Inactive");

        private final String status;

        private AccessKeyStatus(final String text) {
            this.status = text;
        }

        @Override
        public String toString() {
            return status;
        }
    }

    private String userId;
    private String id;
    private String secretKey;
    private String createDate;
    private String token;
    private String expiry;
    private AccessKeyStatus status;

    public String getUserId() {
        return userId;
    }

    public String getId() {
        return id;
    }

    public String getSecretKey() {
        return secretKey;
    }

    public String getStatus() {
        return status.toString();
    }

    public String getCreateDate() {
        return createDate;
    }

    public String getToken() {
        return token;
    }

    public String getExpiry() {
        return expiry;
    }

    public void setUserId(String userId) {
        this.userId = userId;
    }

    public void setId(String accessKeyId) {
        this.id = accessKeyId;
    }

    public void setSecretKey(String secretKey) {
        this.secretKey = secretKey;
    }

    public void setStatus(AccessKeyStatus status) {
        this.status = status;
    }

    public void setCreateDate(String createDate) {
        this.createDate = createDate;
    }

    public void setToken(String token) {
        this.token = token;
    }

    public void setExpiry(String expiry) {
        this.expiry = expiry;
    }

    public Boolean isAccessKeyActive() {
        return status == AccessKeyStatus.ACTIVE;
    }

    public Boolean exists() {
        return userId != null;
    }
}
