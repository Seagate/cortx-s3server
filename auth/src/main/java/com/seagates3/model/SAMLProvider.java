/*
 * COPYRIGHT 2015 SEAGATE LLC
 *
 * THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
 * HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE TECHNOLOGY
 * LIMITED, ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
 * THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
 * BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
 * USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
 * EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.
 *
 * YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
 * THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
 * http://www.seagate.com/contact
 *
 * Original author:  Arjun Hariharan <arjun.hariharan@seagate.com>
 * Original creation date: 14-Oct-2015
 */

package com.seagates3.model;

public class SAMLProvider {

    private String accountName;
    private String name;
    private String samlMetadata;
    private String expiry;
    private KeyDescriptor[] keyDescriptors;
    private SAMLServiceEndpoints[] samlServices;
    private String[] nameIdFormats;
    private SAMLAssertionAttribute[] attributes;

    public String getAccountName() {
        return accountName;
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

    public KeyDescriptor[] getKeyDescriptors() {
        return keyDescriptors;
    }

    public SAMLServiceEndpoints[] getSAMLServices() {
        return samlServices;
    }

    public String[] getNameIdFormats() {
        return nameIdFormats;
    }

    public SAMLAssertionAttribute[] getAttributes() {
        return attributes;
    }

    public void setAccountName(String accountName) {
        this.accountName = accountName;
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

    public void setKeyDescriptors(KeyDescriptor[] keyDescriptors) {
        this.keyDescriptors = keyDescriptors;
    }

    public void setSAMLServices(SAMLServiceEndpoints[] samlServices) {
        this.samlServices = samlServices;
    }

    public void setNameIdFormats(String[] nameIdFormats) {
        this.nameIdFormats = nameIdFormats;
    }

    public void setAttributes(SAMLAssertionAttribute[] attributes) {
        this.attributes = attributes;
    }

    public Boolean exists() {
        return !(samlMetadata == null);
    }
}
