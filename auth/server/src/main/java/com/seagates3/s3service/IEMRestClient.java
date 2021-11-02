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

import com.seagates3.authserver.AuthServerConfig;

import io.netty.handler.codec.http.HttpResponseStatus;

import java.io.IOException;
import org.apache.http.client.ClientProtocolException;
import org.apache.http.client.methods.CloseableHttpResponse;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.entity.ContentType;
import org.apache.http.entity.StringEntity;
import org.apache.http.impl.client.CloseableHttpClient;
import org.apache.http.impl.client.HttpClientBuilder;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public
class IEMRestClient {

 private
  final static Logger LOGGER =
      LoggerFactory.getLogger(IEMRestClient.class.getName());

  /**
   * Method sends IEM alerts to IEM server as POST request
   * @return HttpRespose
   * @throws ClientProtocolException
   * @throws IOException
   */
 public
  static S3HttpResponse postRequest(String iem_msg)
      throws ClientProtocolException,
      IOException {
    // URL will be stored in Authserver.properties file
    // e.g. http://127.0.0.1:28300/EventMessage/event
    String URL = AuthServerConfig.getIEMServerURL();

    HttpPost req = new HttpPost(URL);
    req.setHeader("Content-Type", "application/json");

    req.setEntity(new StringEntity(iem_msg, ContentType.APPLICATION_JSON));
    S3HttpResponse s3Resp = new S3HttpResponse();

    try(CloseableHttpClient httpClient = HttpClientBuilder.create().build();
        CloseableHttpResponse resp = httpClient.execute(req)) {
      s3Resp.setHttpCode(resp.getStatusLine().getStatusCode());
      s3Resp.setHttpStatusMessage(resp.getStatusLine().getReasonPhrase());
    }
    catch (Exception e) {
      s3Resp.setHttpCode(HttpResponseStatus.INTERNAL_SERVER_ERROR.code());
      s3Resp.setHttpStatusMessage(e.getMessage());
      LOGGER.error("Failed to send IEM alert message" + e.getMessage());
    }
    finally { req.releaseConnection(); }
    return s3Resp;
  }
}
