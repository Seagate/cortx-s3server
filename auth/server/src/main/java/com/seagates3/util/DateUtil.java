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

import java.text.DateFormat;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.TimeZone;

import org.joda.time.DateTime;
import org.joda.time.DateTimeZone;
import org.joda.time.format.DateTimeFormat;
import org.joda.time.format.DateTimeFormatter;

/*
 * TODO
 * replace java.util.date with org.joda.time.DateTime.
 */
public class DateUtil {

    private static final String LDAP_DATE_FORMAT = "yyyyMMddHHmmss";
    private static final String SERVER_RESPONSE_DATE_FORMAT
            = "yyyy-MM-dd'T'HH:mm:ss.SSSZ";

    public static String toLdapDate(Date date) {
        SimpleDateFormat sdf = getSimpleDateFormat(LDAP_DATE_FORMAT);
        return (sdf.format(date) + "Z");
    }

    public static String toLdapDate(String date) {
        return toLdapDate(toDate(date));
    }

    public static String toServerResponseFormat(Date date) {
        SimpleDateFormat serverResponseFormat
                = getSimpleDateFormat(SERVER_RESPONSE_DATE_FORMAT);

        return serverResponseFormat.format(date);
    }

    public static String toServerResponseFormat(DateTime date) {
        DateTimeFormatter fmt = DateTimeFormat.forPattern(
                SERVER_RESPONSE_DATE_FORMAT);
        return fmt.print(date);
    }

    public static String toServerResponseFormat(String ldapDate) {
        return toServerResponseFormat(toDate(ldapDate));
    }

    public static DateTime getCurrentDateTime() {
        return DateTime.now(DateTimeZone.UTC);
    }

     /*
     * <IEM_INLINE_DOCUMENTATION>
     *     <event_code>048003001</event_code>
     *     <application>S3 Authserver</application>
     *     <submodule>Parser</submodule>
     *     <description>Unparseable date</description>
     *     <audience>Development</audience>
     *     <details>
     *         Unparseable date.
     *         The data section of the event has following keys:
     *           time - timestamp
     *           node - node name
     *           pid - process id of Authserver
     *           file - source code filename
     *           line - line number within file where error occurred
     *     </details>
     *     <service_actions>
     *         Save authserver log files and contact development team for
     *         further investigation.
     *     </service_actions>
     * </IEM_INLINE_DOCUMENTATION>
     *
     */
    /**
     * Return the current time in UTC.
     */
    public static long getCurrentTime() {
        SimpleDateFormat dateFormatGmt = getSimpleDateFormat("yyyy-MMM-dd HH:mm:ss");
        dateFormatGmt.setTimeZone(TimeZone.getTimeZone("UTC"));

        try {
            return dateFormatGmt.parse(dateFormatGmt.format(new Date())).getTime();
        } catch (ParseException ex) {
            IEMUtil.log(IEMUtil.Level.ERROR, IEMUtil.UNPARSABLE_DATE,
                    "Unparseable date", null);
        }

        return 0;
    }

    public static DateTime toDateTime(String date) {
        DateTimeFormatter dateFormatter = getDateTimeFormat(LDAP_DATE_FORMAT);

        try {
            return dateFormatter.parseDateTime(date);
        } catch (Exception ex) {
        }

        dateFormatter = getDateTimeFormat(SERVER_RESPONSE_DATE_FORMAT);
        return dateFormatter.parseDateTime(date);

    }

    public static Date toDate(String date) {
        SimpleDateFormat sdf = getSimpleDateFormat(LDAP_DATE_FORMAT);

        try {
            return sdf.parse(date);
        } catch (ParseException e) {
        }

        sdf = getSimpleDateFormat(SERVER_RESPONSE_DATE_FORMAT);
        try {
            return sdf.parse(date);
        } catch (ParseException e) {
            IEMUtil.log(IEMUtil.Level.ERROR, IEMUtil.UNPARSABLE_DATE,
                    "Unparseable date", null);
        }

        return null;
    }

    private static DateTimeFormatter getDateTimeFormat(String pattern) {
        DateTimeFormatter dtf = DateTimeFormat.forPattern(pattern);
        return dtf.withZoneUTC();
    }

    private static SimpleDateFormat getSimpleDateFormat(String pattern) {
        SimpleDateFormat sdf = new SimpleDateFormat(pattern);
        sdf.setTimeZone(getDefatulTimeZone());

        return sdf;
    }

    /**
     * Return the default time zone (UTC).
     *
     * @return UTC Time zone.
     */
    private static TimeZone getDefatulTimeZone() {
        return TimeZone.getTimeZone("UTC");
    }
    /**
     * Return the current time in GMT.
     * Returns Date in format "Thu, 05 Jul 2018 03:55:43 GMT"
     */
    public static String getCurrentTimeGMT() {
        DateFormat df = new SimpleDateFormat("EEE, dd MMM yyyy HH:mm:ss ");
        df.setTimeZone(TimeZone.getTimeZone("GMT"));
        String d = df.format(new Date()) + "GMT";
        return d;
    }

    /**
     * Returns Date in given pattern in UTC format.
     */
   public
    static Date parseDateString(String dateString, String pattern) {

      Date date = null;
      SimpleDateFormat sdf = new SimpleDateFormat(pattern);
      sdf.setTimeZone(TimeZone.getTimeZone("UTC"));
      if (dateString == null || dateString.trim().isEmpty()) {
        return null;
      }
      try {
        date = sdf.parse(dateString);
        return date;
      }
      catch (ParseException e) {
      }
      return null;
    }
}
