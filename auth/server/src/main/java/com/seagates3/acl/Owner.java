/* This Class Represents Xml Node As
 *
 *  <Owner>
 *    <ID>123er45678</ID>
 *    <DisplayName>S3test</DisplayName>
 *  </Owner>
 */

package com.seagates3.acl;

public
class Owner {

  String canonicalId;
  String displayName;

 public
  Owner(String canonicalId, String displayName) {
    this.canonicalId = canonicalId;
    this.displayName = displayName;
  }

  void setCanonicalId(String Id) { canonicalId = Id; }

  String getCanonicalId() { return canonicalId; }

  void setDisplayName(String Name) { displayName = Name; }

  String getDisplayName() { return displayName; }
}
