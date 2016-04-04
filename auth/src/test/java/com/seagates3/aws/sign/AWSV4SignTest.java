/*
 * COPYRIGHT 2016 SEAGATE LLC
 *
 * THIS DRAWING/DOCUMENT, ITS SPECIFICATIONS, AND THE DATA CONTAINED
 * HEREIN, ARE THE EXCLUSIVE PROPERTY OF SEAGATE TECHNOLOGY
 * LIMITED, ISSUED IN STRICT CONFIDENCE AND SHALL NOT, WITHOUT
 * THE PRIOR WRITTEN PERMISSION OF SEAGATE TECHNOLOGY LIMITED,
 * BE REPRODUCED, COPIED, OR DISCLOSED TO A THIRD PARTY, OR
 * USED FOR ANY PURPOSE WHATSOEVER, OR STORED IN A RETRIEVAL SYSTEM
 * EXCEPT AS ALLOWED BY THE TERMS OF SEAGATE LICENSES AND AGREEMENTS.
 *
 * YOU SHOULD HAVE RECEIVED A COPY OF SEAGATE'S LICENSE ALONG WITH
 * THIS RELEASE. IF NOT PLEASE CONTACT A SEAGATE REPRESENTATIVE
 * http://www.seagate.com/contact
 *
 * Original author:  Arjun Hariharan <arjun.hariharan@seagate.com>
 * Original creation date: 17-Mar-2016
 */
package com.seagates3.aws.sign;

import com.seagates3.aws.AWSV4RequestHelper;
import com.seagates3.model.ClientRequestToken;
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

        Assert.assertEquals(awsv4Sign.authenticate(requestToken, requestor),
                Boolean.TRUE);

    }

    @Test
    public void Authenticate_ChunkedSeedRequest_False() {
        ClientRequestToken requestToken
                = AWSV4RequestHelper.getChunkedSeedRequestClientToken();

        requestToken.setHttpMethod("GET");

        Assert.assertEquals(awsv4Sign.authenticate(requestToken, requestor),
                Boolean.FALSE);

    }

    @Test
    public void Authenticate_ChunkedRequest_True() {
        ClientRequestToken requestToken
                = AWSV4RequestHelper.getChunkedRequestClientToken();

        Assert.assertEquals(awsv4Sign.authenticate(requestToken, requestor),
                Boolean.TRUE);

    }

    @Test
    public void Authenticate_RequestPathStyle_True() {
        ClientRequestToken requestToken
                = AWSV4RequestHelper.getRequestClientTokenPathStyle();

        Assert.assertEquals(awsv4Sign.authenticate(requestToken, requestor),
                Boolean.TRUE);

    }

    @Test
    public void Authenticate_RequestVirtualHostStyle_True() {
        ClientRequestToken requestToken
                = AWSV4RequestHelper.getRequestClientTokenVirtualHostStyle();

        Assert.assertEquals(awsv4Sign.authenticate(requestToken, requestor),
                Boolean.TRUE);

    }

}
