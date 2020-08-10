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

package com.seagates3.response.generator;

import java.util.LinkedHashMap;

import com.seagates3.model.AccessKey;
import com.seagates3.response.ServerResponse;
import com.seagates3.response.formatter.xml.AccessKeyResponseFormatter;

public
class TempAuthCredentialsResponseGenerator extends AbstractResponseGenerator {

 public
  ServerResponse generateCreateResponse(String userName, AccessKey accessKey) {
    LinkedHashMap<String, String> responseElements = new LinkedHashMap<>();
    responseElements.put("UserName", userName);
    responseElements.put("AccessKeyId", accessKey.getId());
    responseElements.put("Status", accessKey.getStatus());
    responseElements.put("SecretAccessKey", accessKey.getSecretKey());
    responseElements.put("ExpiryTime", accessKey.getExpiry());
    responseElements.put("SessionToken", accessKey.getToken());

    return new AccessKeyResponseFormatter().formatCreateResponse(
        "GetTempAuthCredentials", "AccessKey", responseElements, "0000");
  }
}
