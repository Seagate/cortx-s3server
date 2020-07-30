package com.seagates3.model;

public class Account {

    private String id;
    private String name;
    private String canonicalId;
    private String email;
    private
     String password;
    private
     String profileCreateDate;
    private
     String pwdResetRequired;

    public String getId() {
        return id;
    }

    public String getName() {
        return name;
    }

    public String getCanonicalId() {
        return canonicalId;
    }

    public String getEmail() {
        return email;
    }

    public void setId(String id) {
        this.id = id;
    }

    public void setName(String name) {
        this.name = name;
    }

    public void setCanonicalId(String canonicalId) {
        this.canonicalId = canonicalId;
    }

    public void setEmail(String email) {
        this.email = email;
    }

    public Boolean exists() {
        return id != null;
    }

    public
     String getPassword() { return password; }

    public
     void setPassword(String pwd) { password = pwd; }

    public
     void setProfileCreateDate(String createDate) {
       profileCreateDate = createDate;
     }

    public
     String getProfileCreateDate() { return profileCreateDate; }

    public
     void setPwdResetRequired(String pwdReset) { pwdResetRequired = pwdReset; }

    public
     String getPwdResetRequired() { return pwdResetRequired; }
}
