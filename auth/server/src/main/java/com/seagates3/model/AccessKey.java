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
