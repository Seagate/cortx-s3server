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

import java.util.Date;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

import com.seagates3.model.AccessKey;
import com.seagates3.response.ServerResponse;
import com.seagates3.util.DateUtil;

import io.netty.handler.codec.http.HttpResponseStatus;

public
class TempAuthCredentialsResponseGeneratorTest {

  @Before public void setUp() throws Exception {
  }

  /**
   * Below test will check successful GetTempAuthCredentials response
   */
  @Test public void generateCreateResponseTest() {
    TempAuthCredentialsResponseGenerator responseGenerator =
        new TempAuthCredentialsResponseGenerator();

    AccessKey accessKey = new AccessKey();
    accessKey.setUserId("123");
    accessKey.setId("sdfjgsdfhRRTT335");
    accessKey.setSecretKey("djegjhgwhfg667766333EERTFFFxxxxxx3c");
    accessKey.setToken("djgfhsgshfgTTTTTVVQQAPP90jbdjcbsbdcshb");
    accessKey.setStatus(AccessKey.AccessKeyStatus.ACTIVE);
    Date expiryDate = new Date(DateUtil.getCurrentTime());
    accessKey.setExpiry(DateUtil.toServerResponseFormat(expiryDate));

    ServerResponse response =
        responseGenerator.generateCreateResponse("user1", accessKey);
    Assert.assertEquals(HttpResponseStatus.CREATED,
                        response.getResponseStatus());
  }
}
