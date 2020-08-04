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

package com.seagates3.model;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;

public class AccountTest {

    private final String ACCOUNT_ID = "12345";
    private final String ACCOUNT_NAME = "admin";
    private final String CANONICAL_ID = "asdfghjkl";
    private final String EMAIL = "admin@seagate.com";
    private Account account;

    @Before
    public void setup() {
        account = new Account();
    }

    @Test
    public void accountIdGetSetTest() {
        account.setId(ACCOUNT_ID);
        Assert.assertEquals(ACCOUNT_ID, account.getId());
    }

    @Test
    public void accountNameGetSetTest() {
        account.setName(ACCOUNT_NAME);
        Assert.assertEquals(ACCOUNT_NAME, account.getName());
    }

    @Test
    public void accountCanonicalIdGetSetTest() {
        account.setCanonicalId(CANONICAL_ID);
        Assert.assertEquals(CANONICAL_ID, account.getCanonicalId());
    }

    @Test
    public void accountEmailGetSetTest() {
        account.setEmail(EMAIL);
        Assert.assertEquals(EMAIL, account.getEmail());
    }

    @Test
    public void accountExistsTest() {
        account.setId(ACCOUNT_ID);
        Assert.assertTrue(account.exists());
    }

    @Test
    public void accountDoesNotExistsTest() {
        account.setId(null);
        Assert.assertFalse(account.exists());
    }
}
