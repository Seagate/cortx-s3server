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

package com.seagates3.fi;

import org.junit.Test;

import static org.junit.Assert.*;

public class FaultPointTest {

    private FaultPoint faultPoint;

    @Test
    public void isActiveTest_FAIL_ONCE() {
        faultPoint = new FaultPoint("LDAP_SEARCH", "FAIL_ONCE", 0);

        assertTrue(faultPoint.isActive());
        assertFalse(faultPoint.isActive());
        assertFalse(faultPoint.isActive());
    }

    @Test
    public void isActiveTest_FAIL_N_TIMES() {
        faultPoint = new FaultPoint("LDAP_SEARCH", "FAIL_N_TIMES", 2);

        assertTrue(faultPoint.isActive());
        assertTrue(faultPoint.isActive());
        assertFalse(faultPoint.isActive());
        assertFalse(faultPoint.isActive());
    }

    @Test
    public void isActiveTest_FAIL_0_TIMES() {
        faultPoint = new FaultPoint("LDAP_SEARCH", "FAIL_N_TIMES", 0);

        assertFalse(faultPoint.isActive());
        assertFalse(faultPoint.isActive());
    }

    @Test
    public void isActiveTest_SKIP_FIRST_N_TIMES() {
        faultPoint = new FaultPoint("LDAP_SEARCH", "SKIP_FIRST_N_TIMES", 2);

        assertFalse(faultPoint.isActive());
        assertFalse(faultPoint.isActive());
        assertTrue(faultPoint.isActive());
        assertTrue(faultPoint.isActive());
        assertTrue(faultPoint.isActive());
    }

    @Test
    public void isActiveTest_SKIP_FIRST_0_TIMES() {
        faultPoint = new FaultPoint("LDAP_SEARCH", "SKIP_FIRST_N_TIMES", 0);

        assertTrue(faultPoint.isActive());
        assertTrue(faultPoint.isActive());
        assertTrue(faultPoint.isActive());
    }

    @Test
    public void isActiveTest_FAIL_ALWAYS() {
        faultPoint = new FaultPoint("LDAP_SEARCH", "FAIL_ALWAYS", 0);

        assertTrue(faultPoint.isActive());
        assertTrue(faultPoint.isActive());
        assertTrue(faultPoint.isActive());
    }
}