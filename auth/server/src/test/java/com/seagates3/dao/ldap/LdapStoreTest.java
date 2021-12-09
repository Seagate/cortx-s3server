package com.seagates3.dao.ldap;

import static org.mockito.Matchers.any;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mockito;
import org.powermock.api.mockito.PowerMockito;
import org.powermock.core.classloader.annotations.PowerMockIgnore;
import org.powermock.core.classloader.annotations.PrepareForTest;
import org.powermock.modules.junit4.PowerMockRunner;

import com.novell.ldap.LDAPAttribute;
import com.novell.ldap.LDAPEntry;
import com.novell.ldap.LDAPSearchResults;
import com.seagates3.dao.ldap.LDAPUtils;
import com.seagates3.dao.ldap.LdapStore;
import com.seagates3.model.Account;
import com.seagates3.model.Policy;
import com.seagates3.util.DateUtil;

@RunWith(PowerMockRunner.class)
    @PrepareForTest({LDAPUtils.class, LdapStore.class, DateUtil.class})
    @PowerMockIgnore({"javax.management.*"}) public class LdapStoreTest {

 private
  LdapStore ldapstore;
 private
  Policy policy;
 private
  Account account;
 private
  LDAPSearchResults ldapResults;

  @Before public void setUp() throws Exception {
    LDAPEntry entry;
    LDAPAttribute ldapAttr;
    ldapResults = Mockito.mock(LDAPSearchResults.class);
    entry = Mockito.mock(LDAPEntry.class);
    ldapAttr = Mockito.mock(LDAPAttribute.class);

    Mockito.when(ldapResults.next()).thenReturn(entry);
    Mockito.when(ldapAttr.getStringValue()).thenReturn("1");
    Mockito.when(entry.getAttribute(LDAPUtils.POLICY_ID)).thenReturn(ldapAttr);
    Mockito.when(entry.getAttribute(LDAPUtils.PATH)).thenReturn(ldapAttr);
    Mockito.when(entry.getAttribute(LDAPUtils.DEFAULT_VERSION_ID))
        .thenReturn(ldapAttr);
    Mockito.when(entry.getAttribute(LDAPUtils.POLICY_DOC)).thenReturn(ldapAttr);
    Mockito.when(entry.getAttribute(LDAPUtils.POLICY_NAME))
        .thenReturn(ldapAttr);
    Mockito.when(entry.getAttribute(LDAPUtils.POLICY_ARN)).thenReturn(ldapAttr);
    Mockito.when(entry.getAttribute(LDAPUtils.POLICY_ATTACHMENT_COUNT))
        .thenReturn(ldapAttr);
    Mockito.when(entry.getAttribute(LDAPUtils.IS_POLICY_ATTACHABLE))
        .thenReturn(ldapAttr);
    Mockito.when(entry.getAttribute(LDAPUtils.POLICY_PERMISSION_BOUNDARY))
        .thenReturn(ldapAttr);
    Mockito.when(entry.getAttribute(LDAPUtils.POLICY_CREATE_DATE))
        .thenReturn(ldapAttr);
    Mockito.when(entry.getAttribute(LDAPUtils.POLICY_UPDATE_DATE))
        .thenReturn(ldapAttr);

    account = new Account();
    account.setId("A1234");
    account.setName("account1");

    policy = new Policy();
    policy.setPolicyId("P1234");
    policy.setPath("/");
    policy.setName("policy1");
    policy.setAccount(account);
    policy.setARN("arn:aws:iam::A1234:policy/policy1");
    policy.setCreateDate("2021/12/12 12:23:34");
    policy.setDefaultVersionId("0");
    policy.setAttachmentCount(0);
    policy.setPermissionsBoundaryUsageCount(0);
    policy.setIsPolicyAttachable("true");
    policy.setUpdateDate("2021/12/12 12:23:34");
    policy.setPolicyDoc(
        "{\r\n" + "  \"Id\": \"Policy1632740111416\",\r\n" +
        "  \"Version\": \"2012-10-17\",\r\n" + "  \"Statement\": [\r\n" +
        "    {\r\n" + "      \"Sid\": \"Stmt1632740110513\",\r\n" +
        "      \"Action\": [\r\n" + "        \"s3:PutBucketAcljhghsghsd\"\r\n" +
        "      ],\r\n" + "      \"Effect\": \"Allow\",\r\n" +
        "      \"Resource\": \"arn:aws:s3:::buck1\"\r\n" + "	  \r\n" +
        "    }\r\n" + "\r\n" + "  ]\r\n" + "}");

    ldapstore = new LdapStore();

    PowerMockito.mockStatic(LDAPUtils.class);
  }

  @Test public void testSave() throws Exception {
    try {
      PowerMockito.doNothing().when(LDAPUtils.class, "add",
                                    any(LDAPEntry.class));
      Map sampleMap = new HashMap();
      sampleMap.put("", policy);
      ldapstore.save(sampleMap, "Policy");
    }
    catch (Exception e) {
      Assert.assertTrue(e.getMessage().contains("Failed to delete the policy"));
    }
  }

  @Test public void testFind() throws Exception {
    PowerMockito.mockStatic(DateUtil.class);
    PowerMockito.when(DateUtil.toServerResponseFormat(Mockito.anyString()))
        .thenReturn("");
    PowerMockito.when(LDAPUtils.class, "search", any(String.class),
                      any(Integer.class), any(String.class),
                      any(String[].class)).thenReturn(ldapResults);
    Mockito.when(ldapResults.hasMore()).thenReturn(Boolean.TRUE);
    Policy returnObj = (Policy)ldapstore.find("", policy, account, "Policy");
    Assert.assertEquals(true, returnObj.exists());
  }

  @Test public void testFindAll() throws Exception {
	Map<String, Object> parameters = new HashMap<>();
    PowerMockito.mockStatic(DateUtil.class);
    PowerMockito.when(DateUtil.toServerResponseFormat(Mockito.anyString()))
        .thenReturn("");
    PowerMockito.when(LDAPUtils.class, "search", any(String.class),
                      any(Integer.class), any(String.class),
                      any(String[].class)).thenReturn(ldapResults);
    List returnObj = ldapstore.findAll("", account, parameters, "Policy");
    Assert.assertEquals(true, returnObj.isEmpty());
  }

  @Test public void testDelete() throws Exception {
    try {
      PowerMockito.doNothing().when(LDAPUtils.class, "delete",
                                    any(String.class));
      ldapstore.delete ("", policy, "Policy");
    }
    catch (Exception e) {
      Assert.assertTrue(e.getMessage().contains("Failed to delete the policy"));
    }
  }

  @Test public void testFindNoMatch() throws Exception {
    PowerMockito.when(LDAPUtils.class, "search", any(String.class),
                      any(Integer.class), any(String.class),
                      any(String[].class)).thenReturn(null);
    Policy returnObj = (Policy)ldapstore.find("", policy, account, "Policy");
    Assert.assertEquals(false, returnObj.exists());
  }
}
