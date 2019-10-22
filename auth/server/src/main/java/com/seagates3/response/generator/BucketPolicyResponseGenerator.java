package com.seagates3.response.generator;

import com.seagates3.response.ServerResponse;

import io.netty.handler.codec.http.HttpResponseStatus;

public
class BucketPolicyResponseGenerator extends AbstractResponseGenerator {

 public
  ServerResponse malformedPolicy(String errorMessage) {
    return formatResponse(HttpResponseStatus.BAD_REQUEST, "MalformedPolicy",
                          errorMessage);
  }

 public
  ServerResponse noSuchBucketPolicy() {
    String errorMessage = "The specified bucket does not have a bucket policy.";
    return formatResponse(HttpResponseStatus.NOT_FOUND, "NoSuchBucketPolicy",
                          errorMessage);
  }

 public
  ServerResponse invalidPolicyDocument() {
    String errorMessage =
        "The content of the form does not meet the conditions specified in " +
        "the " + "policy document.";
    return formatResponse(HttpResponseStatus.BAD_REQUEST,
                          "InvalidPolicyDocument", errorMessage);
  }
}
