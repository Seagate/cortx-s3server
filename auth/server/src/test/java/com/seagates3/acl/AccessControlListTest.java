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

package com.seagates3.acl;

import static org.junit.Assert.*;

import java.util.ArrayList;

import org.junit.Before;
import org.junit.Test;

import com.seagates3.authserver.AuthServerConfig;
import com.seagates3.exception.GrantListFullException;

public
class AccessControlListTest {

  AccessControlList acl = new AccessControlList();
  ArrayList<Grant> grantList;
  Grant defaultGrant;

  @Before public void setUp() throws Exception {
    grantList = new ArrayList<>();
    defaultGrant = new Grant(new Grantee("id1", "name1"), "FULL_CONTROL");
  }

  // Adds grant to grant list
  @Test public void testAddGrant_Success() throws GrantListFullException {
    acl.addGrant(defaultGrant);
    assertEquals(defaultGrant, acl.getGrantList().get(0));
    assertEquals(1, acl.getGrantList().size());
  }

  // Runs into GrantListFullException when adding more than 100 grants
  @Test(expected = GrantListFullException
                       .class) public void testAddGrant_GrantListFullException()
      throws GrantListFullException {
    for (int i = 0; i <= AuthServerConfig.MAX_GRANT_SIZE; i++)
      acl.addGrant(defaultGrant);
  }

  //  getGrant succeeds with matching canonical id
  @Test public void testGetGrant_Success() throws GrantListFullException {
    acl.addGrant(defaultGrant);
    assertEquals(defaultGrant, acl.getGrant("id1").get(0));
  }

  // getGrant returns null for invalid canonical id
  @Test public void testGetGrant_NotFound() throws GrantListFullException {
    acl.addGrant(defaultGrant);
    assertEquals(0, acl.getGrant("id2").size());
  }

  // getGrant returns null for null canonical id
  @Test public void testGetGrant_NullID() throws GrantListFullException {
    acl.addGrant(defaultGrant);
    assertEquals(null, acl.getGrant(null));
  }
}
