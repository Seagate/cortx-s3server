package com.seagates3.response.formatter;

import java.util.ArrayList;
import java.util.LinkedHashMap;

import com.seagates3.response.ServerResponse;

import io.netty.handler.codec.http.HttpResponseStatus;

public
abstract class AbstractResponseFormatter {

 protected
  final String IAM_XMLNS = "https://iam.seagate.com/doc/2010-05-08/";

 public
  abstract ServerResponse
      formatCreateResponse(String operation, String returnObject,
                           LinkedHashMap<String, String> responseElements,
                           String requestId);

 public
  abstract ServerResponse formatDeleteResponse(String operation);

 public
  abstract ServerResponse formatUpdateResponse(String operation);

 public
  abstract ServerResponse formatListResponse(
      String operation, String returnObject,
      ArrayList<LinkedHashMap<String, String>> responseElements,
      Boolean isTruncated, String requestId);

 public
  abstract ServerResponse
      formatErrorResponse(HttpResponseStatus httpResponseStatus, String code,
                          String message);

 public
  abstract ServerResponse formatGetResponse(
      String operation, String returnObject,
      ArrayList<LinkedHashMap<String, String>> responseElements,
      String requestId);

 public
  abstract ServerResponse formatChangePasswordResponse(String operation);
}
