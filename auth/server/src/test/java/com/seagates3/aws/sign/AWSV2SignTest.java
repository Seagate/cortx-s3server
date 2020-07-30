package com.seagates3.aws.sign;

import com.seagates3.authentication.AWSV2Sign;
import com.seagates3.aws.AWSV2RequestHelper;
import com.seagates3.authentication.ClientRequestToken;
import com.seagates3.model.Requestor;
import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.powermock.core.classloader.annotations.PowerMockIgnore;
import org.powermock.modules.junit4.PowerMockRunner;

@RunWith(PowerMockRunner.class)
@PowerMockIgnore({"javax.crypto.*", "javax.management.*"})
public class AWSV2SignTest {

    private final AWSV2Sign awsv2Sign;
    private final Requestor requestor;

    public AWSV2SignTest() {
        awsv2Sign = new AWSV2Sign();
        requestor = AWSV2RequestHelper.getRequestor();
    }

    @Test
    public void Authenticate_RequestPathStyle_True() {
        ClientRequestToken requestToken
                = AWSV2RequestHelper.getRequestClientTokenPathStyle();

        Assert.assertEquals(awsv2Sign.authenticate(requestToken, requestor),
                Boolean.TRUE);

    }

    @Test
    public void Authenticate_RequestPathStyle_False() {
        ClientRequestToken requestToken
                = AWSV2RequestHelper.getRequestClientTokenPathStyle();
        requestToken.setUri("test");

        Assert.assertEquals(awsv2Sign.authenticate(requestToken, requestor),
                Boolean.FALSE);
    }

    @Test
    public void Authenticate_RequestVirtualHostStyle_True() {
        ClientRequestToken requestToken
                = AWSV2RequestHelper.getRequestClientTokenVirtualHostStyle();

        Assert.assertEquals(awsv2Sign.authenticate(requestToken, requestor),
                Boolean.TRUE);

    }

    @Test
    public void Authenticate_RequestHasSubResource_True() {
        ClientRequestToken requestToken
                = AWSV2RequestHelper.getClientRequestTokenSubResourceStringTest();

        Assert.assertEquals(awsv2Sign.authenticate(requestToken, requestor),
                Boolean.TRUE);
    }

}
