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

public class SAMLProvider {

    private Account account;
    private String name;
    private String samlMetadata;
    private String expiry;
    private String issuer;
    private String createDate;
    private SAMLMetadataTokens samlMetadataTokens;

    public Account getAccount() {
        return account;
    }

    public String getCreateDate() {
        return createDate;
    }

    public String getName() {
        return name;
    }

    public String getSamlMetadata() {
        return samlMetadata;
    }

    public String getExpiry() {
        return expiry;
    }

    public String getIssuer() {
        return issuer;
    }

    public SAMLMetadataTokens getSAMLMetadataTokens() {
        return samlMetadataTokens;
    }

    public void setAccount(Account account) {
        this.account = account;
    }

    public void setCreateDate(String createDate) {
        this.createDate = createDate;
    }

    public void setName(String name) {
        this.name = name;
    }

    public void setSamlMetadata(String samlMetadata) {
        this.samlMetadata = samlMetadata;
    }

    public void setExpiry(String expiry) {
        this.expiry = expiry;
    }

    public void setIssuer(String issuer) {
        this.issuer = issuer;
    }

    public void setSAMLMetadataTokens(SAMLMetadataTokens samlTokens) {
        this.samlMetadataTokens = samlTokens;
    }

    public Boolean exists() {
        return !(issuer == null);
    }
}
