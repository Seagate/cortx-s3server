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
package com.seagates3.response.formatter.xml;

import com.seagates3.response.ServerResponse;
import io.netty.handler.codec.http.HttpResponseStatus;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import org.junit.Assert;
import org.junit.Test;

public
class XMLResponseFormatterTest {

  @Test public void testFormatCreateResponse() {
    LinkedHashMap responseElements = new LinkedHashMap();
    responseElements.put("Path", "/");
    responseElements.put("UserName", "s3test");
    responseElements.put("UserId", "123");
    responseElements.put("Arn", "arn:123");

    final String expectedResponseBody =
        "<?xml version=\"1.0\" " + "encoding=\"UTF-8\" standalone=\"no\"?>" +
        "<CreateUserResponse " +
        "xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">" +
        "<CreateUserResult>" + "<User>" + "<Path>/</Path>" +
        "<UserName>s3test</UserName>" + "<UserId>123</UserId>" +
        "<Arn>arn:123</Arn>" + "</User>" + "</CreateUserResult>" +
        "<ResponseMetadata>" + "<RequestId>9999</RequestId>" +
        "</ResponseMetadata>" + "</CreateUserResponse>";

    ServerResponse response = new XMLResponseFormatter().formatCreateResponse(
        "CreateUser", "User", responseElements, "9999");

    Assert.assertEquals(expectedResponseBody, response.getResponseBody());
    Assert.assertEquals(HttpResponseStatus.CREATED,
                        response.getResponseStatus());
  }

  @Test public void testFormatDeleteResponse() {
    final String expectedResponseBody =
        "<?xml version=\"1.0\" " + "encoding=\"UTF-8\" standalone=\"no\"?>" +
        "<DeleteUserResponse " +
        "xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">" +
        "<ResponseMetadata>" + "<RequestId>0000</RequestId>" +
        "</ResponseMetadata>" + "</DeleteUserResponse>";

    ServerResponse response =
        new XMLResponseFormatter().formatDeleteResponse("DeleteUser");

    Assert.assertEquals(expectedResponseBody, response.getResponseBody());
    Assert.assertEquals(HttpResponseStatus.OK, response.getResponseStatus());
  }

  @Test public void testFormatUpdateResponse() {
    final String expectedResponseBody =
        "<?xml version=\"1.0\" " + "encoding=\"UTF-8\" standalone=\"no\"?>" +
        "<UpdateUserResponse " +
        "xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">" +
        "<ResponseMetadata>" + "<RequestId>0000</RequestId>" +
        "</ResponseMetadata>" + "</UpdateUserResponse>";

    ServerResponse response =
        new XMLResponseFormatter().formatUpdateResponse("UpdateUser");

    Assert.assertEquals(expectedResponseBody, response.getResponseBody());
    Assert.assertEquals(HttpResponseStatus.OK, response.getResponseStatus());
  }

  @Test public void testFormatListResponse() {
    ArrayList<LinkedHashMap<String, String>> userMemebers = new ArrayList<>();
    LinkedHashMap user1 = new LinkedHashMap();
    user1.put("Path", "/");
    user1.put("UserName", "s3test1");
    user1.put("UserId", "123");
    user1.put("Arn", "arn:123");

    userMemebers.add(user1);

    LinkedHashMap user2 = new LinkedHashMap();
    user2.put("Path", "/test/");
    user2.put("UserName", "s3test2");
    user2.put("UserId", "456");
    user2.put("Arn", "arn:456");

    userMemebers.add(user2);

    final String expectedResponseBody =
        "<?xml version=\"1.0\" " + "encoding=\"UTF-8\" standalone=\"no\"?>" +
        "<ListUsersResponse " +
        "xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">" +
        "<ListUsersResult>" + "<Users>" + "<member>" + "<Path>/</Path>" +
        "<UserName>s3test1</UserName>" + "<UserId>123</UserId>" +
        "<Arn>arn:123</Arn>" + "</member>" + "<member>" +
        "<Path>/test/</Path>" + "<UserName>s3test2</UserName>" +
        "<UserId>456</UserId>" + "<Arn>arn:456</Arn>" + "</member>" +
        "</Users>" + "<IsTruncated>false</IsTruncated>" + "</ListUsersResult>" +
        "<ResponseMetadata>" + "<RequestId>9999</RequestId>" +
        "</ResponseMetadata>" + "</ListUsersResponse>";

    ServerResponse response = new XMLResponseFormatter().formatListResponse(
        "ListUsers", "Users", userMemebers, false, "9999");

    Assert.assertEquals(expectedResponseBody, response.getResponseBody());
    Assert.assertEquals(HttpResponseStatus.OK, response.getResponseStatus());
  }

  @Test public void testFormatResetAccountAccessKeyResponse() {
    LinkedHashMap responseElements = new LinkedHashMap();
    responseElements.put("Path", "/");
    responseElements.put("UserName", "s3test");
    responseElements.put("UserId", "123");
    responseElements.put("Arn", "arn:123");

    final String expectedResponseBody =
        "<?xml version=\"1.0\" " + "encoding=\"UTF-8\" standalone=\"no\"?>" +
        "<ResetAccountAccessKeyResponse " +
        "xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">" +
        "<ResetAccountAccessKeyResult>" + "<User>" + "<Path>/</Path>" +
        "<UserName>s3test</UserName>" + "<UserId>123</UserId>" +
        "<Arn>arn:123</Arn>" + "</User>" + "</ResetAccountAccessKeyResult>" +
        "<ResponseMetadata>" + "<RequestId>9999</RequestId>" +
        "</ResponseMetadata>" + "</ResetAccountAccessKeyResponse>";

    ServerResponse response =
        new XMLResponseFormatter().formatResetAccountAccessKeyResponse(
            "ResetAccountAccessKey", "User", responseElements, "9999");

    Assert.assertEquals(expectedResponseBody, response.getResponseBody());
    Assert.assertEquals(HttpResponseStatus.CREATED,
                        response.getResponseStatus());
  }

  /**
   * Below method will test formatGetResponse method
   */
  @Test public void testFormatGetResponse() {
    ArrayList<LinkedHashMap<String, String>> userMembers = new ArrayList<>();
    LinkedHashMap<String, String> user = new LinkedHashMap<>();
    user.put("UserName", "testUser");
    userMembers.add(user);
    final String expectedResponseBody =
        "<?xml version=\"1.0\" " + "encoding=\"UTF-8\" standalone=\"no\"?>" +
        "<GetLoginProfileResponse " +
        "xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">" +
        "<GetLoginProfileResult>" + "<LoginProfile>" +
        "<UserName>testUser</UserName>" + "</LoginProfile>" +
        "</GetLoginProfileResult>" + "<ResponseMetadata>" +
        "<RequestId>0000</RequestId>" + "</ResponseMetadata>" +
        "</GetLoginProfileResponse>";

    ServerResponse response = new XMLResponseFormatter().formatGetResponse(
        "GetLoginProfile", "LoginProfile", userMembers, "0000");

    Assert.assertEquals(expectedResponseBody, response.getResponseBody());
    Assert.assertEquals(HttpResponseStatus.OK, response.getResponseStatus());
  }
}
