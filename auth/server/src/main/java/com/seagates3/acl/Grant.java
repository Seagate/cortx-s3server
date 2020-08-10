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

/*
 * This Class Represents Xml Node As
 *  <Grant>
 *    <Grantee xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
 *         xsi:type="CanonicalUser">
 *      <ID>Int</ID>
 *      <DisplayName>String</DisplayName>
 *    </Grantee>
 *    <Permission>String</Permission>
 *  </Grant>
 */

package com.seagates3.acl;
public
class Grant {

  String permission;
  Grantee grantee;

 public
  Grant(Grantee grantee, String permission) {
    this.grantee = grantee;
    this.permission = permission;
  }

  void setGrantee(Grantee gran_tee) { grantee = gran_tee; }

  Grantee getGrantee() { return grantee; }

  void setPermission(String permi_ssion) { permission = permi_ssion; }

  String getPermission() { return permission; }
}
