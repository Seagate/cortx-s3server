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

public class Role {

    private String rolePolicyDoc, path, name, arn, createDate, roleId;
    private Account account;

    /**
     * Return the ARN of the role.
     *
     * @return
     */
    public String getARN() {
        return arn;
    }

    /**
     * Return the role policy document.
     *
     * @return
     */
    public String getRolePolicyDoc() {
        return rolePolicyDoc;
    }

    /**
     * Return the role id.
     *
     * @return
     */
    public String getRoleId() {
        return roleId;
    }

    /**
     * Return the Path of the role.
     *
     * @return
     */
    public String getPath() {
        return path;
    }

    /**
     * Return the name of the role.
     *
     * @return
     */
    public String getName() {
        return name;
    }

    /**
     * Return the account.
     *
     * @return
     */
    public Account getAccount() {
        return account;
    }

    /**
     * Return the create date.
     *
     * @return
     */
    public String getCreateDate() {
        return createDate;
    }

    /**
     * Set the ARN of the role.
     *
     * @param arn
     */
    public void setARN(String arn) {
        this.arn = arn;
    }

    /**
     * Set the role policy document.
     *
     * @param rolePolicyDoc
     */
    public void setRolePolicyDoc(String rolePolicyDoc) {
        this.rolePolicyDoc = rolePolicyDoc;
    }

    /**
     * Set the role id .
     *
     * @param roleId
     */
    public void setRoleId(String roleId) {
        this.roleId = roleId;
    }

    /**
     * Set the path of the role.
     *
     * @param path
     */
    public void setPath(String path) {
        this.path = path;
    }

    /**
     * Set the name of the role.
     *
     * @param name
     */
    public void setName(String name) {
        this.name = name;
    }

    public void setAccount(Account account) {
        this.account = account;
    }

    /**
     * Set the create date.
     *
     * @param createDate
     */
    public void setCreateDate(String createDate) {
        this.createDate = createDate;
    }

    /**
     * Return true if the role exists.
     *
     * @return
     */
    public boolean exists() {
        return !(roleId == null);
    }
}
