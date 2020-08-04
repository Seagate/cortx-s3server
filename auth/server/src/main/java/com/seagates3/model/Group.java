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

public class Group {

    private String path, name, arn, createDate, groupId;
    private Account account;
    public
     static final String AllUsersURI =
         "http://acs.amazonaws.com/groups/global/AllUsers";
    public
     static final String AuthenticatedUsersURI =
         "http://acs.amazonaws.com/groups/global/AuthenticatedUsers";
    public
     static final String LogDeliveryURI =
         "http://acs.amazonaws.com/groups/s3/LogDelivery";

    /**
     * Return the account to which the group belongs.
     *
     * @return
     */
    public Account getAccount() {
        return account;
    }

    /**
     * Return the ARN of the group.
     *
     * @return
     */
    public String getARN() {
        return arn;
    }

    /**
     * Return the create date of the account.
     *
     * @return
     */
    public String getCreateDate() {
        return createDate;
    }

    /**
     * Return the group ID.
     *
     * @return
     */
    public String getGroupId() {
        return groupId;
    }

    /**
     * Return the name of the group
     *
     * @return
     */
    public String getName() {
        return name;
    }

    /**
     * Return the path of the group.
     *
     * @return
     */
    public String getPath() {
        return path;
    }

    /**
     * Set the account to which the group belongs.
     *
     * @param account
     */
    public void setAccount(Account account) {
        this.account = account;
    }

    /**
     * Set the ARN of the group
     *
     * @param arn
     */
    public void setARN(String arn) {
        this.arn = arn;
    }

    /**
     * Set the create date of the group.
     *
     * @param createDate
     */
    public void setCreateDate(String createDate) {
        this.createDate = createDate;
    }

    /**
     * Set the ID of the group.
     *
     * @param groupId
     */
    public void setGroupId(String groupId) {
        this.groupId = groupId;
    }

    /**
     * Set the name of the group.
     *
     * @param name
     */
    public void setName(String name) {
        this.name = name;
    }

    /**
     * Set the path of the group.
     *
     * @param path
     */
    public void setPath(String path) {
        this.path = path;
    }

    /**
     * Return true if group exists.
     *
     * @return
     */
    public boolean exists() {
        return groupId != null;
    }
}
