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

package com.seagates3.authorization;

import com.seagates3.authorization.IAMApiAuthorizer;
import com.seagates3.exception.InvalidUserException;
import com.seagates3.model.Account;
import com.seagates3.model.Requestor;
import org.junit.rules.ExpectedException;
import org.junit.Rule;
import org.junit.Test;
import static org.junit.Assert.*;
import java.util.Map;
import java.util.TreeMap;

public class IAMApiAuthorizerTest {

    /*
     *  Set up for tests
     */
    @Rule
    public ExpectedException exception = ExpectedException.none();

    private Map<String, String> requestBody = new TreeMap<>();
    private Requestor requestor = new Requestor();

    /*
     * Test for the scenario:
     *
     * When root user's access key and secret key are given,
     * validateIfUserCanPerformAction() function should return true.
     */
    @Test
    public void validateIfUserCanPerformAction_Positive_Test() {

        IAMApiAuthorizer iamApiAuthorizer  = new IAMApiAuthorizer();
        requestor.setName("root");
        requestBody.put("UserName", "usr2");;

        assertEquals(true,
                iamApiAuthorizer.validateIfUserCanPerformAction(requestor,
                requestBody));
    }

    /*
     * Test for the scenario:
     *
     * When access key and secret key belong to non root user(i.e. usr1) and
     * attempting to perform operation on another user(i.e. usr2),
     * validateIfUserCanPerformAction() function should return false.
     */
    @Test
    public void validateIfUserCanPerformAction_Negative_Test() {

        IAMApiAuthorizer iamApiAuthorizer  = new IAMApiAuthorizer();
        requestor.setName("usr1");
        requestBody.put("UserName", "usr2");

        assertEquals(false,
                iamApiAuthorizer.validateIfUserCanPerformAction(requestor,
                requestBody));
    }

    /*
     * Test for the scenario:
     *
     * When access key and secret key belong to non root user(i.e. usr1) and
     * attempting to perform operation on another user(i.e. usr2),
     * authorize() function should throw InvalidUserException.
     *
     */
    @Test
    public void authorize_ThrowsInvalidUserExceptionTest() throws
        InvalidUserException{

        IAMApiAuthorizer iamApiAuthorizer  = new IAMApiAuthorizer();
        requestor.setName("usr1");
        requestBody.put("UserName", "usr2");

        exception.expect(InvalidUserException.class);
        iamApiAuthorizer.authorize(requestor, requestBody);
    }

    /*
     * Test for the scenario:
     *
     * When root user's access key and secret key are given,
     * authorize() function should pass.
     */
    @Test
    public void authorize_DoesNotThrowInvalidUserExceptionTest1() throws
        InvalidUserException{

        IAMApiAuthorizer iamApiAuthorizer  = new IAMApiAuthorizer();
        requestor.setName("root");
        requestBody.put("UserName", "usr2");

        iamApiAuthorizer.authorize(requestor, requestBody);
    }

    /*
     * Test for the scenario:
     *
     * When non user's access key and secret key are given and
     * attempting to perform operation on self,
     * authorize() function should pass.
     */
    @Test
    public void authorize_DoesNotThrowInvalidUserExceptionTest2() throws
        InvalidUserException{

        IAMApiAuthorizer iamApiAuthorizer  = new IAMApiAuthorizer();
        requestor.setName("usr2");
        requestBody.put("UserName", "usr2");

        iamApiAuthorizer.authorize(requestor, requestBody);
    }

    /**
    * Below test will check invalid user authorization
    * @throws InvalidUserException
    */
    @Test public void authorizeRootUserExceptionTest()
        throws InvalidUserException {
      IAMApiAuthorizer iamApiAuthorizer = new IAMApiAuthorizer();
      requestor.setName("usr1");
      requestBody.put("UserName", "user1");
      exception.expect(InvalidUserException.class);
      iamApiAuthorizer.authorizeRootUser(requestor, requestBody);
    }

    /**
    * Below test will check different user authorization
    * @throws InvalidUserException
    */
    @Test public void authorizeRootUserDifferentUserTest()
        throws InvalidUserException {
      IAMApiAuthorizer iamApiAuthorizer = new IAMApiAuthorizer();
      Account account = new Account();
      account.setName("user1");
      requestor.setAccount(account);
      requestor.setName("root");
      requestBody.put("AccountName", "user2");
      exception.expect(InvalidUserException.class);
      iamApiAuthorizer.authorizeRootUser(requestor, requestBody);
    }
    /**
    * Below test will check non root user authorization
    * @throws InvalidUserException
    */
    @Test public void authorizeRootUserNonRootUserTest()
        throws InvalidUserException {
      IAMApiAuthorizer iamApiAuthorizer = new IAMApiAuthorizer();
      Account account = new Account();
      account.setName("user1");
      requestor.setAccount(account);
      requestor.setName("user1");
      requestBody.put("UserName", "user2");
      exception.expect(InvalidUserException.class);
      iamApiAuthorizer.authorizeRootUser(requestor, requestBody);
    }
}
