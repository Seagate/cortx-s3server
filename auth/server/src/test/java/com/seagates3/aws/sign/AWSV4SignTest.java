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

package com.seagates3.aws.sign;

import com.seagates3.authentication.AWSV4Sign;
import com.seagates3.aws.AWSV4RequestHelper;
import com.seagates3.exception.InvalidTokenException;
import com.seagates3.authentication.ClientRequestToken;
import com.seagates3.model.Requestor;
import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.powermock.core.classloader.annotations.PowerMockIgnore;
import org.powermock.modules.junit4.PowerMockRunner;

@RunWith(PowerMockRunner.class)
@PowerMockIgnore({"javax.crypto.*", "javax.management.*"})
public class AWSV4SignTest {

    private final AWSV4Sign awsv4Sign;
    private final Requestor requestor;

    public AWSV4SignTest() {
        awsv4Sign = new AWSV4Sign();
        requestor = AWSV4RequestHelper.getRequestor();
    }

    @Test
    public void Authenticate_ChunkedSeedRequest_True() {
        ClientRequestToken requestToken
                = AWSV4RequestHelper.getChunkedSeedRequestClientToken();

        try {
                Assert.assertEquals(awsv4Sign.authenticate(requestToken, requestor),
                   Boolean.TRUE);
        } catch (InvalidTokenException e) {
            e.printStackTrace();
            Assert.fail("This Shouldn't have thrown exception");
        }

    }

    @Test
    public void Authenticate_ChunkedSeedRequest_False() {
        ClientRequestToken requestToken
                = AWSV4RequestHelper.getChunkedSeedRequestClientToken();

        requestToken.setHttpMethod("GET");

        try {
            Assert.assertEquals(awsv4Sign.authenticate(requestToken, requestor),
               Boolean.FALSE);
        } catch (InvalidTokenException e) {
            e.printStackTrace();
            Assert.fail("This Shouldn't have thrown exception");
        }

    }

    @Test
    public void Authenticate_ChunkedRequest_True() {
        ClientRequestToken requestToken
                = AWSV4RequestHelper.getChunkedRequestClientToken();

        try {
            Assert.assertEquals(awsv4Sign.authenticate(requestToken, requestor),
               Boolean.TRUE);
        } catch (InvalidTokenException e) {
            e.printStackTrace();
            Assert.fail("This Shouldn't have thrown exception");
        }

    }

    @Test
    public void Authenticate_RequestPathStyle_True() {
        ClientRequestToken requestToken
                = AWSV4RequestHelper.getRequestClientTokenPathStyle();

        try {
            Assert.assertEquals(awsv4Sign.authenticate(requestToken, requestor),
               Boolean.TRUE);
        } catch (InvalidTokenException e) {
            e.printStackTrace();
            Assert.fail("This Shouldn't have thrown exception");
        }

    }

    @Test
    public void Authenticate_RequestVirtualHostStyle_True() {
        ClientRequestToken requestToken
                = AWSV4RequestHelper.getRequestClientTokenVirtualHostStyle();

        try {
            Assert.assertEquals(awsv4Sign.authenticate(requestToken, requestor),
                    Boolean.TRUE);
        } catch (InvalidTokenException e) {
            e.printStackTrace();
            Assert.fail("This Shouldn't have thrown exception");
        }

    }

    @Test
    public void Authenticate_RequestSpecialQueryParams() {
        ClientRequestToken requestToken
                = AWSV4RequestHelper.getRequestClientTokenSpecialQuery();

        try {
            Assert.assertEquals(awsv4Sign.authenticate(requestToken, requestor),
                    Boolean.TRUE);
        } catch (InvalidTokenException e) {
            e.printStackTrace();
            Assert.fail("This Shouldn't have thrown exception");
        }

    }

    @Test
    public void Authenticate_RequestVirtualHostStyleHead_True() {
        ClientRequestToken requestToken
                = AWSV4RequestHelper.getFullHttpRequestClientTokenHEAD();

        Requestor requestor1 =
                AWSV4RequestHelper.getRequestorMock("AKIAJTYX36YCKQSAJT7Q",
                                "A6k2z84BqwXmee4WUUS2oWwM/tha7Wrd4Hc/8yRt");
        try {
            Assert.assertEquals(awsv4Sign.authenticate(requestToken, requestor1),
                    Boolean.TRUE);
        } catch (InvalidTokenException e) {
            e.printStackTrace();
            Assert.fail("This Shouldn't have thrown exception");
        }

    }

    @Test
    public void Authenticate_RequestVirtualHostStyle_InvalidRequest() {
        ClientRequestToken requestToken
                = AWSV4RequestHelper.getInvalidHttpRequestClientToken();

        Requestor requestor1 =
                AWSV4RequestHelper.getRequestorMock("AKIAJTYX36YCKQSAJT7Q",
                                "A6k2z84BqwXmee4WUUS2oWwM/tha7Wrd4Hc/8yRt");
        try {
            awsv4Sign.authenticate(requestToken, requestor1);
            Assert.fail("Didn't throw BadRequest Exception");
        } catch (InvalidTokenException e) {
          Assert.assertTrue(e.getMessage().contains(
              "Signed header :" +
              "connection1 is not found in Request header list"));
        }
    }

}
