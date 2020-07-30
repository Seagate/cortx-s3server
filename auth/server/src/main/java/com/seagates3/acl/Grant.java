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
