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
 * Original author: Sushant Mane <sushant.mane@seagate.com>
 * Original creation date: 26-Dec-2016
 */
package com.seagates3.authentication;

import com.seagates3.model.Requestor;
import com.seagates3.response.ServerResponse;
import io.netty.handler.codec.http.HttpResponseStatus;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.powermock.api.mockito.PowerMockito;
import org.powermock.core.classloader.annotations.PrepareForTest;
import org.powermock.modules.junit4.PowerMockRunner;
import org.powermock.reflect.internal.WhiteboxImpl;

import static junit.framework.TestCase.assertFalse;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;
import static org.powermock.api.mockito.PowerMockito.doReturn;

@RunWith(PowerMockRunner.class)
@PrepareForTest(SignatureValidator.class)
public class SignatureValidatorTest {

    private ClientRequestToken clientRequestToken;
    private SignatureValidator signatureValidator;

    @Before
    public void setUp() {
        clientRequestToken = mock(ClientRequestToken.class);
        signatureValidator = new SignatureValidator();
    }

    @Test
    public void validateTest_ShouldReturnOk() throws Exception {
        // Arrange
        Requestor requestor = mock(Requestor.class);

        AWSSign awsSign = mock(AWSSign.class);
        SignatureValidator signatureValidatorSpy = PowerMockito.spy(signatureValidator);
        doReturn(awsSign).when(signatureValidatorSpy, "getSigner", clientRequestToken);
        when(awsSign.authenticate(clientRequestToken, requestor)).thenReturn(Boolean.TRUE);

        // Act
        ServerResponse response = signatureValidatorSpy.validate(clientRequestToken, requestor);

        // Verify
        assertEquals(HttpResponseStatus.OK, response.getResponseStatus());
    }

    @Test
    public void validateTest_ShouldReturnSignatureDoesNotMatch() throws Exception {
        // Arrange
        Requestor requestor = mock(Requestor.class);

        AWSSign awsSign = mock(AWSSign.class);
        SignatureValidator signatureValidatorSpy = PowerMockito.spy(signatureValidator);
        doReturn(awsSign).when(signatureValidatorSpy, "getSigner", clientRequestToken);
        when(awsSign.authenticate(clientRequestToken, requestor)).thenReturn(Boolean.FALSE);

        // Act
        ServerResponse response = signatureValidatorSpy.validate(clientRequestToken, requestor);

        // Verify
        assertEquals(HttpResponseStatus.UNAUTHORIZED, response.getResponseStatus());
    }

    @Test
    public void getSignerTest_AWSSignV2() throws Exception {
        // Arrange
        when(clientRequestToken.getSignVersion()).thenReturn(ClientRequestToken.AWSSigningVersion.V2);

        // Act
        AWSSign awsSign = WhiteboxImpl.invokeMethod(signatureValidator, "getSigner", clientRequestToken);

        // Verify
        assertTrue(awsSign instanceof AWSV2Sign);
        assertFalse(awsSign instanceof AWSV4Sign);
    }

    @Test
    public void getSignerTest_AWSSignV4() throws Exception {
        // Arrange
        when(clientRequestToken.getSignVersion()).thenReturn(ClientRequestToken.AWSSigningVersion.V4);

        // Act
        AWSSign awsSign = WhiteboxImpl.invokeMethod(signatureValidator, "getSigner", clientRequestToken);

        // Verify
        assertTrue(awsSign instanceof AWSV4Sign);
        assertFalse(awsSign instanceof AWSV2Sign);
    }
}
