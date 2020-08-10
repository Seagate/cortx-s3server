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

package com.seagates3.response.formatter.xml;

import com.seagates3.response.ServerResponse;
import io.netty.handler.codec.http.HttpResponseStatus;
import java.util.LinkedHashMap;
import org.junit.Assert;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.ExpectedException;

public class SAMLProviderResponseFormatterTest {

    @Rule
    public final ExpectedException exception = ExpectedException.none();

    @Test
    public void testFormatCreateResponse() {
        LinkedHashMap responseElements = new LinkedHashMap();
        responseElements.put("Arn", "arn:seagate:iam::s3test:s3TestIDP");

        final String expectedResponseBody = "<?xml version=\"1.0\" "
                + "encoding=\"UTF-8\" standalone=\"no\"?>"
                + "<CreateSAMLProviderResponse "
                + "xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">"
                + "<CreateSAMLProviderResult>"
                + "<SAMLProviderArn>arn:seagate:iam::s3test:s3TestIDP"
                + "</SAMLProviderArn>"
                + "</CreateSAMLProviderResult>"
                + "<ResponseMetadata>"
                + "<RequestId>9999</RequestId>"
                + "</ResponseMetadata>"
                + "</CreateSAMLProviderResponse>";

        ServerResponse response = new SAMLProviderResponseFormatter()
                .formatCreateResponse("CreateSAMLProvider",
                        null, responseElements, "9999");

        Assert.assertEquals(expectedResponseBody, response.getResponseBody());
        Assert.assertEquals(HttpResponseStatus.CREATED, response.getResponseStatus());
    }

    @Test
    public void testFormatUpdateResponse() {
        LinkedHashMap responseElements = new LinkedHashMap();
        responseElements.put("Name", "s3TestIDP");

        final String expectedResponseBody = "<?xml version=\"1.0\" "
                + "encoding=\"UTF-8\" standalone=\"no\"?>"
                + "<UpdateSAMLProviderResponse "
                + "xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">"
                + "<UpdateSAMLProviderResult>"
                + "<SAMLProviderArn>arn:seagate:iam:::s3TestIDP"
                + "</SAMLProviderArn>"
                + "</UpdateSAMLProviderResult>"
                + "<ResponseMetadata>"
                + "<RequestId>9999</RequestId>"
                + "</ResponseMetadata>"
                + "</UpdateSAMLProviderResponse>";

        ServerResponse response = new SAMLProviderResponseFormatter()
                .formatUpdateResponse("s3TestIDP", "9999");

        Assert.assertEquals(expectedResponseBody, response.getResponseBody());
        Assert.assertEquals(HttpResponseStatus.OK, response.getResponseStatus());
    }

    @Test
    public void testFormatUpdateResponseException() {
        exception.expect(UnsupportedOperationException.class);
        new SAMLProviderResponseFormatter().formatUpdateResponse("UpdateSAMLProvider");
    }
}
