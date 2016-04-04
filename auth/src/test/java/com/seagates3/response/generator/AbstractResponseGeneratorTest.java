/*
 * COPYRIGHT 2015 SEAGATE LLC
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
 * Original creation date: 18-Dec-2015
 */
package com.seagates3.response.generator;

import com.seagates3.response.ServerResponse;
import io.netty.handler.codec.http.HttpResponseStatus;
import org.junit.Assert;
import org.junit.Test;

public class AbstractResponseGeneratorTest extends AbstractResponseGenerator {

    @Test
    public void testBadRequest() {
        final String expectedResponseBody = "<?xml version=\"1.0\" "
                + "encoding=\"UTF-8\" standalone=\"no\"?>"
                + "<Error xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">"
                + "<Code>BadRequest</Code>"
                + "<Message>Bad Request. Check request headers and body.</Message>"
                + "<RequestId>0000</RequestId>"
                + "</Error>";

        ServerResponse response = badRequest();
        Assert.assertEquals(expectedResponseBody, response.getResponseBody());
        Assert.assertEquals(HttpResponseStatus.BAD_REQUEST, response.getResponseStatus());
    }

    @Test
    public void testDeleteConflict() {
        final String expectedResponseBody = "<?xml version=\"1.0\" "
                + "encoding=\"UTF-8\" standalone=\"no\"?>"
                + "<Error xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">"
                + "<Code>DeleteConflict</Code>"
                + "<Message>The request was rejected because it attempted to "
                + "delete a resource that has attached subordinate entities. "
                + "The error message describes these entities.</Message>"
                + "<RequestId>0000</RequestId>"
                + "</Error>";

        ServerResponse response = deleteConflict();
        Assert.assertEquals(expectedResponseBody, response.getResponseBody());
        Assert.assertEquals(HttpResponseStatus.CONFLICT, response.getResponseStatus());
    }

    @Test
    public void testEntityAlreadyExists() {
        final String expectedResponseBody = "<?xml version=\"1.0\" "
                + "encoding=\"UTF-8\" standalone=\"no\"?>"
                + "<Error xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">"
                + "<Code>EntityAlreadyExists</Code>"
                + "<Message>The request was rejected because it attempted to "
                + "create or update a resource that already exists.</Message>"
                + "<RequestId>0000</RequestId>"
                + "</Error>";

        ServerResponse response = entityAlreadyExists();
        Assert.assertEquals(expectedResponseBody, response.getResponseBody());
        Assert.assertEquals(HttpResponseStatus.CONFLICT, response.getResponseStatus());
    }

    @Test
    public void testExpiredCredential() {
        final String expectedResponseBody = "<?xml version=\"1.0\" "
                + "encoding=\"UTF-8\" standalone=\"no\"?>"
                + "<Error xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">"
                + "<Code>ExpiredCredential</Code>"
                + "<Message>The request was rejected because the credential "
                + "used to sign the request has expired.</Message>"
                + "<RequestId>0000</RequestId>"
                + "</Error>";

        ServerResponse response = expiredCredential();
        Assert.assertEquals(expectedResponseBody, response.getResponseBody());
        Assert.assertEquals(HttpResponseStatus.UNAUTHORIZED, response.getResponseStatus());
    }

    @Test
    public void testInactiveAccessKey() {
        final String expectedResponseBody = "<?xml version=\"1.0\" "
                + "encoding=\"UTF-8\" standalone=\"no\"?>"
                + "<Error xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">"
                + "<Code>InactiveAccessKey</Code>"
                + "<Message>The access key used to sign the request is inactive."
                + "</Message>"
                + "<RequestId>0000</RequestId>"
                + "</Error>";

        ServerResponse response = inactiveAccessKey();
        Assert.assertEquals(expectedResponseBody, response.getResponseBody());
        Assert.assertEquals(HttpResponseStatus.UNAUTHORIZED,
                response.getResponseStatus());
    }

    @Test
    public void testSignatureDoesNotMatch() {
        final String expectedResponseBody = "<?xml version=\"1.0\" "
                + "encoding=\"UTF-8\" standalone=\"no\"?>"
                + "<Error xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">"
                + "<Code>SignatureDoesNotMatch</Code>"
                + "<Message>The request signature we calculated does not match "
                + "the signature you provided. Check your AWS secret access key "
                + "and signing method. For more information, see REST "
                + "Authentication andSOAP Authentication for details.</Message>"
                + "<RequestId>0000</RequestId>"
                + "</Error>";

        ServerResponse response = signatureDoesNotMatch();
        Assert.assertEquals(expectedResponseBody, response.getResponseBody());
        Assert.assertEquals(HttpResponseStatus.UNAUTHORIZED, response.getResponseStatus());
    }

    @Test
    public void testInternalServerError() {
        final String expectedResponseBody = "<?xml version=\"1.0\" "
                + "encoding=\"UTF-8\" standalone=\"no\"?>"
                + "<Error xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">"
                + "<Code>InternalFailure</Code>"
                + "<Message>The request processing has failed because of an "
                + "unknown error, exception or failure.</Message>"
                + "<RequestId>0000</RequestId>"
                + "</Error>";

        ServerResponse response = internalServerError();
        Assert.assertEquals(expectedResponseBody, response.getResponseBody());
        Assert.assertEquals(HttpResponseStatus.INTERNAL_SERVER_ERROR,
                response.getResponseStatus());
    }

    @Test
    public void testInvalidAction() {
        final String expectedResponseBody = "<?xml version=\"1.0\" "
                + "encoding=\"UTF-8\" standalone=\"no\"?>"
                + "<Error xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">"
                + "<Code>InvalidAction</Code>"
                + "<Message>The action or operation requested is invalid. "
                + "Verify that the action is typed correctly.</Message>"
                + "<RequestId>0000</RequestId>"
                + "</Error>";

        ServerResponse response = invalidAction();
        Assert.assertEquals(expectedResponseBody, response.getResponseBody());
        Assert.assertEquals(HttpResponseStatus.BAD_REQUEST,
                response.getResponseStatus());
    }

    @Test
    public void testInvalidClientTokenId() {
        final String expectedResponseBody = "<?xml version=\"1.0\" "
                + "encoding=\"UTF-8\" standalone=\"no\"?>"
                + "<Error xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">"
                + "<Code>InvalidClientTokenId</Code>"
                + "<Message>The X.509 certificate or AWS access key ID provided "
                + "does not exist in our records.</Message>"
                + "<RequestId>0000</RequestId>"
                + "</Error>";

        ServerResponse response = invalidClientTokenId();
        Assert.assertEquals(expectedResponseBody, response.getResponseBody());
        Assert.assertEquals(HttpResponseStatus.UNAUTHORIZED,
                response.getResponseStatus());
    }

    @Test
    public void testInvalidParameterValue() {
        final String expectedResponseBody = "<?xml version=\"1.0\" "
                + "encoding=\"UTF-8\" standalone=\"no\"?>"
                + "<Error xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">"
                + "<Code>InvalidParameterValue</Code>"
                + "<Message>An invalid or out-of-range value was supplied for "
                + "the input parameter.</Message>"
                + "<RequestId>0000</RequestId>"
                + "</Error>";

        ServerResponse response = invalidParametervalue();
        Assert.assertEquals(expectedResponseBody, response.getResponseBody());
        Assert.assertEquals(HttpResponseStatus.BAD_REQUEST,
                response.getResponseStatus());
    }

    @Test
    public void testMissingParameter() {
        final String expectedResponseBody = "<?xml version=\"1.0\" "
                + "encoding=\"UTF-8\" standalone=\"no\"?>"
                + "<Error xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">"
                + "<Code>MissingParameter</Code>"
                + "<Message>A required parameter for the specified action is "
                + "not supplied.</Message>"
                + "<RequestId>0000</RequestId>"
                + "</Error>";

        ServerResponse response = missingParameter();
        Assert.assertEquals(expectedResponseBody, response.getResponseBody());
        Assert.assertEquals(HttpResponseStatus.BAD_REQUEST, response.getResponseStatus());
    }

    @Test
    public void testNoSuchEntity() {
        final String expectedResponseBody = "<?xml version=\"1.0\" "
                + "encoding=\"UTF-8\" standalone=\"no\"?>"
                + "<Error xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">"
                + "<Code>NoSuchEntity</Code>"
                + "<Message>The request was rejected because it referenced an "
                + "entity that does not exist. </Message>"
                + "<RequestId>0000</RequestId>"
                + "</Error>";

        ServerResponse response = noSuchEntity();
        Assert.assertEquals(expectedResponseBody, response.getResponseBody());
        Assert.assertEquals(HttpResponseStatus.UNAUTHORIZED,
                response.getResponseStatus());
    }

    @Test
    public void testOKResponse() {
        final String expectedResponseBody = "Action successful";

        ServerResponse response = ok();
        Assert.assertEquals(expectedResponseBody, response.getResponseBody());
        Assert.assertEquals(HttpResponseStatus.OK,
                response.getResponseStatus());
    }

}
