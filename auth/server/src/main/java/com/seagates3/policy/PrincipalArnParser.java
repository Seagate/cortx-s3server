
package com.seagates3.policy;

public
class PrincipalArnParser extends ArnParser {

 public
  PrincipalArnParser() {
    /**
     *  ARN format - arn:aws:iam::<accountID>:<user>
     *  e.g. arn:aws:iam::KO87b1p0TKWa184S6xrINQ:user/u1
     *       arn:aws:iam::KO87b1p0TKWa184S6xrINQ:root
     *       arn:aws:iam::123456789012:user/user-name
     *       arn:aws:iam::123456789012:role/role-name
     */
    this.regEx = "arn:aws:iam::[a-zA-Z0-9~@#$^*\\\\/_.:-]+" +
                 ":[a-zA-Z0-9~@#$^*\\\\/_.:-]+";
  }
}