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

package com.seagates3.s3service;

import java.io.IOException;

import java.util.HashMap;
import java.util.Iterator;
import java.util.Map;

import org.apache.http.client.methods.HttpRequestBase;

import org.apache.http.client.ClientProtocolException;
import org.apache.http.client.methods.CloseableHttpResponse;
import org.apache.http.client.methods.HttpDelete;

import org.apache.http.client.methods.HttpPost;
import org.apache.http.impl.client.CloseableHttpClient;
import org.apache.http.impl.client.HttpClientBuilder;

import com.seagates3.exception.S3RequestInitializationException;
import com.seagates3.util.AWSSignUtil;
import com.seagates3.util.DateUtil;

import io.netty.handler.codec.http.HttpMethod;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public
class S3RestClient {

 private
  final Logger LOGGER =
      LoggerFactory.getLogger(S3AccountNotifier.class.getName());

 public
  String method = null;
 public
  Map<String, String> headers = new HashMap<String, String>();
 public
  String payload = null;
 public
  String accessKey = null;
 public
  String secretKey = null;
 public
  String URL = null;
 public
  String resource = null;

 public
  void setHeader(String header, String headerValue) {
    headers.put(header, headerValue);
  }

 public
  void setMethod(String reqType) { method = reqType; }

 public
  void setCredentials(String aKey, String sKey) {
    accessKey = aKey;
    secretKey = sKey;
  }

 public
  void setURL(String url) { URL = url; }

 public
  void setResource(String res) { resource = res; }

  /**
   * Validate values of all mandatory S3 Args
   * @return boolean
   */
 private
  boolean validateRequiredParams() {
    if (method == null) {
      LOGGER.error("Rest request method type missing");
      return false;
    }
    if (resource == null) {
      LOGGER.error("Rest request resource value missing");
      return false;
    }
    if (URL == null) {
      LOGGER.error("Rest request URL value missing");
      return false;
    }
    if (accessKey == null || accessKey.isEmpty() || secretKey == null ||
        secretKey.isEmpty()) {
      LOGGER.error("Missing/invalid accessKey or secretKey values");
      return false;
    }
    return true;
  }

  /**
   * Initializes REST request
   * @param HttpRequestBase
   * @return boolean
   */
 private
  boolean prepareRequest(HttpRequestBase req) {
    final String d = DateUtil.getCurrentTimeGMT();
    final String payload;
    LOGGER.debug("headers-- " + headers);
    if (headers.containsKey("x-amz-security-token")) {
      payload = method + "\n\n\n" + d + "\n" + "x-amz-security-token:" +
                headers.get("x-amz-security-token") + "\n" + resource;
    } else {
      payload = method + "\n\n\n" + d + "\n" + resource;
    }
    String signature = AWSSignUtil.calculateSignatureAWSV2(payload, secretKey);

    if (signature == null) {
      LOGGER.error("Failed to calculate signature for payload:" + payload);
      return false;
    }

    LOGGER.debug("Payload:" + payload + " Signature:" + signature);

    req.setHeader("Authorization", "AWS " + accessKey + ":" + signature);
    req.setHeader("Date", d);
    Iterator<Map.Entry<String, String>> itr = headers.entrySet().iterator();

    while (itr.hasNext()) {
      Map.Entry<String, String> entry = itr.next();
      req.setHeader(entry.getKey(), entry.getValue());
    }
    return true;
  }

  /**
   * Method to prepare and execute S3 Rest Request
   * @throws S3RequestInitializationException
   * @throws IOException
   * @throws ClientProtocolException
   */
 private
  S3HttpResponse execute(HttpRequestBase req)
      throws S3RequestInitializationException,
      ClientProtocolException, IOException {

    LOGGER.debug("Executing " + method + "request for resource:" + resource);

    if (!prepareRequest(req)) {
      LOGGER.error("Failed to prepare S3 REST Post request");
      throw new S3RequestInitializationException("Failed to initialize " +
                                                 "S3 Post Request");
    }

    S3HttpResponse s3Resp = new S3HttpResponse();
    try(CloseableHttpClient httpClient = HttpClientBuilder.create().build();
        CloseableHttpResponse resp = httpClient.execute(req)) {
      s3Resp.setHttpCode(resp.getStatusLine().getStatusCode());
      s3Resp.setHttpStatusMessage(resp.getStatusLine().getReasonPhrase());
    }
    finally { req.releaseConnection(); }

    return s3Resp;
  }

  /**
   * Method sends a POST request to S3 Server
   * @return HttpRespose
   * @throws S3RequestInitializationException
   * @throws ClientProtocolException
   * @throws IOException
   */
 public
  S3HttpResponse postRequest() throws S3RequestInitializationException,
      ClientProtocolException, IOException {
    method = HttpMethod.POST.toString();

    if (!validateRequiredParams()) {
      LOGGER.error("Invalid REST request parameter values found");
      throw new S3RequestInitializationException("Failed to initialize " +
                                                 "S3 Post Request");
    }
    HttpPost req = new HttpPost(URL);

    return execute(req);
  }

  /**
   * Method sends DELETE request to S3 Server
   * @return HttpResponse
   * @throws S3RequestInitializationException
   * @throws ClientProtocolException
   * @throws IOException
   */
 public
  S3HttpResponse deleteRequest() throws S3RequestInitializationException,
      ClientProtocolException, IOException {
    method = HttpMethod.DELETE.toString();

    if (!validateRequiredParams()) {
      LOGGER.error("Invalid REST request parameter values found");
      throw new S3RequestInitializationException("Failed to initialize " +
                                                 "S3 Delete Request");
    }
    HttpDelete req = new HttpDelete(URL);

    return execute(req);
  }
}
