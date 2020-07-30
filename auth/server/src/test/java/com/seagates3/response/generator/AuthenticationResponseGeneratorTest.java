package com.seagates3.response.generator;

import com.seagates3.model.Account;
import com.seagates3.authentication.ClientRequestToken;
import com.seagates3.model.Requestor;
import com.seagates3.response.ServerResponse;
import io.netty.handler.codec.http.HttpResponseStatus;
import org.junit.Assert;
import org.junit.Test;

public class AuthenticationResponseGeneratorTest {

    @Test
    public void testCreateResponse() {
        ClientRequestToken requestToken = new ClientRequestToken();
        requestToken.setSignature("testsign");

        Account account = new Account();
        account.setId("12345");
        account.setName("s3test");
        account.setCanonicalId("dsfhgsjhsdgfQW23cdjbjb");
        account.setEmail("abc@sjsj.com");

        Requestor requestor = new Requestor();
        requestor.setId("123");
        requestor.setName("s3test");
        requestor.setAccount(account);

        final String expectedResponseBody =
            "<?xml version=\"1.0\" " +
            "encoding=\"UTF-8\" standalone=\"no\"?>" +
            "<AuthenticateUserResponse " +
            "xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">" +
            "<AuthenticateUserResult>" + "<UserId>123</UserId>" +
            "<UserName>s3test</UserName>" + "<AccountId>12345</AccountId>" +
            "<AccountName>s3test</AccountName>" +
            "<SignatureSHA256>testsign</SignatureSHA256>" +
            "<CanonicalId>dsfhgsjhsdgfQW23cdjbjb</CanonicalId>" +
            "<Email>abc@sjsj.com</Email>" + "</AuthenticateUserResult>" +
            "<ResponseMetadata>" + "<RequestId>0000</RequestId>" +
            "</ResponseMetadata>" + "</AuthenticateUserResponse>";

        AuthenticationResponseGenerator responseGenerator
                = new AuthenticationResponseGenerator();
        ServerResponse response = responseGenerator.
                generateAuthenticatedResponse(requestor, requestToken);

        Assert.assertEquals(expectedResponseBody, response.getResponseBody());
        Assert.assertEquals(HttpResponseStatus.OK, response.getResponseStatus());
    }
}

