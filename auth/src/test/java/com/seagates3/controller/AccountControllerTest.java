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
 * Original creation date: 29-Dec-2015
 */
package com.seagates3.controller;

import com.seagates3.dao.AccessKeyDAO;
import com.seagates3.dao.AccountDAO;
import com.seagates3.dao.DAODispatcher;
import com.seagates3.dao.DAOResource;
import com.seagates3.dao.UserDAO;
import com.seagates3.exception.DataAccessException;
import com.seagates3.model.AccessKey;
import com.seagates3.model.Account;
import com.seagates3.model.Requestor;
import com.seagates3.model.User;
import com.seagates3.response.ServerResponse;
import com.seagates3.util.KeyGenUtil;
import io.netty.handler.codec.http.HttpResponseStatus;
import java.util.HashMap;
import java.util.Map;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import static org.mockito.Matchers.any;
import org.mockito.Mockito;
import static org.mockito.Mockito.mock;
import org.powermock.api.mockito.PowerMockito;
import org.powermock.core.classloader.annotations.PowerMockIgnore;
import org.powermock.core.classloader.annotations.PrepareForTest;
import org.powermock.modules.junit4.PowerMockRunner;

@RunWith(PowerMockRunner.class)
@PrepareForTest({DAODispatcher.class, KeyGenUtil.class})
@PowerMockIgnore({"javax.management.*"})
public class AccountControllerTest {

    private final AccountController accountController;
    private final AccountDAO accountDAO;
    private final UserDAO userDAO;
    private final AccessKeyDAO accessKeyDAO;

    public AccountControllerTest() throws Exception {
        PowerMockito.mockStatic(DAODispatcher.class);

        Requestor requestor = mock(Requestor.class);
        Map<String, String> requestBody = new HashMap<>();
        requestBody.put("AccountName", "s3test");

        accountDAO = Mockito.mock(AccountDAO.class);
        userDAO = Mockito.mock(UserDAO.class);
        accessKeyDAO = Mockito.mock(AccessKeyDAO.class);

        PowerMockito.doReturn(accountDAO).when(DAODispatcher.class,
                "getResourceDAO", DAOResource.ACCOUNT
        );

        PowerMockito.doReturn(userDAO).when(DAODispatcher.class,
                "getResourceDAO", DAOResource.USER
        );

        PowerMockito.doReturn(accessKeyDAO).when(DAODispatcher.class,
                "getResourceDAO", DAOResource.ACCESS_KEY
        );

        accountController = new AccountController(requestor, requestBody);
    }

    @Before
    public void setUp() throws Exception {
        PowerMockito.mockStatic(DAODispatcher.class);
        PowerMockito.mockStatic(KeyGenUtil.class);

        PowerMockito.doReturn("987654test").when(KeyGenUtil.class,
                "userId"
        );
    }

    @Test
    public void CreateAccount_AccountSearchFailed_ReturnInternalServerError()
            throws Exception {
        Mockito.when(accountDAO.find("s3test")).thenThrow(
                new DataAccessException("failed to search account.\n"));

        final String expectedResponseBody = "<?xml version=\"1.0\" "
                + "encoding=\"UTF-8\" standalone=\"no\"?>"
                + "<Error xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">"
                + "<Code>InternalFailure</Code>"
                + "<Message>The request processing has failed because of an "
                + "unknown error, exception or failure.</Message>"
                + "<RequestId>0000</RequestId>"
                + "</Error>";

        ServerResponse response = accountController.create();
        Assert.assertEquals(expectedResponseBody, response.getResponseBody());
        Assert.assertEquals(HttpResponseStatus.INTERNAL_SERVER_ERROR,
                response.getResponseStatus());
    }

    @Test
    public void CreateAccount_AccountExists_ReturnEntityAlreadyExists()
            throws Exception {
        Account account = new Account();
        account.setId("12345");
        account.setName("s3test");

        Mockito.when(accountDAO.find("s3test")).thenReturn(account);

        final String expectedResponseBody = "<?xml version=\"1.0\" "
                + "encoding=\"UTF-8\" standalone=\"no\"?>"
                + "<Error xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">"
                + "<Code>EntityAlreadyExists</Code>"
                + "<Message>The request was rejected because it attempted to "
                + "create an account that already exists.</Message>"
                + "<RequestId>0000</RequestId>"
                + "</Error>";

        ServerResponse response = accountController.create();
        Assert.assertEquals(expectedResponseBody, response.getResponseBody());
        Assert.assertEquals(HttpResponseStatus.CONFLICT,
                response.getResponseStatus());
    }

    @Test
    public void CreateAccount_AccountSaveFailed_ReturnInternalServerError()
            throws Exception {
        Account account = new Account();
        account.setName("s3test");

        Mockito.doReturn(account).when(accountDAO).find("s3test");
        Mockito.doThrow(new DataAccessException("failed to add new account.\n"))
                .when(accountDAO).save(account);

        final String expectedResponseBody = "<?xml version=\"1.0\" "
                + "encoding=\"UTF-8\" standalone=\"no\"?>"
                + "<Error xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">"
                + "<Code>InternalFailure</Code>"
                + "<Message>The request processing has failed because of an "
                + "unknown error, exception or failure.</Message>"
                + "<RequestId>0000</RequestId>"
                + "</Error>";

        ServerResponse response = accountController.create();
        Assert.assertEquals(expectedResponseBody, response.getResponseBody());
        Assert.assertEquals(HttpResponseStatus.INTERNAL_SERVER_ERROR,
                response.getResponseStatus());
    }

    @Test
    public void CreateAccount_FailedToCreateRootUser_ReturnInternalServerError()
            throws Exception {
        Account account = new Account();
        account.setName("s3test");

        Mockito.doReturn(account).when(accountDAO).find("s3test");
        Mockito.doNothing().when(accountDAO).save(any(Account.class));
        Mockito.doThrow(new DataAccessException("failed to save new user.\n"))
                .when(userDAO).save(any(User.class));

        final String expectedResponseBody = "<?xml version=\"1.0\" "
                + "encoding=\"UTF-8\" standalone=\"no\"?>"
                + "<Error xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">"
                + "<Code>InternalFailure</Code>"
                + "<Message>The request processing has failed because of an "
                + "unknown error, exception or failure.</Message>"
                + "<RequestId>0000</RequestId>"
                + "</Error>";

        ServerResponse response = accountController.create();
        Assert.assertEquals(expectedResponseBody, response.getResponseBody());
        Assert.assertEquals(HttpResponseStatus.INTERNAL_SERVER_ERROR,
                response.getResponseStatus());
    }

    @Test
    public void CreateAccount_FailedToCreateRootAccessKey_ReturnInternalServerError()
            throws Exception {
        Account account = new Account();
        account.setName("s3test");

        Mockito.doReturn(account).when(accountDAO).find("s3test");
        Mockito.doNothing().when(accountDAO).save(any(Account.class));
        Mockito.doNothing().when(userDAO).save(any(User.class));
        Mockito.doThrow(new DataAccessException("failed to save root access key.\n"))
                .when(accessKeyDAO).save(any(AccessKey.class));

        final String expectedResponseBody = "<?xml version=\"1.0\" "
                + "encoding=\"UTF-8\" standalone=\"no\"?>"
                + "<Error xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">"
                + "<Code>InternalFailure</Code>"
                + "<Message>The request processing has failed because of an "
                + "unknown error, exception or failure.</Message>"
                + "<RequestId>0000</RequestId>"
                + "</Error>";

        ServerResponse response = accountController.create();
        Assert.assertEquals(expectedResponseBody, response.getResponseBody());
        Assert.assertEquals(HttpResponseStatus.INTERNAL_SERVER_ERROR,
                response.getResponseStatus());
    }

    @Test
    public void CreateAccount_Success_ReturnCreateResponse()
            throws Exception {
        Account account = new Account();
        account.setName("s3test");

        PowerMockito.doReturn("AKIASIAS").when(KeyGenUtil.class,
                "userAccessKeyId"
        );

        PowerMockito.doReturn("htuspscae/123").when(KeyGenUtil.class,
                "userSercretKey", any(String.class)
        );

        Mockito.doReturn(account).when(accountDAO).find("s3test");
        Mockito.doNothing().when(accountDAO).save(any(Account.class));
        Mockito.doNothing().when(userDAO).save(any(User.class));
        Mockito.doNothing().when(accessKeyDAO).save(any(AccessKey.class));

        final String expectedResponseBody = "<?xml version=\"1.0\" "
                + "encoding=\"UTF-8\" standalone=\"no\"?>"
                + "<CreateAccountResponse "
                + "xmlns=\"https://iam.seagate.com/doc/2010-05-08/\">"
                + "<CreateAccountResult>"
                + "<Account>"
                + "<AccountId>987654test</AccountId>"
                + "<AccountName>s3test</AccountName>"
                + "<RootUserName>root</RootUserName>"
                + "<AccessKeyId>AKIASIAS</AccessKeyId>"
                + "<RootSecretKeyId>htuspscae/123</RootSecretKeyId>"
                + "<Status>Active</Status>"
                + "</Account>"
                + "</CreateAccountResult>"
                + "<ResponseMetadata>"
                + "<RequestId>0000</RequestId>"
                + "</ResponseMetadata>"
                + "</CreateAccountResponse>";

        ServerResponse response = accountController.create();
        Assert.assertEquals(expectedResponseBody, response.getResponseBody());
        Assert.assertEquals(HttpResponseStatus.CREATED,
                response.getResponseStatus());
    }
}
