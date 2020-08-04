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

package com.seagates3.aws.request;

import com.seagates3.authentication.AWSRequestParserV2;
import com.seagates3.authserver.AuthServerConfig;
import com.seagates3.aws.AWSV2RequestHelper;
import com.seagates3.exception.InvalidTokenException;
import com.seagates3.authentication.ClientRequestToken;
import java.util.Map;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.internal.matchers.apachecommons.ReflectionEquals;
import org.powermock.api.mockito.PowerMockito;
import org.powermock.core.classloader.annotations.PowerMockIgnore;
import org.powermock.core.classloader.annotations.PrepareForTest;
import org.powermock.modules.junit4.PowerMockRunner;

@RunWith(PowerMockRunner.class)
@PrepareForTest(AuthServerConfig.class)
@PowerMockIgnore( {"javax.management.*"})

public class AWSRequestParserV2Test {

    private final AWSRequestParserV2 parser;

    public AWSRequestParserV2Test() {
        parser = new AWSRequestParserV2();
    }

    @Before
    public void setup() throws Exception {
        String defaultEndPoints = "s3.seagate.com";
        String[] endpoints = {"s3-us.seagate.com",
            "s3-europe.seagate.com", "s3-asia.seagate.com"};

        PowerMockito.mockStatic(AuthServerConfig.class);
        PowerMockito.doReturn(endpoints).when(AuthServerConfig.class,
                "getEndpoints");

        PowerMockito.doReturn(defaultEndPoints).when(AuthServerConfig.class,
                "getDefaultEndpoint");
    }

    @Test
    public void Parse_RequestBody_PathStyle() throws InvalidTokenException {
        Map<String, String> requestHeaders
                = AWSV2RequestHelper.getRequestHeadersPathStyle();
        ClientRequestToken requestToken
                = AWSV2RequestHelper.getRequestClientTokenPathStyle();

        Assert.assertThat(requestToken,
                new ReflectionEquals(parser.parse(requestHeaders)));
    }

    @Test
    public void Parse_RequestBody_VirtualHostStyle() throws InvalidTokenException {
        Map<String, String> requestHeaders
                = AWSV2RequestHelper.getRequestHeadersVirtualHostStyle();
        ClientRequestToken requestToken
                = AWSV2RequestHelper.getRequestClientTokenVirtualHostStyle();

        ClientRequestToken actualtoken = parser.parse(requestHeaders);
        Assert.assertThat(requestToken,
                new ReflectionEquals(parser.parse(requestHeaders)));
    }
}
