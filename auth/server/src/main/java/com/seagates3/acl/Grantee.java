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

/* This Class Represents Xml Node As
 *    <Grantee xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
 *      xsi:type="CanonicalUser">
 *      <ID>Int</ID>
 *      <DisplayName>String</DisplayName>
 *    </Grantee>
 */

package com.seagates3.acl;

public
class Grantee {

  String canonicalId;
  String displayName;
  String uri;
  String emailAddress;
  Types type;

  enum Types {
    CanonicalUser,
    Group,
    AmazonCustomerByEmail
  };

 public
  Grantee(String canonicalId, String displayName, String uri,
          String emailAddress, Types type) {
    this.canonicalId = canonicalId;
    this.displayName = displayName;
    this.uri = uri;
    this.emailAddress = emailAddress;
    this.type = type;
  }

 public
  Grantee(String canonicalId, String displayName) {
    this.canonicalId = canonicalId;
    this.displayName = displayName;
    this.type = Types.CanonicalUser;
  }

  void setCanonicalId(String Id) { canonicalId = Id; }

  String getCanonicalId() { return canonicalId; }

  void setDisplayName(String Name) { displayName = Name; }

  String getDisplayName() { return displayName; }

  Types getType() { return type; }

  void setType(Types type) { this.type = type; }

  String getUri() { return uri; }

  void setUri(String uri) { this.uri = uri; }

  String getEmailAddress() { return emailAddress; }

  void setEmailAddress(String emailAddress) {
    this.emailAddress = emailAddress;
  }
}
