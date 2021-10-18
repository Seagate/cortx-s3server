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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.mockito.Matchers.any;
import static org.mockito.Mockito.mock;
import static org.powermock.api.mockito.PowerMockito.mockStatic;
import static org.powermock.api.mockito.PowerMockito.when;
import static org.powermock.api.mockito.PowerMockito.whenNew;

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.security.NoSuchAlgorithmException;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.powermock.core.classloader.annotations.PowerMockIgnore;
import org.powermock.core.classloader.annotations.PrepareForTest;
import org.powermock.modules.junit4.PowerMockRunner;

import com.seagates3.authserver.AuthServerConstants;

@RunWith(PowerMockRunner.class)
@PrepareForTest({KeyGenUtil.class})
@PowerMockIgnore("javax.management.*")
public class KeyGenUtilTest {

    @Before
    public void setUp() throws Exception {
        mockStatic(Runtime.class);
    }

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
    
    @Test
    public void testGenerateKeyByS3CipherUtil() throws Exception {
    	String sampleKey = "-WMAT3gNMCBIBEvfNwsxGHUNWFIzJa6iA-SLaI_hFZw=";
    	Runtime mockedRuntime = mock(Runtime.class);
    	InputStream mockedInputStream = mock(InputStream.class);
    	Process mockedS3Cipher = mock(Process.class);
    	InputStreamReader mockedInputStreamReader = mock(InputStreamReader.class);
    	BufferedReader mockedBufferedReader = mock(BufferedReader.class);

    	when(Runtime.getRuntime()).thenReturn(mockedRuntime);
    	when(mockedRuntime.exec(any(String.class))).thenReturn(mockedS3Cipher);
    	when(mockedS3Cipher.waitFor()).thenReturn(0);
    	when(mockedS3Cipher.getInputStream()).thenReturn(mockedInputStream);
    	when(mockedBufferedReader.readLine()).thenReturn(sampleKey);
        whenNew(InputStreamReader.class).withArguments(mockedInputStream).thenReturn(mockedInputStreamReader);
        whenNew(BufferedReader.class).withArguments(mockedInputStreamReader).thenReturn(mockedBufferedReader);
        
        String key = KeyGenUtil.generateKeyByS3CipherUtil("cortx-s3-secret-key");
		assertNotNull(key);
    }
}
