package com.seagates3.policy;

public
class IAMArnParser extends ArnParser {
 public
  IAMArnParser() {

    this.regEx = "arn:aws:(iam|s3)::[a-zA-Z0-9~@#$^*\\\\/_.:-]+";
  }
}