
package com.seagates3.policy;

public
class S3ArnParser extends ArnParser {

 public
  S3ArnParser() {
    /**
     *  ARN format - arn:aws:s3:::<bucket_name>/<key_name>
     *  e.g. - arn:aws:s3:::seagatebucket/abc
     *       - arn:aws:s3:::seagatebucket/dir1/*
     *       - arn:aws:s3:::seagatebucket
     */
    this.regEx = "arn:aws:s3:::[a-zA-Z0-9~@#$^*\\\\/_.:-]+";
  }
}