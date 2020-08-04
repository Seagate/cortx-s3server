/*
 * Copyright (c) 2020 Seagate Technology LLC and/or its Affiliates
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * For any questions about this software or licensing,
 * please email opensource@seagate.com or cortx-questions@seagate.com.
 *
 */

package com.seagates3.model;

public class User {

    public enum UserType {

        IAM_USER("iamUser"),
        IAM_FED_USER("iamFedUser"),
        ROLE_USER("roleUser");

        private final String userType;

        private UserType(final String userType) {
            this.userType = userType;
        }

        @Override
        public String toString() {
            return userType;
        }
    }

    private String name;
    private String accountName;
    private String path;
    private String id;
    private String createDate;
    private String passwordLastUsed;
    private UserType userType;
    private
     String password;
    private
     String profileCreateDate;
    private
     String pwdResetRequired = "false";
    private
     String arn;

    /**
     * TODO - Remove RoleName. User Type is sufficient to identify a role user
     * from federated user or IAM user.
     */
    private String roleName;

    public String getName() {
        return name;
    }

    public String getAccountName() {
        return accountName;
    }

    public String getPath() {
        return path;
    }

    public
     String getPassword() { return password; }

    public
     void setPassword(String password) { this.password = password; }

    public String getId() {
        return id;
    }

    public String getCreateDate() {
        return createDate;
    }

    public String getPasswordLastUsed() {
        return passwordLastUsed;
    }

    public UserType getUserType() {
        return userType;
    }

    public String getRoleName() {
        return roleName;
    }

    public
     String getProfileCreateDate() { return profileCreateDate; }

    public
     String getPwdResetRequired() { return pwdResetRequired; }

    public void setName(String name) {
        this.name = name;
    }

    public void setAccountName(String accountName) {
        this.accountName = accountName;
    }

    public void setPath(String path) {
        this.path = path;
    }

    public void setId(String id) {
        this.id = id;
    }

    public void setUserType(UserType userType) {
        this.userType = userType;
    }

    public void setUserType(String userClass) {
        this.userType = getUserTypeConstant(userClass);
    }

    public void setCreateDate(String createDate) {
        this.createDate = createDate;
    }

    public void setPasswordLastUsed(String passwordLastUsed) {
        this.passwordLastUsed = passwordLastUsed;
    }

    public void setRoleName(String roleName) {
        this.roleName = roleName;
    }

    public Boolean exists() {
        return id != null;
    }

    private UserType getUserTypeConstant(String userClass) {
        if (userClass.compareToIgnoreCase("iamuser") == 0) {
            return UserType.IAM_USER;
        }

        if (userClass.compareToIgnoreCase("iamfeduser") == 0) {
            return UserType.IAM_FED_USER;
        }

        return UserType.ROLE_USER;
    }

   public
    void setProfileCreateDate(String pCreateDate) {
      profileCreateDate = pCreateDate;
    }

   public
    void setPwdResetRequired(String pwdReset) { pwdResetRequired = pwdReset; }

   public
    String getArn() { return arn; }

   public
    void setArn(String arn) { this.arn = arn; }
}
