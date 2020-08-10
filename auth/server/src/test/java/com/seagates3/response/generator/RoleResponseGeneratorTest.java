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

package com.seagates3.response.generator;

import com.seagates3.model.Account;
import com.seagates3.model.Role;
import com.seagates3.response.ServerResponse;
import io.netty.handler.codec.http.HttpResponseStatus;
import java.util.ArrayList;
import org.junit.Assert;
import org.junit.Test;

public class RoleResponseGeneratorTest {

    final String policyDoc = "{\n"
            + "  \"Version\": \"2012-10-17\",\n"
            + "  \"Statement\": {\n"
            + "    \"Effect\": \"Allow\",\n"
            + "    \"Principal\": {\"Service\": \"s3.seagate.com\"},\n"
            + "    \"Action\": \"sts:AssumeRole\"\n"
            + "  }\n"
            + "}";

    @Test
    public void testCreateResponse() {

        Account account = new Account();
        account.setName("s3test");

        Role role = new Role();
        role.setRoleId("1234");
        role.setARN("arn:seagate:iam::s3test:role/1234");
        role.setPath("/");
        role.setAccount(account);
        role.setName("s3role");
        role.setRolePolicyDoc(policyDoc);
        role.setCreateDate("2015-12-19T07:20:29.000+0530");

        final String expectedResponseBody = "<?xml version=\"1.0\" "
                + "encoding=\"UTF-8\" standalone=\"no\"?>"
                + "<CreateRoleResponse "
                + "xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">"
                + "<CreateRoleResult>"
                + "<Role>"
                + "<Path>/</Path>"
                + "<Arn>arn:seagate:iam::s3test:role/1234</Arn>"
                + "<RoleName>s3role</RoleName>"
                + "<AssumeRolePolicyDocument>{\n"
                + "  \"Version\": \"2012-10-17\",\n"
                + "  \"Statement\": {\n"
                + "    \"Effect\": \"Allow\",\n"
                + "    \"Principal\": {\"Service\": \"s3.seagate.com\"},\n"
                + "    \"Action\": \"sts:AssumeRole\"\n"
                + "  }\n"
                + "}</AssumeRolePolicyDocument>"
                + "<CreateDate>2015-12-19T07:20:29.000+0530</CreateDate>"
                + "<RoleId>1234</RoleId>"
                + "</Role>"
                + "</CreateRoleResult>"
                + "<ResponseMetadata>"
                + "<RequestId>0000</RequestId>"
                + "</ResponseMetadata>"
                + "</CreateRoleResponse>";

        RoleResponseGenerator responseGenerator = new RoleResponseGenerator();
        ServerResponse response = responseGenerator.generateCreateResponse(role);

        Assert.assertEquals(expectedResponseBody, response.getResponseBody());
        Assert.assertEquals(HttpResponseStatus.CREATED, response.getResponseStatus());
    }

    @Test
    public void testListResponse() {
        ArrayList<Role> roles = new ArrayList<>();

        Account account = new Account();
        account.setName("s3test");

        Role role1 = new Role();
        role1.setPath("/");
        role1.setAccount(account);
        role1.setName("s3role");
        role1.setRolePolicyDoc(policyDoc);
        role1.setCreateDate("2015-12-19T07:20:29.000+0530");

        Role role2 = new Role();
        role2.setPath("/");
        role2.setAccount(account);
        role2.setName("s3role2");
        role2.setRolePolicyDoc(policyDoc);
        role2.setCreateDate("2015-12-18T07:20:29.000+0530");

        roles.add(role1);
        roles.add(role2);

        final String expectedResponseBody = "<?xml version=\"1.0\" "
                + "encoding=\"UTF-8\" standalone=\"no\"?>"
                + "<ListRolesResponse "
                + "xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">"
                + "<ListRolesResult>"
                + "<Roles>"
                + "<member>"
                + "<Path>/</Path>"
                + "<Arn>arn:seagate:iam::s3test:s3role</Arn>"
                + "<RoleName>s3role</RoleName>"
                + "<AssumeRolePolicyDocument>{\n"
                + "  \"Version\": \"2012-10-17\",\n"
                + "  \"Statement\": {\n"
                + "    \"Effect\": \"Allow\",\n"
                + "    \"Principal\": {\"Service\": \"s3.seagate.com\"},\n"
                + "    \"Action\": \"sts:AssumeRole\"\n"
                + "  }\n"
                + "}</AssumeRolePolicyDocument>"
                + "<CreateDate>2015-12-19T07:20:29.000+0530</CreateDate>"
                + "<RoleId>s3role</RoleId>"
                + "</member>"
                + "<member>"
                + "<Path>/</Path>"
                + "<Arn>arn:seagate:iam::s3test:s3role2</Arn>"
                + "<RoleName>s3role2</RoleName>"
                + "<AssumeRolePolicyDocument>{\n"
                + "  \"Version\": \"2012-10-17\",\n"
                + "  \"Statement\": {\n"
                + "    \"Effect\": \"Allow\",\n"
                + "    \"Principal\": {\"Service\": \"s3.seagate.com\"},\n"
                + "    \"Action\": \"sts:AssumeRole\"\n"
                + "  }\n"
                + "}</AssumeRolePolicyDocument>"
                + "<CreateDate>2015-12-18T07:20:29.000+0530</CreateDate>"
                + "<RoleId>s3role2</RoleId>"
                + "</member>"
                + "</Roles>"
                + "<IsTruncated>false</IsTruncated>"
                + "</ListRolesResult>"
                + "<ResponseMetadata>"
                + "<RequestId>0000</RequestId>"
                + "</ResponseMetadata>"
                + "</ListRolesResponse>";

        Role[] roleList = new Role[roles.size()];
        RoleResponseGenerator responseGenerator = new RoleResponseGenerator();
        ServerResponse response = responseGenerator.generateListResponse(
                roles.toArray(roleList));

        Assert.assertEquals(expectedResponseBody, response.getResponseBody());
        Assert.assertEquals(HttpResponseStatus.OK, response.getResponseStatus());
    }

    @Test
    public void testDeleteResponse() {
        final String expectedResponseBody = "<?xml version=\"1.0\" "
                + "encoding=\"UTF-8\" standalone=\"no\"?>"
                + "<DeleteRoleResponse "
                + "xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">"
                + "<ResponseMetadata>"
                + "<RequestId>0000</RequestId>"
                + "</ResponseMetadata>"
                + "</DeleteRoleResponse>";

        RoleResponseGenerator responseGenerator = new RoleResponseGenerator();
        ServerResponse response = responseGenerator.generateDeleteResponse();

        Assert.assertEquals(expectedResponseBody, response.getResponseBody());
        Assert.assertEquals(HttpResponseStatus.OK, response.getResponseStatus());
    }
}
