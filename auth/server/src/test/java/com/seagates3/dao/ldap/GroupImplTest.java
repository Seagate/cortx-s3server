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

package com.seagates3.dao.ldap;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mockito;
import org.mockito.internal.matchers.apachecommons.ReflectionEquals;
import org.powermock.api.mockito.PowerMockito;
import org.powermock.core.classloader.annotations.PowerMockIgnore;
import org.powermock.core.classloader.annotations.PrepareForTest;
import org.powermock.modules.junit4.PowerMockRunner;

import com.novell.ldap.LDAPAttribute;
import com.novell.ldap.LDAPEntry;
import com.novell.ldap.LDAPSearchResults;
import com.seagates3.model.Account;
import com.seagates3.model.Group;

@RunWith(PowerMockRunner.class) @PrepareForTest(LDAPUtils.class)
    @PowerMockIgnore(
        {"javax.management.*"}) public class GroupImplTest extends GroupImpl {

 private
  final String BASE_DN = "dc=s3,dc=seagate,dc=com";
 private
  LDAPSearchResults ldapResults;
 private
  GroupImpl groupImpl;
 private
  LDAPEntry entry;
 private
  LDAPAttribute idAttribute;
 private
  LDAPAttribute nameAttribute;
 private
  LDAPAttribute timestampAttribute;

  @Before public void setUp() throws Exception {
    PowerMockito.mockStatic(LDAPUtils.class);
    idAttribute = Mockito.mock(LDAPAttribute.class);
    nameAttribute = Mockito.mock(LDAPAttribute.class);
    timestampAttribute = Mockito.mock(LDAPAttribute.class);
    ldapResults = Mockito.mock(LDAPSearchResults.class);
    groupImpl = Mockito.spy(new GroupImpl());
    entry = Mockito.mock(LDAPEntry.class);
    Mockito.when(ldapResults.next()).thenReturn(entry);

    Mockito.when(entry.getAttribute("groupId")).thenReturn(idAttribute);
    Mockito.when(entry.getAttribute("groupName")).thenReturn(nameAttribute);
    Mockito.when(entry.getAttribute("createtimestamp"))
        .thenReturn(timestampAttribute);

    Mockito.when(idAttribute.getStringValue()).thenReturn("G1");
    Mockito.when(nameAttribute.getStringValue()).thenReturn("Group1");
    Mockito.when(timestampAttribute.getStringValue())
        .thenReturn("9999-12-31 23:59:59");
  }

  @Test public void findByPath_ReturnGroupObject_Success() throws Exception {

    String path = "http://s3.seagate.com/groups/global/AuthenticatedUsers";
    String[] attrs = {LDAPUtils.GROUP_ID, LDAPUtils.GROUP_NAME};
    String filter =
        String.format("(&(%s=%s)(%s=%s))", LDAPUtils.PATH, path,
                      LDAPUtils.OBJECT_CLASS, LDAPUtils.GROUP_OBJECT_CLASS);

    PowerMockito.doReturn(ldapResults)
        .when(LDAPUtils.class, "search", BASE_DN, 2, filter, attrs);

    Mockito.when(ldapResults.hasMore()).thenReturn(Boolean.TRUE);

    Group group = groupImpl.findByPath(path);
    Assert.assertThat(group, new ReflectionEquals(group));
  }

  @Test public void findByPathAndAccount_ReturnGroupObject_Success()
      throws Exception {
    Account account = new Account();
    account.setName("s3test");
    account.setId("98765test");
    account.setCanonicalId("C12345");
    account.setEmail("test@seagate.com");

    String path = "http://s3.seagate.com/groups/global/AuthenticatedUsers";
    String[] attrs = {LDAPUtils.GROUP_ID, LDAPUtils.GROUP_NAME,
                      LDAPUtils.CREATE_TIMESTAMP};

    String ldapBase = String.format(
        "%s=%s,%s=%s,%s=%s,%s", LDAPUtils.ORGANIZATIONAL_UNIT_NAME,
        LDAPUtils.GROUP_OU, LDAPUtils.ORGANIZATIONAL_NAME, account.getName(),
        LDAPUtils.ORGANIZATIONAL_UNIT_NAME, LDAPUtils.ACCOUNT_OU,
        LDAPUtils.BASE_DN);
    String filter = String.format("(%s=%s)", LDAPUtils.PATH, path);

    PowerMockito.doReturn(ldapResults)
        .when(LDAPUtils.class, "search", ldapBase, 2, filter, attrs);

    Mockito.when(ldapResults.hasMore()).thenReturn(Boolean.TRUE);

    Group group = groupImpl.findByPathAndAccount(account, path);
    Assert.assertThat(group, new ReflectionEquals(group));
  }
}
