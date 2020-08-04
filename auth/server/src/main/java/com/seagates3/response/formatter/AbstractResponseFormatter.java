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
