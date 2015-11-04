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
 * Original creation date: 31-Oct-2015
 */

package com.seagates3.model;

public class Role {
    private String rolePolicyDoc;
    private String path;
    private String name;
    private String accountName;

    /*
     * Store the date in the server response format i.e
     *   yyyy-MM-dd'T'HH:mm:ss.SSSZ
     */
    private String createDate;

    public String getRolePolicyDoc() {
        return rolePolicyDoc;
    }

    public String getPath() {
        return path;
    }

    public String getName() {
        return name;
    }

    public String getAccountName() {
        return accountName;
    }

    public String getCreateDate() {
        return createDate;
    }

    public void setRolePolicyDoc(String rolePolicyDoc) {
        this.rolePolicyDoc = rolePolicyDoc;
    }

    public void setPath(String path) {
        this.path = path;
    }

    public void setName(String name) {
        this.name = name;
    }

    public void setAccountName(String accountName) {
        this.accountName = accountName;
    }

    public void setCreateDate(String createDate) {
        this.createDate = createDate;
    }

    public boolean exists() {
        return !(rolePolicyDoc == null);
    }
}
