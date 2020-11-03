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

package com.seagates3.util;

import org.junit.Test;

import static org.junit.Assert.*;

import java.security.NoSuchAlgorithmException;
import com.seagates3.authserver.AuthServerConstants;

public class KeyGenUtilTest {

    @Test
    public void createUserIdTest() {
        String userID = KeyGenUtil.createUserId();

        assertNotNull(userID);
        assertFalse(userID.startsWith("-") || userID.startsWith("_"));
    }

    @Test public void createUserAccessKeyIdTest() {
      String accessKeyId = KeyGenUtil.createUserAccessKeyId(true);
      assertNotNull(accessKeyId);
      assertFalse(accessKeyId.contains("-"));
      assertTrue(
          accessKeyId.startsWith(AuthServerConstants.PERMANENT_KEY_PREFIX));
    }

    @Test public void createUserAccessKeyIdTestForTemporaryKey() {
      String accessKeyId = KeyGenUtil.createUserAccessKeyId(false);
        assertNotNull(accessKeyId);
        assertFalse(accessKeyId.contains("-"));
        assertTrue(
            accessKeyId.startsWith(AuthServerConstants.TEMPORARY_KEY_PREFIX));
    }

    @Test
    public void createUserSecretKeyTest() {
        String secretKey = KeyGenUtil.generateSecretKey();

        assertNotNull(secretKey);
        assertFalse(secretKey.startsWith("-") || secretKey.startsWith("_"));
        assertEquals(40, secretKey.length());
    }

    @Test
    public void createSessionIdTest() {
        String sessionId = KeyGenUtil.createSessionId("SomeRandomString");

        assertNotNull(sessionId);
        assertFalse(sessionId.startsWith("-") || sessionId.startsWith("_"));
    }

    @Test(expected = NullPointerException.class)
    public void createSessionIdTest_ShouldThrowException() {
        KeyGenUtil.createSessionId(null);
    }

    @Test
    public void createIdTest() {
        String id = KeyGenUtil.createId();

        assertNotNull(id);
        assertFalse(id.startsWith("-") || id.startsWith("_"));
    }

    @Test public void generateSSHATest() {
      String id = null;
      try {
        id = KeyGenUtil.generateSSHA("testtext");
      }
      catch (NoSuchAlgorithmException e) {
        e.printStackTrace();
        fail();
      }

      assertNotNull(id);
      assertTrue(id.startsWith("{SSHA}"));
    }

    @Test public void createCanonicalIdTest() {
      String id = null;
      // test multiple canonical ids and validate it
      for (int i = 0; i < 10; i++) {
        id = KeyGenUtil.createCanonicalId();

        assertNotNull(id);
        assertFalse(id.contains("-") || id.contains("_") || id.contains("&") ||
                    id.contains("+"));
        assertEquals(64, id.length());
      }
    }

    @Test public void createAccountIdTest() {
      String id = null;
      // test multiple account ids and validate it
      for (int i = 0; i < 10; i++) {
        id = KeyGenUtil.createAccountId();

        assertNotNull(id);
        assertFalse(id.contains("-") || id.contains("_") || id.contains("&") ||
                    id.contains("+"));
        assertEquals(12, id.length());
      }
    }

    @Test public void createIamUserIdTest() {
      String id = null;
      // test multiple iam user ids and validate it
      for (int i = 0; i < 10; i++) {
        id = KeyGenUtil.createIamUserId();

        assertNotNull(id);
        assertFalse(id.contains("-") || id.contains("_") || id.contains("&") ||
                    id.contains("+"));
        assertEquals(17, id.length());
      }
    }
}
