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

import org.joda.time.DateTime;
import org.joda.time.DateTimeZone;
import org.junit.Test;

import java.util.Date;

import static org.junit.Assert.*;

public class DateUtilTest {

    @Test
    public void toLdapDateTest() {
        String expected = "20170110094320Z";
        Date date = new Date(1484041400136L);

        String actual = DateUtil.toLdapDate(date);

        assertEquals(expected, actual);
    }

    @Test
    public void toLdapDateTest_DateAsLdapDateString() {
        String date = "20170110093501";
        String expected = "20170110093501Z";

        String actual = DateUtil.toLdapDate(date);

        assertEquals(expected, actual);
    }

    @Test
    public void toLdapDateTest_DateAsServerResponseDateString() {
        String date = "2017-01-10T09:43:20.136+0000";
        String expected = "20170110094320Z";

        String actual = DateUtil.toLdapDate(date);

        assertEquals(expected, actual);
    }

    @Test(expected = NullPointerException.class)
    public void toLdapDateTest_DateAsString_ShouldThrowException() {
        DateUtil.toLdapDate("WrongDateFormat");
    }

    @Test
    public void toServerResponseFormatTest_Date() {
        String expected = "2017-01-10T09:43:20.136+0000";
        Date date = new Date(1484041400136L);

        String actual = DateUtil.toServerResponseFormat(date);

        assertEquals(expected, actual);
    }

    @Test
    public void toServerResponseFormatTest_DateTime() {
        String expected = "2017-01-10T13:02:47.806+0000";
        DateTime date = new DateTime("2017-01-10T08:02:47.806-05:00", DateTimeZone.UTC);

        String actual = DateUtil.toServerResponseFormat(date);

        assertEquals(expected, actual);
    }

    @Test
    public void toServerResponseFormatTest_LdapDate() {
        String expected = "2017-01-10T09:43:20.000+0000";
        String ldapDate = "20170110094320Z";

        String actual = DateUtil.toServerResponseFormat(ldapDate);

        assertEquals(expected, actual);
    }

    @Test
    public void getCurrentDateTimeTest() {
        DateTime result = DateUtil.getCurrentDateTime();

        assertEquals("UTC", result.getZone().toString());
    }

    @Test
    public void getCurrentTimeTest() {
        Object result = DateUtil.getCurrentTime();

        assertNotNull(result);
        assertTrue(result instanceof Long);
    }

    @Test
    public void toDateTimeTest() {
        String expected = "2017-01-10T13:02:47.806Z";

        Object result = DateUtil.toDateTime("2017-01-10T08:02:47.806-05:00");

        assertNotNull(result);
        assertTrue(result instanceof DateTime);
        assertEquals(expected, result.toString());
    }

    @Test
    public void toDateTimeTest_LdapDate() {
        String expected = "2017-01-10T09:43:20.000Z";

        Object result = DateUtil.toDateTime("20170110094320");

        assertNotNull(result);
        assertTrue(result instanceof DateTime);
        assertEquals(expected, result.toString());
    }
}
