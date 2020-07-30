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
