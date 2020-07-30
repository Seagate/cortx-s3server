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
